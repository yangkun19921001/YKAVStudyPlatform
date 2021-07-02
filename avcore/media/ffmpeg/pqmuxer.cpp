//
// Created by 阳坤 on 2021/6/30.
//

#include "pqmuxer.h"

/**
 * 实例化创建执行
 */
FFmpegMuxer::FFmpegMuxer() {
    this->ofmt = NULL;
    this->ifmt_ctx_v = NULL;
    this->ofmt_ctx = NULL;
    this->ofmt = NULL;
    this->videoindex_v = -1;
    this->videoindex_out = -1;
    this->ret = -1;
    this->audioTempo = 1.0;
    this->videoTempo = 1.0;
}

/**
 * 当前对象释放
 */
FFmpegMuxer::~FFmpegMuxer() {
}

int FFmpegMuxer::InitFFmpeg(const char *inpath, const char *outpath) {
    printf("FFmpeg INIT START! inpath=%s,outpath=%s\n", inpath, outpath);
    if (!inpath || !outpath) {
        printf("check inpath or outpath isEmpty? inpath=%s outpath=%s\n", inpath, outpath);
        return ERROR;
    }
    const char *in_filename_v = inpath;
    const char *out_filename = outpath;
    //注册 ffmpeg 所有函数
    av_register_all();
    //初始化 FFmpeg 网络框架
    avformat_network_init();
    //打开输入流，获取头信息
    if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
        printf("Could not open input file.\n");
        CloseMuxer();
        return ERROR;
    }

    //获取流信息
    if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
        printf("Failed to retrieve input stream information %s\n", strerror(ret));
        CloseMuxer();
        return ERROR;
    }
    //打印输入流信息
    printf("===========Input Information==========\n");
    av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
    printf("======================================\n");

    //实例化输出格式上下文
    if ((ret = avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename)) < 0) {
        printf("Could not create output context ! %s\n", strerror(ret));
        CloseMuxer();
        return ERROR;
    }
    //输出格式简单赋值
    ofmt = ofmt_ctx->oformat;

    //找到视频流
    videoindex_v = av_find_best_stream(ifmt_ctx_v, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    //说明找到流了
    if (videoindex_v >= 0) {
        AVStream *in_stream = ifmt_ctx_v->streams[videoindex_v];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        if (!out_stream) {
            printf("Failed allocating output stream\n");
            CloseMuxer();
            return ERROR;
        }
        videoindex_out = out_stream->index;

        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            printf("Failed to copy context from input to output stream codec context\n");
            CloseMuxer();
            return ERROR;
        }
        out_stream->codec->codec_tag = 0;

        printf("==========Output Information==========\n");
        av_dump_format(ofmt_ctx, 0, out_filename, 1);
        printf("======================================\n");

        //打开输出文件写入 header
        if (!(ofmt->flags & AVFMT_NOFILE)) {
            if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
                printf("Could not open output file '%s'", out_filename);
                CloseMuxer();
                return ERROR;
            }
        }
        if (avformat_write_header(ofmt_ctx, NULL) < 0) {
            printf("Error occurred when opening output file\n");
            CloseMuxer();
            return ERROR;
        }
    } else {
        printf("Could not find video stream info\n");
        CloseMuxer();
        return ERROR;
    }
    //总时长 毫秒
    int total_seconds = ifmt_ctx_v->duration / (AV_TIME_BASE);
    int hour = total_seconds / 3600;
    int minute = (total_seconds % 3600) / 60;
    int second = (total_seconds % 60);
    printf("total=%d ,duration: %02d:%02d:%02d\n", total_seconds, hour, minute, second);
    printf("FFmpeg INIT SUCCESS!%s\n", inpath, outpath);
    return SUCCESS;
}

int FFmpegMuxer::StartMuxer(double aTempo, double vTempo, TEMPO_LIST tempoList) {
    printf("FFmpeg StartMuxer START! aTempo=%f vTempo=%f\n", aTempo, vTempo);
    if (vTempo > 0.0)
        this->videoTempo = vTempo;
    int frame_index = 0;
    int64_t lastPts = 0;
    pkt = av_packet_alloc();
    if (!pkt)return ERROR;
    while (1) {
        AVFormatContext *ifmt_ctx = NULL;
        int stream_index = 0;
        AVStream *in_stream = NULL, *out_stream = NULL;
        ifmt_ctx = ifmt_ctx_v;
        stream_index = videoindex_out;
        if (av_read_frame(ifmt_ctx, pkt) >= 0) {
            do {
                in_stream = ifmt_ctx->streams[pkt->stream_index];
                out_stream = ofmt_ctx->streams[stream_index];
                if (pkt->stream_index == videoindex_v) {
                    //FIX：No PTS (Example: Raw H.264)
                    //Simple Write PTS
                    if (pkt->pts == AV_NOPTS_VALUE) {
                        //Write PTS
                        AVRational time_base1 = in_stream->time_base;
                        //Duration between 2 frames (us)
                        int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
                        pkt->pts =
                                (double) (frame_index * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
                        //实际的帧率时间
//                        pkt->pts = (double) (frame_index * calc_duration) / 1000;
//                        pkt->pts = frame_index*calc_duration*1000;
                        pkt->duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
                        int start_code_len = 4;
                        if (pkt->data[0] == 0x00 && pkt->data[1] == 0x00 && pkt->data[2] == 0x01)start_code_len = 3;
                        else start_code_len = 4;
                        frame_index++;
                    }
                    break;
                }
            } while (av_read_frame(ifmt_ctx, pkt) >= 0);
        } else {
            break;
        }
        AVRational time_base1 = in_stream->time_base;
        int64_t calc_duration = (double) AV_TIME_BASE / 1000 / av_q2d(in_stream->r_frame_rate);
        int index = frame_index - 1;
        int64_t cPts = index * calc_duration;
//        printf("frame_index=%d cPts=%lld calc_duration=%lld oldPts=%lld\n", frame_index, cPts, calc_duration, pkt->pts);
        this->videoTempo = 1.0;
        if (!tempoList.empty()) {
            TempoValue tempoValue = tempoList.front();
            //如果当前视频时间戳比需要倍速的时间戳大，那么需要取下一个节点
            while (cPts > tempoValue.end && !tempoList.empty()) {
                tempoList.pop_front();
                tempoValue = tempoList.front();
            }
            if (cPts >= tempoValue.start && cPts < tempoValue.end) {
                this->videoTempo = tempoValue.vTempo;
                if (tempoValue.vTempo > 0.0) {
                    pkt->pts /= this->videoTempo;
                    pkt->duration /= this->videoTempo;
                    printf("config vtempo=%lf cPts=%lld pts=%lld\n", tempoValue.vTempo, cPts, pkt->pts);
                }
            }
        } else {
            pkt->pts /= videoTempo;
        }
        if (lastPts >= 5950) {
            printf("lastPts=%lld\n", lastPts);
        }
        //如果当前的 pts 小于上一帧的 pts
        if ((pkt->pts != 0 && pkt->pts <= lastPts)//主要条件
            || (pkt->pts > lastPts + (double) (calc_duration * 2 * 1000) / videoTempo)) //*2 是因为last比当前 pts 小一帧
        {
            printf("---------->lastPts=%lld pkt->pts=%lld,videoTempo=%f\n", lastPts, pkt->pts, videoTempo);
            pkt->pts = lastPts + (double) (calc_duration * 1000) / videoTempo;
        }
        lastPts = pkt->pts;
        printf("lastPts=%lld pkt->pts=%lld,\n", lastPts, pkt->pts);
//        pkt->pts *= 1000;
        //Convert PTS/DTS
        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base,
                                    (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base,
                                    (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;
        pkt->stream_index = stream_index;
//        printf("Write Packet. size:%5d\tpts:%lld\n", pkt->size, pkt->pts);

        //Write
        if (av_interleaved_write_frame(ofmt_ctx, pkt) < 0) {
            printf("Error muxing packet\n");
            CloseMuxer();
            return ERROR;
        }
        //释放，引用计数-1 为0释放空间
        av_packet_unref(pkt);
    }

    printf("FFmpeg StartMuxer END!\n");

    CloseMuxer();

    return SUCCESS;
}

int FFmpegMuxer::CloseMuxer() {
    printf("FFmpeg CloseMuxer START!\n");
    if (ofmt_ctx) av_write_trailer(ofmt_ctx);
    if (pkt) av_packet_free(&pkt);
    if (ifmt_ctx_v) avformat_close_input(&ifmt_ctx_v);
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE)) avio_close(ofmt_ctx->pb);
    if (ofmt_ctx) avformat_free_context(ofmt_ctx);
    ofmt_ctx = NULL;
    pkt = NULL;
    ifmt_ctx_v = NULL;
    printf("FFmpeg CloseMuxer END!\n");
    return SUCCESS;
}



