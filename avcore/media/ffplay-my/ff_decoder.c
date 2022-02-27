//
// Created by 阳坤 on 2022/2/20.
//

#include "ff_decoder.h"


/**
 * @brief 这里是设置给ffmpeg内部，当ffmpeg内部当执行耗时操作时（一般是在执行while或者for循环的数据读取时）
 *          就会调用该函数
 * @param ctx
 * @return 若直接退出阻塞则返回1，等待读取则返回0
 */

static int decode_interrupt_cb(void *ctx) {
    static int64_t s_pre_time = 0;
    int64_t cur_time = av_gettime_relative() / 1000;
    //    printf("decode_interrupt_cb interval:%lldms\n", cur_time - s_pre_time);
    s_pre_time = cur_time;
    VideoState *is = (VideoState *) ctx;
    return is->abort_request;
}

static int audio_decode_thread(void *arg) {
    VideoState *is = arg;

    //todo ...

    return 0;
}

static int video_decode_thread(void *arg) {
    VideoState *is = arg;

    //todo ...

    return 0;
}

static int subtitle_decode_thread(void *arg) {
    VideoState *is = arg;

    //todo ...

    return 0;
}

int ff_decoder_init(struct VideoState *is) {
    int ret;
    if (!is)
        goto fail;
    /* 2.设置中断回调函数，如果出错或者退出，就根据目前程序设置的状态选择继续check或者直接退出 */
    /* 当执行耗时操作时（一般是在执行while或者for循环的数据读取时），会调用interrupt_callback.callback
     * 回调函数中返回1则代表ffmpeg结束耗时操作退出当前函数的调用
     * 回调函数中返回0则代表ffmpeg内部继续执行耗时操作，直到完成既定的任务(比如读取到既定的数据包)
     */
    is->ic->interrupt_callback.callback = decode_interrupt_cb;
    /* open the streams */
    /* 8. 打开视频、音频解码器。在此会打开相应解码器，并创建相应的解码线程。 */
    if (is->st_index[AVMEDIA_TYPE_AUDIO] >= 0) {// 如果有音频流则打开音频流
        ff_stream_component_open(is, is->st_index[AVMEDIA_TYPE_AUDIO]);
    }

    ret = -1;
    if (is->st_index[AVMEDIA_TYPE_VIDEO] >= 0) { // 如果有视频流则打开视频流
        ret = ff_stream_component_open(is, is->st_index[AVMEDIA_TYPE_VIDEO]);
    }
    if (is->show_mode == SHOW_MODE_NONE) {
        //选择怎么显示，如果视频打开成功，就显示视频画面，否则，显示音频对应的频谱图
        is->show_mode = ret >= 0 ? SHOW_MODE_VIDEO : SHOW_MODE_RDFT;
    }

    if (is->st_index[AVMEDIA_TYPE_SUBTITLE] >= 0) { // 如果有字幕流则打开字幕流
        ff_stream_component_open(is, is->st_index[AVMEDIA_TYPE_SUBTITLE]);
    }

    if (is->video_stream < 0 && is->audio_stream < 0) {
        av_log(NULL, AV_LOG_FATAL, "Failed to open file '%s' or configure filtergraph\n",
               is->filename);
        ret = -1;
        goto fail;
    }

    if (infinite_buffer < 0 && is->realtime)
        infinite_buffer = 1;    // 如果是实时流
    fail:

    ret = -1;
    return 0;
}

int ff_stream_component_open(struct VideoState *is, int stream_index) {
    AVFormatContext *ic = is->ic;
    AVCodecContext *avctx;
    AVCodec *codec;
    const char *forced_codec_name = NULL;
    AVDictionary *opts = NULL;
    AVDictionaryEntry *t = NULL;
    int sample_rate, nb_channels;
    int64_t channel_layout;
    int ret = 0;
    int stream_lowres = lowres;
    if (stream_index < 0 || stream_index >= ic->nb_streams)
        return -1;
    /*  为解码器分配一个编解码器上下文结构体 */
    avctx = avcodec_alloc_context3(NULL);
    if (!avctx) {
        ret = AVERROR(ENOMEM);
        goto fail;
    }
    /* 将码流中的编解码器信息拷贝到新分配的编解码器上下文结构体 */
    ret = avcodec_parameters_to_context(avctx, ic->streams[stream_index]->codecpar);
    if (ret < 0)
        goto fail;
    // 设置pkt_timebase
    avctx->pkt_timebase = ic->streams[stream_index]->time_base;

    /* 根据codec_id查找解码器 */
    codec = avcodec_find_decoder(avctx->codec_id);

    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO   :
            is->last_audio_stream = stream_index;
            forced_codec_name = audio_codec_name;
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            is->last_subtitle_stream = stream_index;
            forced_codec_name = subtitle_codec_name;
            break;
        case AVMEDIA_TYPE_VIDEO   :
            is->last_video_stream = stream_index;
            forced_codec_name = video_codec_name;
            break;
    }

//    通过编码器名称来找
    if (forced_codec_name)
        codec = avcodec_find_decoder_by_name(forced_codec_name);
    if (!codec) {
        if (forced_codec_name)
            av_log(NULL, AV_LOG_WARNING,
                   "No codec could be found with name '%s'\n", forced_codec_name);
        else
            av_log(NULL, AV_LOG_WARNING,
                   "No decoder could be found for codec %s\n", avcodec_get_name(avctx->codec_id));
        ret = AVERROR(EINVAL);
        goto fail;
    }
    avctx->codec_id = codec->id;
    if (stream_lowres > codec->max_lowres) {
        av_log(avctx, AV_LOG_WARNING, "The maximum value for lowres supported by the decoder is %d\n",
               codec->max_lowres);
        stream_lowres = codec->max_lowres;
    }
    avctx->lowres = stream_lowres;

    if (fast)
        avctx->flags2 |= AV_CODEC_FLAG2_FAST;

    opts = filter_codec_opts(codec_opts, avctx->codec_id, ic, ic->streams[stream_index], codec);

//    配置解码线程
    if (!av_dict_get(opts, "threads", NULL, 0))
        av_dict_set(&opts, "threads", "auto", 0);
    if (stream_lowres)
        av_dict_set_int(&opts, "lowres", stream_lowres, 0);
    if (avctx->codec_type == AVMEDIA_TYPE_VIDEO || avctx->codec_type == AVMEDIA_TYPE_AUDIO)
        av_dict_set(&opts, "refcounted_frames", "1", 0);

    //打开编码器
    ret = avcodec_open2(avctx, codec, &opts);
    if (ret < 0)
        goto fail;
    if ((t = av_dict_get(opts, "", NULL, AV_DICT_IGNORE_SUFFIX))) {
        av_log(NULL, AV_LOG_ERROR, "Option %s not found.\n", t->key);
        ret = AVERROR_OPTION_NOT_FOUND;
        goto fail;
    }
    is->eof = 0;
    ic->streams[stream_index]->discard = AVDISCARD_DEFAULT;

    switch (avctx->codec_type) {
        case AVMEDIA_TYPE_AUDIO:
#if CONFIG_AVFILTER
            {
                AVFilterContext *sink;

                is->audio_filter_src.freq           = avctx->sample_rate;
                is->audio_filter_src.channels       = avctx->channels;
                is->audio_filter_src.channel_layout = get_valid_channel_layout(avctx->channel_layout, avctx->channels);
                is->audio_filter_src.fmt            = avctx->sample_fmt;
                if ((ret = configure_audio_filters(is, afilters, 0)) < 0)
                    goto fail;
                sink = is->out_audio_filter;
                sample_rate    = av_buffersink_get_sample_rate(sink);
                nb_channels    = av_buffersink_get_channels(sink);
                channel_layout = av_buffersink_get_channel_layout(sink);
            }
#else
            //从avctx(即AVCodecContext)中获取音频格式参数
            sample_rate = avctx->sample_rate;
            nb_channels = avctx->channels;
            channel_layout = avctx->channel_layout;
#endif
            /* prepare audio output 准备音频输出*/
            //调用audio_open打开sdl音频输出，实际打开的设备参数保存在audio_tgt，返回值表示输出设备的缓冲区大小
//            if ((ret = audio_open(is, channel_layout, nb_channels, sample_rate, &is->audio_tgt)) < 0)
//                goto fail;
            is->audio_hw_buf_size = ret;
            is->audio_src = is->audio_tgt;  //暂且将数据源参数等同于目标输出参数
            //初始化audio_buf相关参数
            is->audio_buf_size = 0;
            is->audio_buf_index = 0;

            /* init averaging filter 初始化averaging滤镜, 非audio master时使用 */
            is->audio_diff_avg_coef = exp(log(0.01) / AUDIO_DIFF_AVG_NB); //0.794  exp，高等数学里以自然常数e为底的指数函数
            is->audio_diff_avg_count = 0;
            /* 由于我们没有精确的音频数据填充FIFO,故只有在大于该阈值时才进行校正音频同步*/
            is->audio_diff_threshold = (double) (is->audio_hw_buf_size) / is->audio_tgt.bytes_per_sec;

            is->audio_stream = stream_index;    // 获取audio的stream索引
            is->audio_st = ic->streams[stream_index];  // 获取audio的stream指针
            // 初始化ffplay封装的音频解码器
            ff_decoder_parameters_init(&is->auddec, avctx, &is->audioq, is->continue_read_thread);
            if ((is->ic->iformat->flags & (AVFMT_NOBINSEARCH | AVFMT_NOGENSEARCH | AVFMT_NO_BYTE_SEEK)) &&
                !is->ic->iformat->read_seek) {
                is->auddec.start_pts = is->audio_st->start_time;
                is->auddec.start_pts_tb = is->audio_st->time_base;
            }
            // 启动音频解码线程
            if ((ret = ff_decoder_start(&is->auddec, audio_decode_thread, "audio_decoder", is)) < 0)
                goto out;
            break;
        case AVMEDIA_TYPE_VIDEO:
            is->video_stream = stream_index;    // 获取video的stream索引
            is->video_st = ic->streams[stream_index];// 获取video的stream指针
            // 初始化ffplay封装的视频解码器
            ff_decoder_parameters_init(&is->viddec, avctx, &is->videoq, is->continue_read_thread);
            // 启动视频频解码线程
            if ((ret = ff_decoder_start(&is->viddec, video_decode_thread, "video_decoder", is)) < 0)
                goto out;
            is->queue_attachments_req = 1; // 使能请求mp3、aac等音频文件的封面
            break;
        case AVMEDIA_TYPE_SUBTITLE:
            is->subtitle_stream = stream_index;
            is->subtitle_st = ic->streams[stream_index];

            ff_decoder_parameters_init(&is->subdec, avctx, &is->subtitleq, is->continue_read_thread);
            if ((ret = ff_decoder_start(&is->subdec, subtitle_decode_thread, "subtitle_decoder", is)) < 0)
                goto out;
            break;
        default:
            break;
    }

    fail:
    avcodec_free_context(&avctx);
    out:
    av_dict_free(&opts);
    return ret;
}

void ff_decoder_parameters_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond) {
    memset(d, 0, sizeof(Decoder));
    d->avctx = avctx;   // 解码器上下文
    d->queue = queue;   // 绑定对应的packet queue
    d->empty_queue_cond = empty_queue_cond; // 绑定read_thread线程的continue_read_thread
    d->start_pts = AV_NOPTS_VALUE;      // 起始设置为无效
    d->pkt_serial = -1;                 // 起始设置为-1
}

int ff_decoder_start(Decoder *d, int (*fn)(void *), const char *thread_name, void *arg) {
    ff_packet_queue_start(d->queue);   // 启用对应的packet 队列
    d->decoder_tid = SDL_CreateThread(fn, thread_name, arg);    // 创建解码线程
    if (!d->decoder_tid) {
        av_log(NULL, AV_LOG_ERROR, "SDL_CreateThread(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    return 0;
}
