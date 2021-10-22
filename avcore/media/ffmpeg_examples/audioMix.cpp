/*
*一个简单的混音demo，把文件a和文件b的音频混为一个音频流输出并存为文件，只处理每个文件的第一个音频流
*源代码是网友Larry_Liang(1085803139)写的，我帮其调试通过
*MK(821486004@qq.com)
*/


extern "C" {
#include "libavcodec/avcodec.h"
#include "libavcodec/avfft.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libavfilter/avfilter.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/audio_fifo.h"
#include "libavutil/avutil.h"
#include "libavutil/fifo.h"
#include "libswresample/swresample.h"
}

#include <time.h>
#include <pthread.h>
#include <unistd.h>

enum CaptureState
{
    PREPARED,
    RUNNING,
    STOPPED,
    FINISHED
};

 typedef struct BufferSourceContext {
    const AVClass    *bscclass;
    AVFifoBuffer     *fifo;
    AVRational        time_base;     ///< time_base to set in the output link
    AVRational        frame_rate;    ///< frame_rate to set in the output link
    unsigned          nb_failed_requests;
    unsigned          warning_limit;

    /* video only */
    int               w, h;
    enum AVPixelFormat  pix_fmt;
    AVRational        pixel_aspect;
    char              *sws_param;

    AVBufferRef *hw_frames_ctx;

    /* audio only */
    int sample_rate;
    enum AVSampleFormat sample_fmt;
    int channels;
    uint64_t channel_layout;
    char    *channel_layout_str;

    int got_format_from_params;
    int eof;
 } BufferSourceContext;
AVFormatContext* _fmt_ctx_net = NULL;
AVFormatContext* _fmt_ctx_spk = NULL;
AVFormatContext* _fmt_ctx_mic = NULL;
AVFormatContext* _fmt_ctx_out = NULL;
int _index_net = -1;
int _index_spk = -1;
int _index_mic = -1;
int _index_a_out = -1;

AVFilterGraph* _filter_graph = NULL;
AVFilterContext* _filter_ctx_src_net = NULL;
AVFilterContext* _filter_ctx_src_spk = NULL;
AVFilterContext* _filter_ctx_src_mic = NULL;
AVFilterContext* _filter_ctx_src_vol = NULL;
AVFilterContext* _filter_ctx_sink = NULL;

CaptureState _state = PREPARED;

pthread_mutex_t mutex_net = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_spk = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mic = PTHREAD_MUTEX_INITIALIZER;
//CRITICAL_SECTION _section_spk;
//CRITICAL_SECTION _section_mic;
AVAudioFifo* _fifo_net = NULL;
AVAudioFifo* _fifo_spk = NULL;
AVAudioFifo* _fifo_mic = NULL;

void InitRecorder()
{
    av_register_all();
    avdevice_register_all();
    avfilter_register_all();
}

int OpenNetworkInput(char* inputForamt, const char* url)
{
    AVInputFormat* ifmt = av_find_input_format(inputForamt);
    AVDictionary* opt1 = NULL;
    av_dict_set(&opt1, "rtbufsize", "10M", 0);

    int ret = 0;
    ret = avformat_open_input(&_fmt_ctx_net, url, ifmt, &opt1);
    if (ret < 0)
    {
        printf("Speaker: failed to call avformat_open_input\n");
        return -1;
    }
    ret = avformat_find_stream_info(_fmt_ctx_net, NULL);
    if (ret < 0)
    {
        printf("Speaker: failed to call avformat_find_stream_info\n");
        return -1;
    }
    for (int i = 0; i < _fmt_ctx_net->nb_streams; i++)
    {
        if (_fmt_ctx_net->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            _index_net = i;
            break;
        }
    }
    if (_index_net < 0)
    {
        printf("Speaker: negative audio index\n");
        return -1;
    }
    AVCodecContext* codec_ctx = _fmt_ctx_net->streams[_index_net]->codec;
    AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL)
    {
        printf("Speaker: null audio decoder\n");
        return -1;
    }
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0)
    {
        printf("Speaker: failed to call avcodec_open2\n");
        return -1;
    }
    av_dump_format(_fmt_ctx_net, _index_net, url, 0);

    return 0;
}

int OpenSpeakerInput(char* inputForamt, const char* url)
{
    AVInputFormat* ifmt = av_find_input_format(inputForamt);
    AVDictionary* opt1 = NULL;
    av_dict_set(&opt1, "rtbufsize", "10M", 0);

    int ret = 0;
    ret = avformat_open_input(&_fmt_ctx_spk, url, ifmt, &opt1);
    if (ret < 0)
    {
        printf("Speaker: failed to call avformat_open_input\n");
        return -1;
    }
    ret = avformat_find_stream_info(_fmt_ctx_spk, NULL);
    if (ret < 0)
    {
        printf("Speaker: failed to call avformat_find_stream_info\n");
        return -1;
    }
    for (int i = 0; i < _fmt_ctx_spk->nb_streams; i++)
    {
        if (_fmt_ctx_spk->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            _index_spk = i;
            break;
        }
    }
    if (_index_spk < 0)
    {
        printf("Speaker: negative audio index\n");
        return -1;
    }
    AVCodecContext* codec_ctx = _fmt_ctx_spk->streams[_index_spk]->codec;
    AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL)
    {
        printf("Speaker: null audio decoder\n");
        return -1;
    }
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0)
    {
        printf("Speaker: failed to call avcodec_open2\n");
        return -1;
    }
    av_dump_format(_fmt_ctx_spk, _index_spk, url, 0);

    return 0;
}

int OpenMicrophoneInput(char* inputForamt, const char* url)
{
    AVInputFormat* ifmt = av_find_input_format(inputForamt);
    AVDictionary* opt1 = NULL;
    av_dict_set(&opt1, "rtbufsize", "10M", 0);

    int ret = 0;
    ret = avformat_open_input(&_fmt_ctx_mic, url, ifmt, &opt1);
    if (ret < 0)
    {
        printf("Microphone: failed to call avformat_open_input\n");
        return -1;
    }
    ret = avformat_find_stream_info(_fmt_ctx_mic, NULL);
    if (ret < 0)
    {
        printf("Microphone: failed to call avformat_find_stream_info\n");
        return -1;
    }
    for (int i = 0; i < _fmt_ctx_mic->nb_streams; i++)
    {
        if (_fmt_ctx_mic->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            _index_mic = i;
            break;
        }
    }
    if (_index_mic < 0)
    {
        printf("Microphone: negative audio index\n");
        return -1;
    }
    AVCodecContext* codec_ctx = _fmt_ctx_mic->streams[_index_mic]->codec;
    AVCodec* codec = avcodec_find_decoder(codec_ctx->codec_id);
    if (codec == NULL)
    {
        printf("Microphone: null audio decoder\n");
        return -1;
    }
    ret = avcodec_open2(codec_ctx, codec, NULL);
    if (ret < 0)
    {
        printf("Microphone: failed to call avcodec_open2\n");
        return -1;
    }

    av_dump_format(_fmt_ctx_mic, _index_mic, url, 0);

    return 0;
}

int OpenFileOutput(char* fileName)
{
    int ret = 0;
    ret = avformat_alloc_output_context2(&_fmt_ctx_out, NULL, NULL, fileName);
    if (ret < 0)
    {
        printf("Mixer: failed to call avformat_alloc_output_context2\n");
        return -1;
    }
    AVStream* stream_a = NULL;
    stream_a = avformat_new_stream(_fmt_ctx_out, NULL);
    if (stream_a == NULL)
    {
        printf("Mixer: failed to call avformat_new_stream\n");
        return -1;
    }
    _index_a_out = 0;

    stream_a->codec->codec_type = AVMEDIA_TYPE_AUDIO;
    AVCodec* codec_mp3 = NULL;
    codec_mp3 = avcodec_find_encoder(AV_CODEC_ID_MP3);
    if (codec_mp3 == NULL)
    {
        printf("Mixer: failed to call avcodec_find_encoder ++++\n");
        return -1;
    }
    stream_a->codec->codec = codec_mp3;
    stream_a->codec->sample_rate = 16000;
    stream_a->codec->channels = 2;
    stream_a->codec->channel_layout = av_get_default_channel_layout(2);
    stream_a->codec->sample_fmt = AV_SAMPLE_FMT_FLTP;
    stream_a->codec->bit_rate = 40000;
    stream_a->codec->time_base.num = 1;
    stream_a->codec->time_base.den = stream_a->codec->sample_rate;
    stream_a->codec->codec_tag = 0;


    if (_fmt_ctx_out->oformat->flags & AVFMT_GLOBALHEADER)
        stream_a->codec->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;

    if (avcodec_open2(stream_a->codec, stream_a->codec->codec, NULL) < 0)
    {
        printf("Mixer: failed to call avcodec_open2\n");
        return -1; 
    }
    if (!(_fmt_ctx_out->oformat->flags & AVFMT_NOFILE))
    {
        if (avio_open(&_fmt_ctx_out->pb, fileName, AVIO_FLAG_WRITE) < 0)
        {
            printf("Mixer: failed to call avio_open\n");
            return -1;
        }
    }

    if (avformat_write_header(_fmt_ctx_out, NULL) < 0)
    {
        printf("Mixer: failed to call avformat_write_header\n");
        return -1;
    }

    bool b = (!_fmt_ctx_out->streams[0]->time_base.num && _fmt_ctx_out->streams[0]->codec->time_base.num);

    av_dump_format(_fmt_ctx_out, _index_a_out, fileName, 1);

    _fifo_net = av_audio_fifo_alloc(_fmt_ctx_net->streams[_index_net]->codec->sample_fmt, _fmt_ctx_net->streams[_index_net]->codec->channels, 30*_fmt_ctx_net->streams[_index_net]->codec->frame_size);
    _fifo_spk = av_audio_fifo_alloc(_fmt_ctx_spk->streams[_index_spk]->codec->sample_fmt, _fmt_ctx_spk->streams[_index_spk]->codec->channels, 30*_fmt_ctx_spk->streams[_index_spk]->codec->frame_size);
    _fifo_mic = av_audio_fifo_alloc(_fmt_ctx_mic->streams[_index_mic]->codec->sample_fmt, _fmt_ctx_mic->streams[_index_mic]->codec->channels, 30*_fmt_ctx_mic->streams[_index_spk]->codec->frame_size);

    return 0;
}

int InitFilter(const char* filter_desc)
{
   
    char args_spk[512];
    const char* pad_name_spk = "in0";
    char args_mic[512];
    const char* pad_name_mic = "in1";
    char args_net[512];
    const char* pad_name_net = "in2"; 
    char args_vol[512] = "-3dB";
    const char* pad_name_vol = "vol";
    const AVFilter* filter_src_spk = NULL;
    filter_src_spk = avfilter_get_by_name("abuffer");
    if(filter_src_spk == NULL)
    {
        printf("fail to get avfilter ..\n");
        return -1;
    }
    const AVFilter* filter_src_mic = NULL;
    filter_src_mic = avfilter_get_by_name("abuffer");
    if(filter_src_mic == NULL)
    {
        printf("fail to get avfilter ..\n");
        return -1;
    }
    const AVFilter* filter_src_net = NULL;
    filter_src_net = avfilter_get_by_name("abuffer");
    if(filter_src_net == NULL)
    {
        printf("fail to get avfilter ..\n");
        return -1;
    }
    const AVFilter* filter_src_vol = NULL;
    filter_src_vol = avfilter_get_by_name("volume");
    if(filter_src_vol == NULL)
    {
        printf("fail to get avfilter ..\n");
        return -1;
    }
    const AVFilter* filter_sink = avfilter_get_by_name("abuffersink");
    AVFilterInOut* filter_output_spk = avfilter_inout_alloc();
    AVFilterInOut* filter_output_mic = avfilter_inout_alloc();
    AVFilterInOut* filter_output_net = avfilter_inout_alloc();
    AVFilterInOut* filter_output_vol = avfilter_inout_alloc();
    AVFilterInOut* filter_input = avfilter_inout_alloc();
    _filter_graph = avfilter_graph_alloc();

    //snprintf(args_spk, sizeof(args_spk), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x", 
    snprintf(args_spk, sizeof(args_spk), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
        _fmt_ctx_spk->streams[_index_spk]->codec->time_base.num, 
        _fmt_ctx_spk->streams[_index_spk]->codec->time_base.den, 
        _fmt_ctx_spk->streams[_index_spk]->codec->sample_rate, 
        av_get_sample_fmt_name(_fmt_ctx_spk->streams[_index_spk]->codec->sample_fmt), 
        _fmt_ctx_spk->streams[_index_spk]->codec->channel_layout);
        printf("args spk : %s channel_layout : %d\n",args_spk,_fmt_ctx_spk->streams[_index_spk]->codec->channel_layout);
    //snprintf(args_mic, sizeof(args_mic), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
    snprintf(args_mic, sizeof(args_mic), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
        _fmt_ctx_mic->streams[_index_mic]->codec->time_base.num, 
        _fmt_ctx_mic->streams[_index_mic]->codec->time_base.den, 
        _fmt_ctx_mic->streams[_index_mic]->codec->sample_rate, 
        av_get_sample_fmt_name(_fmt_ctx_mic->streams[_index_mic]->codec->sample_fmt), 
        _fmt_ctx_mic->streams[_index_mic]->codec->channel_layout);
        printf("args mic : %s channel_layout : %d\n",args_mic,_fmt_ctx_mic->streams[_index_mic]->codec->channel_layout);
    //snprintf(args_mic, sizeof(args_mic), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x",
    snprintf(args_net, sizeof(args_net), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%" PRIx64,
        _fmt_ctx_net->streams[_index_net]->codec->time_base.num, 
        _fmt_ctx_net->streams[_index_net]->codec->time_base.den, 
        _fmt_ctx_net->streams[_index_net]->codec->sample_rate, 
        av_get_sample_fmt_name(_fmt_ctx_net->streams[_index_net]->codec->sample_fmt), 
        _fmt_ctx_net->streams[_index_net]->codec->channel_layout);
        printf("args net : %s channel_layout : %d\n",args_net,_fmt_ctx_net->streams[_index_net]->codec->channel_layout);        
    //sprintf_s(args_spk, sizeof(args_spk), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x", _fmt_ctx_out->streams[_index_a_out]->codec->time_base.num, _fmt_ctx_out->streams[_index_a_out]->codec->time_base.den, _fmt_ctx_out->streams[_index_a_out]->codec->sample_rate, av_get_sample_fmt_name(_fmt_ctx_out->streams[_index_a_out]->codec->sample_fmt), _fmt_ctx_out->streams[_index_a_out]->codec->channel_layout);
    //sprintf_s(args_mic, sizeof(args_mic), "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%I64x", _fmt_ctx_out->streams[_index_a_out]->codec->time_base.num, _fmt_ctx_out->streams[_index_a_out]->codec->time_base.den, _fmt_ctx_out->streams[_index_a_out]->codec->sample_rate, av_get_sample_fmt_name(_fmt_ctx_out->streams[_index_a_out]->codec->sample_fmt), _fmt_ctx_out->streams[_index_a_out]->codec->channel_layout);


    int ret = 0;
    ret = avfilter_graph_create_filter(&_filter_ctx_src_spk, filter_src_spk, pad_name_spk, args_spk, NULL, _filter_graph);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_create_filter -- src spk\n");
        return -1;
    }
    ret = avfilter_graph_create_filter(&_filter_ctx_src_mic, filter_src_mic, pad_name_mic, args_mic, NULL, _filter_graph);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_create_filter -- src mic\n");
        return -1;
    }
    ret = avfilter_graph_create_filter(&_filter_ctx_src_net, filter_src_net, pad_name_net, args_net, NULL, _filter_graph);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_create_filter -- src net\n");
        return -1;
    }
    /*ret = avfilter_graph_create_filter(&_filter_ctx_src_vol, filter_src_vol, pad_name_vol, args_vol, NULL, _filter_graph);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_create_filter -- src net\n");
        return -1;
    }   */ 
    ret = avfilter_graph_create_filter(&_filter_ctx_sink, filter_sink, "out", NULL, NULL, _filter_graph);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_create_filter -- sink\n");
        return -1;
    }
    AVCodecContext* encodec_ctx = _fmt_ctx_out->streams[_index_a_out]->codec;
    ret = av_opt_set_bin(_filter_ctx_sink, "sample_fmts", (uint8_t*)&encodec_ctx->sample_fmt, sizeof(encodec_ctx->sample_fmt), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        printf("Filter: failed to call av_opt_set_bin -- sample_fmts\n");
        return -1;
    }
    ret = av_opt_set_bin(_filter_ctx_sink, "channel_layouts", (uint8_t*)&encodec_ctx->channel_layout, sizeof(encodec_ctx->channel_layout), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        printf("Filter: failed to call av_opt_set_bin -- channel_layouts\n");
        return -1;
    }
    ret = av_opt_set_bin(_filter_ctx_sink, "sample_rates", (uint8_t*)&encodec_ctx->sample_rate, sizeof(encodec_ctx->sample_rate), AV_OPT_SEARCH_CHILDREN);
    if (ret < 0)
    {
        printf("Filter: failed to call av_opt_set_bin -- sample_rates\n");
        return -1;
    }

    filter_output_spk->name = av_strdup(pad_name_spk);
    filter_output_spk->filter_ctx = _filter_ctx_src_spk;
    filter_output_spk->pad_idx = 0;
    filter_output_spk->next = filter_output_mic;

    filter_output_mic->name = av_strdup(pad_name_mic);
    filter_output_mic->filter_ctx = _filter_ctx_src_mic;
    filter_output_mic->pad_idx = 0;
    filter_output_mic->next = filter_output_net; //重點！！！！！

    filter_output_net->name = av_strdup(pad_name_net);
    filter_output_net->filter_ctx = _filter_ctx_src_net;
    filter_output_net->pad_idx = 0;
    filter_output_net->next = NULL;


    filter_output_vol->name = av_strdup(pad_name_vol);
    filter_output_vol->filter_ctx = _filter_ctx_src_vol;
    filter_output_vol->pad_idx = 0;
    filter_output_vol->next = filter_input;

    filter_input->name = av_strdup("out");
    filter_input->filter_ctx = _filter_ctx_sink;
    filter_input->pad_idx = 0;
    filter_input->next = NULL;



    AVFilterInOut* filter_outputs[3];
    filter_outputs[0] = filter_output_spk;
    filter_outputs[1] = filter_output_mic;
    filter_outputs[2] = filter_output_net;
    AVFilterInOut* filter_volinputs[2];
    filter_volinputs[0] = filter_output_vol;
    filter_volinputs[1] = filter_input;
    ret = avfilter_graph_parse_ptr(_filter_graph, filter_desc, &filter_input, filter_outputs, NULL);
    //ret = avfilter_graph_parse_ptr(_filter_graph, filter_desc, filter_volinputs, filter_outputs, NULL);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_parse_ptr\n");
        return -1;
    }

    ret = avfilter_graph_config(_filter_graph, NULL);
    if (ret < 0)
    {
        printf("Filter: failed to call avfilter_graph_config\n");
        return -1;
    }
    
    avfilter_inout_free(&filter_input);
    //av_free(filter_src_spk);
    //av_free(filter_src_mic);
    //av_free(filter_src_net);
    avfilter_inout_free(filter_outputs);
    //av_free(filter_outputs);
    char* temp = avfilter_graph_dump(_filter_graph, NULL);
    printf("%s\n", temp);

    return 0;
}

void * NetworkCapThreadProc(void *lpParam)
{
    AVFrame* pFrame = av_frame_alloc();
    AVPacket packet;
    av_init_packet(&packet);

    int got_sound;

    while (_state == RUNNING)
    {
        packet.data = NULL;
        packet.size = 0;

        if (av_read_frame(_fmt_ctx_net, &packet) < 0)
        {
            printf("there is no context spk \n");
            break;
        }
        if (packet.stream_index == _index_net)
        {
            if (avcodec_decode_audio4(_fmt_ctx_net->streams[_index_net]->codec, pFrame, &got_sound, &packet) < 0)
            {
                break;
            }
            av_free_packet(&packet);

            if (!got_sound)
            {
                continue;
            }

            int fifo_net_space = av_audio_fifo_space(_fifo_net);
            while(fifo_net_space < pFrame->nb_samples && _state == RUNNING)
            {
                usleep(10*1000);
                printf("_fifo_spk full !\n");
                fifo_net_space = av_audio_fifo_space(_fifo_net);
            }

            if (fifo_net_space >= pFrame->nb_samples)
            {
                //EnterCriticalSection(&_section_spk);
                pthread_mutex_lock(&mutex_net);
                int nWritten = av_audio_fifo_write(_fifo_net, (void**)pFrame->data, pFrame->nb_samples);
                //LeaveCriticalSection(&_section_spk);
                pthread_mutex_unlock(&mutex_net);
            }
        }
    }
    av_frame_free(&pFrame);

}

void * SpeakerCapThreadProc(void *lpParam)
{
    AVFrame* pFrame = av_frame_alloc();
    AVPacket packet;
    av_init_packet(&packet);

    int got_sound;

    while (_state == RUNNING)
    {
        packet.data = NULL;
        packet.size = 0;

        if (av_read_frame(_fmt_ctx_spk, &packet) < 0)
        {
            printf("there is no context spk \n");
            break;
        }
        if (packet.stream_index == _index_spk)
        {
            if (avcodec_decode_audio4(_fmt_ctx_spk->streams[_index_spk]->codec, pFrame, &got_sound, &packet) < 0)
            {
                break;
            }
            av_free_packet(&packet);

            if (!got_sound)
            {
                continue;
            }

            int fifo_spk_space = av_audio_fifo_space(_fifo_spk);
            while(fifo_spk_space < pFrame->nb_samples && _state == RUNNING)
            {
                usleep(10*1000);
                printf("_fifo_spk full !\n");
                fifo_spk_space = av_audio_fifo_space(_fifo_spk);
            }

            if (fifo_spk_space >= pFrame->nb_samples)
            {
                //EnterCriticalSection(&_section_spk);
                pthread_mutex_lock(&mutex_spk);
                int nWritten = av_audio_fifo_write(_fifo_spk, (void**)pFrame->data, pFrame->nb_samples);
                //LeaveCriticalSection(&_section_spk);
                pthread_mutex_unlock(&mutex_spk);
            }
        }
    }
    av_frame_free(&pFrame);

}

void * MicrophoneCapThreadProc(void *lpParam)
{
    AVFrame* pFrame = av_frame_alloc();
    AVPacket packet;
    av_init_packet(&packet);

    int got_sound;

    while (_state == PREPARED)
    {

    }

    while (_state == RUNNING)
    {
        packet.data = NULL;
        packet.size = 0;

        if (av_read_frame(_fmt_ctx_mic, &packet) < 0)
        {
            printf("there is no context mic \n");
            break;
        }
        if (packet.stream_index == _index_mic)
        {
            if (avcodec_decode_audio4(_fmt_ctx_mic->streams[_index_mic]->codec, pFrame, &got_sound, &packet) < 0)
            {
                break;
            }
            av_free_packet(&packet);

            if (!got_sound)
            {
                continue;
            }

            int fifo_mic_space = av_audio_fifo_space(_fifo_mic);
            while(fifo_mic_space < pFrame->nb_samples && _state == RUNNING)
            {
                usleep(100*1000);
                printf("_fifo_mic full !\n");
                fifo_mic_space = av_audio_fifo_space(_fifo_mic);
            }

            if (fifo_mic_space >= pFrame->nb_samples)
            {
                pthread_mutex_lock(&mutex_mic);
                //EnterCriticalSection(&_section_mic);
                int temp = av_audio_fifo_space(_fifo_mic);
                int temp2 = pFrame->nb_samples;
                int nWritten = av_audio_fifo_write(_fifo_mic, (void**)pFrame->data, pFrame->nb_samples);
                //LeaveCriticalSection(&_section_mic);
                pthread_mutex_unlock(&mutex_mic);
            }
        }
    }
    av_frame_free(&pFrame);

}

int kbhit (void)
{
  struct timeval tv;
  fd_set rdfs;
 
  tv.tv_sec = 0;
  tv.tv_usec = 0;
 
  FD_ZERO(&rdfs);
  FD_SET (STDIN_FILENO, &rdfs);
 
  select(STDIN_FILENO+1, &rdfs, NULL, NULL, &tv);
  return FD_ISSET(STDIN_FILENO, &rdfs);
}

int main()
{
    int ret = 0;

    InitRecorder();
    pthread_t spkPid;
    pthread_t micPid;
    pthread_t netPid;
    char *fileName = "/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/amix.mp3";
    const char* outFileType = ".mp3";
    int tmpFifoFailed = 0;
    int64_t frame_count = 0;
    time_t rawtime;
    tm* timeInfo;
    const char* filter_desc = "[in0][in1][in2]amix=inputs=3[out]";
    time(&rawtime);
    timeInfo = localtime(&rawtime);
//    snprintf(fileName, sizeof(fileName), "%d_%d_%d_%d_%d_%d%s",
//        timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
//        timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec, outFileType);

    printf("%s ...\n",fileName);
    //ret = OpenSpeakerInput("dshow", "audio=virtual-audio-capturer");
    ret = OpenSpeakerInput(NULL, "http://clipres.yishihui.com/longvideo/material/pic/prod/20200919/MUSIC_MIGU_60054704083");
    if (ret < 0)
    {
        goto Release;
    }
    //ret = OpenMicrophoneInput("dshow", "audio=External Microphone (Conexant S");
    ret = OpenMicrophoneInput(NULL, "http://clipres.yishihui.com/longvideo/material/pic/prod/20200919/MUSIC_MIGU_64049302101");
    if (ret < 0)
    {
        goto Release;
    }
    ret = OpenNetworkInput(NULL,"http://clipres.yishihui.com/longvideo/material/pic/prod/20200919/MUSIC_MIGU_6005751G5AU");
    if (ret < 0)
    {
        goto Release;
    }    
    ret = OpenFileOutput(fileName);
    if (ret < 0)
    {
        goto Release;
    }
    ret = InitFilter(filter_desc);
    if (ret < 0)
    {
        goto Release;
    }
    _state = RUNNING;

 //   CreateThread(NULL, 0, SpeakerCapThreadProc, 0, 0, NULL);
 //   CreateThread(NULL, 0, MicrophoneCapThreadProc, 0, 0, NULL);
    if (pthread_create(&spkPid, NULL, SpeakerCapThreadProc, NULL) == -1)
    {
        printf("create error!\n");
        return 1;
    }
    if (pthread_create(&micPid, NULL, MicrophoneCapThreadProc, NULL) == -1)
    {
        printf("create error!\n");
        return 1;
    }
    if (pthread_create(&netPid, NULL, NetworkCapThreadProc, NULL) == -1)
    {
        printf("create error!\n");
        return 1;
    }    
    while (_state != FINISHED)
    {
        if (kbhit())
        {
            _state = STOPPED;
            break;
        }
        else
        {
            int ret = 0;
            AVFrame* pFrame_spk = av_frame_alloc();
            AVFrame* pFrame_mic = av_frame_alloc();
            AVFrame* pFrame_net = av_frame_alloc();

            AVPacket packet_out;

            int got_packet_ptr = 0;

            int fifo_spk_size = av_audio_fifo_size(_fifo_spk);
            int fifo_mic_size = av_audio_fifo_size(_fifo_mic);
            int fifo_net_size = av_audio_fifo_size(_fifo_net);
            int frame_spk_min_size = _fmt_ctx_spk->streams[_index_spk]->codec->frame_size;
            int frame_mic_min_size = _fmt_ctx_mic->streams[_index_mic]->codec->frame_size;
            int frame_net_min_size = _fmt_ctx_net->streams[_index_net]->codec->frame_size;
	    printf("get mini frame size : %d ###################\n",frame_spk_min_size);
            if (fifo_spk_size >= frame_spk_min_size && fifo_mic_size >= frame_mic_min_size && fifo_net_size >= frame_net_min_size)
            {
                tmpFifoFailed = 0;

                pFrame_spk->nb_samples = frame_spk_min_size;
                pFrame_spk->channel_layout = _fmt_ctx_spk->streams[_index_spk]->codec->channel_layout;
                pFrame_spk->format = _fmt_ctx_spk->streams[_index_spk]->codec->sample_fmt;
                pFrame_spk->sample_rate = _fmt_ctx_spk->streams[_index_spk]->codec->sample_rate;
                av_frame_get_buffer(pFrame_spk, 0);

                pFrame_mic->nb_samples = frame_mic_min_size;
                pFrame_mic->channel_layout = _fmt_ctx_mic->streams[_index_mic]->codec->channel_layout;
                pFrame_mic->format = _fmt_ctx_mic->streams[_index_mic]->codec->sample_fmt;
                pFrame_mic->sample_rate = _fmt_ctx_mic->streams[_index_mic]->codec->sample_rate;
                av_frame_get_buffer(pFrame_mic, 0);

                pFrame_net->nb_samples = frame_net_min_size;
                pFrame_net->channel_layout = _fmt_ctx_net->streams[_index_net]->codec->channel_layout;
                pFrame_net->format = _fmt_ctx_net->streams[_index_net]->codec->sample_fmt;
                pFrame_net->sample_rate = _fmt_ctx_net->streams[_index_net]->codec->sample_rate;
                av_frame_get_buffer(pFrame_net, 0);

                pthread_mutex_lock(&mutex_spk);
                //EnterCriticalSection(&_section_spk);
                ret = av_audio_fifo_read(_fifo_spk, (void**)pFrame_spk->data, frame_spk_min_size);
                //LeaveCriticalSection(&_section_spk);
                pthread_mutex_unlock(&mutex_spk);

                pthread_mutex_lock(&mutex_mic);
                //EnterCriticalSection(&_section_mic);
                ret = av_audio_fifo_read(_fifo_mic, (void**)pFrame_mic->data, frame_mic_min_size);
                //LeaveCriticalSection(&_section_mic);
                pthread_mutex_unlock(&mutex_mic);

                pthread_mutex_lock(&mutex_net);
                //EnterCriticalSection(&_section_net);
                ret = av_audio_fifo_read(_fifo_net, (void**)pFrame_net->data, frame_net_min_size);
                //LeaveCriticalSection(&_section_net);
                pthread_mutex_unlock(&mutex_net);

                pFrame_spk->pts = av_frame_get_best_effort_timestamp(pFrame_spk);
                pFrame_mic->pts = av_frame_get_best_effort_timestamp(pFrame_mic);
                pFrame_net->pts = av_frame_get_best_effort_timestamp(pFrame_net);

                BufferSourceContext* s = (BufferSourceContext*)_filter_ctx_src_spk->priv;
                bool b1 = (s->sample_fmt != pFrame_spk->format);
                bool b2 = (s->sample_rate != pFrame_spk->sample_rate);
                bool b3 = (s->channel_layout != pFrame_spk->channel_layout);
                bool b4 = (s->channels != pFrame_spk->channels);

                ret = av_buffersrc_add_frame(_filter_ctx_src_spk, pFrame_spk);
                if (ret < 0)
                {
                    printf("Mixer: failed to call av_buffersrc_add_frame (speaker)\n");
                    break;
                }

                ret = av_buffersrc_add_frame(_filter_ctx_src_mic, pFrame_mic);
                if (ret < 0)
                {
                    printf("Mixer: failed to call av_buffersrc_add_frame (microphone)\n");
                    break;
                }
                ret = av_buffersrc_add_frame(_filter_ctx_src_net, pFrame_net);
                if (ret < 0)
                {
                    printf("Mixer: failed to call av_buffersrc_add_frame (network)\n");
                    break;
                }
                while (1)
                {
                    AVFrame* pFrame_out = av_frame_alloc();
                    //AVERROR(EAGAIN) 返回这个表示还没转换完成既 不存在帧，则返回AVERROR（EAGAIN）
                    ret = av_buffersink_get_frame_flags(_filter_ctx_sink, pFrame_out, 0 );
                    if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    {
                        //printf("%d %d \n",AVERROR(EAGAIN),AVERROR_EOF);
                    }                  
                    if (ret < 0)
                    {
                        printf("Mixer: failed to call av_buffersink_get_frame_flags ret : %d \n",ret);
                        break;
                    }

                    if (pFrame_out->data[0] != NULL)
                    {
                        av_init_packet(&packet_out);
                        packet_out.data = NULL;
                        packet_out.size = 0;

                        ret = avcodec_encode_audio2(_fmt_ctx_out->streams[_index_a_out]->codec, &packet_out, pFrame_out, &got_packet_ptr);
                        if (ret < 0)
                        {
                            printf("Mixer: failed to call avcodec_decode_audio4\n");
                            break;
                        }
                        if (got_packet_ptr)
                        {
                            packet_out.stream_index = _index_a_out;
                            packet_out.pts = frame_count * _fmt_ctx_out->streams[_index_a_out]->codec->frame_size;
                            packet_out.dts = packet_out.pts;
                            packet_out.duration = _fmt_ctx_out->streams[_index_a_out]->codec->frame_size;

                            packet_out.pts = av_rescale_q_rnd(packet_out.pts, 
                                _fmt_ctx_out->streams[_index_a_out]->codec->time_base,
                                _fmt_ctx_out->streams[_index_a_out]->time_base,
                                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
                            packet_out.dts = packet_out.pts;
                            packet_out.duration = av_rescale_q_rnd(packet_out.duration,
                                _fmt_ctx_out->streams[_index_a_out]->codec->time_base,
                                _fmt_ctx_out->streams[_index_a_out]->time_base,
                                (AVRounding)(AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));

                            frame_count++;

                            ret = av_interleaved_write_frame(_fmt_ctx_out, &packet_out);
                            if (ret < 0)
                            {
                                printf("Mixer: failed to call av_interleaved_write_frame\n");
                            }
                            //printf("Mixer: write frame to file\n");
                        }
                        av_free_packet(&packet_out);                    
                    }
                    av_frame_free(&pFrame_out);
                }
            }
            else
            {
                tmpFifoFailed++;
                usleep(20*1000);
                if (tmpFifoFailed > 300)
                {
                    _state = STOPPED;
                    usleep(30*1000);
                    break;
                }
            }
            av_frame_free(&pFrame_spk);
            av_frame_free(&pFrame_mic);
        }
    }

    av_write_trailer(_fmt_ctx_out);

Release:
    av_audio_fifo_free(_fifo_spk);
    av_audio_fifo_free(_fifo_mic);
    av_audio_fifo_free(_fifo_net);

    avfilter_free(_filter_ctx_src_spk);
    avfilter_free(_filter_ctx_src_mic);
    avfilter_free(_filter_ctx_src_net);
    avfilter_free(_filter_ctx_sink);

    avfilter_graph_free(&_filter_graph);


    if (_fmt_ctx_out)
    {
        avio_close(_fmt_ctx_out->pb);
    }

    avformat_close_input(&_fmt_ctx_spk);
    avformat_close_input(&_fmt_ctx_mic);
    avformat_close_input(&_fmt_ctx_net);
    avformat_free_context(_fmt_ctx_out);

    return ret;
}
