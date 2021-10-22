//
// Created by 阳坤 on 2021/10/22.
//
/*
 * Copyright (c) 2010 Nicolas George
 * Copyright (c) 2011 Stefano Sabatini
 * Copyright (c) 2012 Cl??ment B?“sch
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

/**
 * @file
 * API example for audio decoding and filtering
 * @example filtering_audio.c
 */

#include <unistd.h>

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavutil/opt.h>

#define ENABLE_FILTERS 1

static const char *filter_descr = "[in0][in1]amix=inputs=2[out]";//"aresample=8000,aformat=sample_fmts=s16:channel_layouts=mono";
static const char *player = "ffplay -f s16le -ar 8000 -ac 1 -";

static AVFormatContext *fmt_ctx1;
static AVFormatContext *fmt_ctx2;

static AVCodecContext *dec_ctx1;
static AVCodecContext *dec_ctx2;

AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx1;
AVFilterContext *buffersrc_ctx2;

AVFilterGraph *filter_graph;
static int audio_stream_index_1 = -1;
static int audio_stream_index_2 = -1;

FILE *fwirte = NULL;

static int open_input_file_1(const char *filename) {
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx1, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx1, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the audio stream */
    ret = av_find_best_stream(fmt_ctx1, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream in the input file\n");
        return ret;
    }
    audio_stream_index_1 = ret;
    dec_ctx1 = fmt_ctx1->streams[audio_stream_index_1]->codec;
    av_opt_set_int(dec_ctx1, "refcounted_frames", 1, 0);

    /* init the audio decoder */
    if ((ret = avcodec_open2(dec_ctx1, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
        return ret;
    }

    return 0;
}

static int open_input_file_2(const char *filename) {
    int ret;
    AVCodec *dec;

    if ((ret = avformat_open_input(&fmt_ctx2, filename, NULL, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
        return ret;
    }

    if ((ret = avformat_find_stream_info(fmt_ctx2, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
        return ret;
    }

    /* select the audio stream */
    ret = av_find_best_stream(fmt_ctx2, AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot find an audio stream in the input file\n");
        return ret;
    }
    audio_stream_index_2 = ret;
    dec_ctx2 = fmt_ctx2->streams[audio_stream_index_2]->codec;
    av_opt_set_int(dec_ctx2, "refcounted_frames", 1, 0);

    /* init the audio decoder */
    if ((ret = avcodec_open2(dec_ctx2, dec, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot open audio decoder\n");
        return ret;
    }

    return 0;
}

static int init_filters(const char *filters_descr) {
    char args1[512];
    char args2[512];
    int ret = 0;
    AVFilter *abuffersrc1 = avfilter_get_by_name("abuffer");
    AVFilter *abuffersrc2 = avfilter_get_by_name("abuffer");
    AVFilter *abuffersink = avfilter_get_by_name("abuffersink");

    AVFilterInOut *outputs1 = avfilter_inout_alloc();
    AVFilterInOut *outputs2 = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();

    static const enum AVSampleFormat out_sample_fmts[] = {AV_SAMPLE_FMT_S16, -1};
    static const int64_t out_channel_layouts[] = {AV_CH_LAYOUT_MONO, -1};
    static const int out_sample_rates[] = {8000, -1};
    const AVFilterLink *outlink;

    AVRational time_base_1 = fmt_ctx1->streams[audio_stream_index_1]->time_base;
    AVRational time_base_2 = fmt_ctx2->streams[audio_stream_index_2]->time_base;

    filter_graph = avfilter_graph_alloc();
    if (!outputs1 || !inputs || !filter_graph) {
        ret = AVERROR(ENOMEM);
        goto end;
    }

    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    if (!dec_ctx1->channel_layout)
        dec_ctx1->channel_layout = av_get_default_channel_layout(dec_ctx1->channels);
    snprintf(args1, sizeof(args1),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
             time_base_1.num, time_base_1.den, dec_ctx1->sample_rate,
             av_get_sample_fmt_name(dec_ctx1->sample_fmt), dec_ctx1->channel_layout);
    ret = avfilter_graph_create_filter(&buffersrc_ctx1, abuffersrc1, "in1",
                                       args1, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        goto end;
    }

#if (ENABLE_FILTERS)
    /* buffer audio source: the decoded frames from the decoder will be inserted here. */
    if (!dec_ctx2->channel_layout)
        dec_ctx2->channel_layout = av_get_default_channel_layout(dec_ctx2->channels);
    snprintf(args2, sizeof(args2),
             "time_base=%d/%d:sample_rate=%d:sample_fmt=%s:channel_layout=0x%"PRIx64,
             time_base_2.num, time_base_2.den, dec_ctx2->sample_rate,
             av_get_sample_fmt_name(dec_ctx2->sample_fmt), dec_ctx2->channel_layout);
    ret = avfilter_graph_create_filter(&buffersrc_ctx2, abuffersrc1, "in2",
                                       args2, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer source\n");
        goto end;
    }
#endif
    /* buffer audio sink: to terminate the filter chain. */
    ret = avfilter_graph_create_filter(&buffersink_ctx, abuffersink, "out",
                                       NULL, NULL, filter_graph);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot create audio buffer sink\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "sample_fmts", out_sample_fmts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample format\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "channel_layouts", out_channel_layouts, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output channel layout\n");
        goto end;
    }

    ret = av_opt_set_int_list(buffersink_ctx, "sample_rates", out_sample_rates, -1,
                              AV_OPT_SEARCH_CHILDREN);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Cannot set output sample rate\n");
        goto end;
    }

    /*
     * Set the endpoints for the filter graph. The filter_graph will
     * be linked to the graph described by filters_descr.
     */

    /*
     * The buffer source output must be connected to the input pad of
     * the first filter described by filters_descr; since the first
     * filter input label is not specified, it is set to "in" by
     * default.
     */
    outputs1->name = av_strdup("in0");
    outputs1->filter_ctx = buffersrc_ctx1;
    outputs1->pad_idx = 0;
#if (ENABLE_FILTERS)
    outputs1->next = outputs2;

    outputs2->name = av_strdup("in1");
    outputs2->filter_ctx = buffersrc_ctx2;
    outputs2->pad_idx = 0;
    outputs2->next = NULL;
#else
    outputs1->next       = NULL;
#endif
    /*
     * The buffer sink input must be connected to the output pad of
     * the last filter described by filters_descr; since the last
     * filter output label is not specified, it is set to "out" by
     * default.
     */
    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    AVFilterInOut *filter_outputs[2];
    filter_outputs[0] = outputs1;
#if (ENABLE_FILTERS)
    filter_outputs[1] = outputs2;
#endif

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs1, NULL)) < 0)//filter_outputs
    {
        av_log(NULL, AV_LOG_ERROR, "parse ptr fail, ret: %d\n", ret);
        goto end;
    }

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "config graph fail, ret: %d\n", ret);
        goto end;
    }

    /* Print summary of the sink buffer
     * Note: args buffer is reused to store channel layout string */
    outlink = buffersink_ctx->inputs[0];
    av_get_channel_layout_string(args1, sizeof(args1), -1, outlink->channel_layout);
    av_log(NULL, AV_LOG_INFO, "Output: srate:%dHz fmt:%s chlayout:%s\n",
           (int) outlink->sample_rate,
           (char *) av_x_if_null(av_get_sample_fmt_name(outlink->format), "?"),
           args1);

    end:
    avfilter_inout_free(&inputs);
    avfilter_inout_free(&outputs1);

    return ret;
}

static void print_frame(const AVFrame *frame)
#if 1
{
//    FILE *file = NULL;
    const int n = frame->nb_samples * av_get_channel_layout_nb_channels(av_frame_get_channel_layout(frame));
    const uint16_t *p = (uint16_t *) frame->data[0];
    const uint16_t *p_end = p + n;

//    file = fopen("tmp.pcm", "ab+");
    if (NULL == fwirte) {
        perror("fopen tmp.mp3 error\n");
        return;
    } else {
        perror("fopen tmp.aac successful\n");
    }
    fwrite(frame->data[0], n * 2, 1, fwirte);
//    fclose(file);
//    file = NULL;
}

#else
{
    const int n = frame->nb_samples * av_get_channel_layout_nb_channels(av_frame_get_channel_layout(frame));
    const uint16_t *p = (uint16_t *) frame->data[0];
    const uint16_t *p_end = p + n;

//    fwrite(p,n,1,fwirte);

    while (p < p_end) {
        fputc(*p & 0xff, stdout);
        fputc(*p >> 8 & 0xff, stdout);
        p++;
    }
    fflush(stdout);
}

#endif

int main(int argc, char **argv) {
    fwirte = fopen("/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/amix.pcm", "wb");
    int ret;
    AVFrame *frame = av_frame_alloc();
    AVFrame *filt_frame = av_frame_alloc();
    int got_frame;


    AVInputFormat *nput = av_find_input_format("fltp");

    if (!frame || !filt_frame) {
        perror("Could not allocate frame");
        exit(1);
    }
    /*
    if (argc != 2) {
        fprintf(stderr, "Usage: %s file | %s\n", argv[0], player);
        exit(1);
    }
    */

    av_register_all();
    avfilter_register_all();

    if ((ret = open_input_file_1(argv[1])) < 0) {
        av_log(NULL, AV_LOG_ERROR, "open input file fail, ret: %d\n", ret);
        goto end;
    }
    if ((ret = open_input_file_2(argv[2])) < 0) {
        av_log(NULL, AV_LOG_ERROR, "open input file fail, ret: %d\n", ret);
        goto end;
    }
    if ((ret = init_filters(filter_descr)) < 0) {
        av_log(NULL, AV_LOG_ERROR, "init filters fail, ret: %d\n", ret);
        goto end;
    }

    AVPacket packet0, packet;
    AVPacket _packet0, _packet;

    /* read all packets */
    packet0.data = NULL;
    packet.data = NULL;

    _packet0.data = NULL;
    _packet.data = NULL;
    while (1) {
        if (!packet0.data) {
            if ((ret = av_read_frame(fmt_ctx1, &packet)) < 0)
                break;
            packet0 = packet;
        }

        if (packet.stream_index == audio_stream_index_1) {
            got_frame = 0;
            ret = avcodec_decode_audio4(dec_ctx1, frame, &got_frame, &packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding audio\n");
                continue;
            }
            packet.size -= ret;
            packet.data += ret;

            if (got_frame) {
                av_log(NULL, AV_LOG_ERROR, "push frame\n");
                /* push the audio data from decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx1, frame, 0) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
                    break;
                }
                av_log(NULL, AV_LOG_ERROR, "pull frame\n");
            }

            if (packet.size <= 0)
                av_packet_unref(&packet0);
        } else {
            /* discard non-wanted packets */
            av_packet_unref(&packet0);
        }

        if (!_packet0.data) {
            if ((ret = av_read_frame(fmt_ctx2, &_packet)) < 0)
                break;
            _packet0 = _packet;
        }

        if (_packet.stream_index == audio_stream_index_2) {
            got_frame = 0;
            ret = avcodec_decode_audio4(dec_ctx2, frame, &got_frame, &_packet);
            if (ret < 0) {
                av_log(NULL, AV_LOG_ERROR, "Error decoding audio\n");
                continue;
            }
            _packet.size -= ret;
            _packet.data += ret;

            if (got_frame) {
                av_log(NULL, AV_LOG_ERROR, "push frame\n");
                /* push the audio data from decoded frame into the filtergraph */
                if (av_buffersrc_add_frame_flags(buffersrc_ctx2, frame, 0) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Error while feeding the audio filtergraph\n");
                    break;
                }
                av_log(NULL, AV_LOG_ERROR, "pull frame\n");
            }

            if (_packet.size <= 0)
                av_packet_unref(&_packet0);
        } else {
            /* discard non-wanted packets */
            av_packet_unref(&_packet0);
        }
        /* pull filtered audio from the filtergraph */
        if (got_frame) {
            while (1) {
                ret = av_buffersink_get_frame(buffersink_ctx, filt_frame);
                if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
                    break;
                if (ret < 0) {
                    av_log(NULL, AV_LOG_ERROR, "buffersink get frame fail, ret: %d\n", ret);
                    goto end;
                }

                // 获取一个采样点字节数，比如16位采样点值为2字节
                int data_size = av_get_bytes_per_sample(dec_ctx2->sample_fmt);

                // frame->nb_samples为这个frame中一个声道的采样点的个数
//                for (int i = 0; i < filt_frame->nb_samples; i++)
//                    for (int ch = 0; ch < dec_ctx2->channels; ch++)
//                        if (filt_frame->data[ch])
//                            fwrite(filt_frame->data[ch] + data_size * i, 1, data_size, fwirte);

                print_frame(filt_frame);
                av_frame_unref(filt_frame);
            }
        }
    }
    end:
    avfilter_graph_free(&filter_graph);
    avcodec_close(dec_ctx1);
    avformat_close_input(&fmt_ctx1);
    avcodec_close(dec_ctx2);
    avformat_close_input(&fmt_ctx2);
    av_frame_free(&frame);
    av_frame_free(&filt_frame);

    if (ret < 0 && ret != AVERROR_EOF) {
        fprintf(stderr, "Error occurred: %s\n", av_err2str(ret));
        exit(1);
    }

    exit(0);
}