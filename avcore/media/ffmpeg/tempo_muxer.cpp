//
// Created by 阳坤 on 2021/6/29.
//

#include "tempo_muxer.h"
#include "muxer.h"
#include "pqmuxer.h"

int muxer(const char *src, const char *dst) {
    AVOutputFormat *ofmt = NULL;
    AVFormatContext *ifmt_ctx_v = NULL, *ofmt_ctx = NULL;
    int ret;
    int videoindex_v = -1, videoindex_out = -1;
    int frame_index = 0;
//    int64_t cur_pts_v=0;

    enum AVRounding avRounding;

    const char *in_filename_v = src;
    const char *out_filename = dst;
    //The very start of the muxing
    av_register_all();
    avformat_network_init();
    AVPacket *pkt = av_packet_alloc();
    //write basic info into input AVFormatContext(1)
    if ((ret = avformat_open_input(&ifmt_ctx_v, in_filename_v, 0, 0)) < 0) {
        printf("Could not open input file.");
        goto end;
    }
    //write basic info into input AVFormatContext(2)
    if ((ret = avformat_find_stream_info(ifmt_ctx_v, 0)) < 0) {
        printf("Failed to retrieve input stream information");
        goto end;
    }

    printf("===========Input Information==========\n");
    av_dump_format(ifmt_ctx_v, 0, in_filename_v, 0);
    printf("======================================\n");

    //alloc memory for output AVFormatContext
    avformat_alloc_output_context2(&ofmt_ctx, NULL, NULL, out_filename);
    if (!ofmt_ctx) {
        printf("Could not create output context\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }
    ofmt = ofmt_ctx->oformat;

    //make sure the input stream is video
    if (ifmt_ctx_v->streams[0]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
        AVStream *in_stream = ifmt_ctx_v->streams[0];
        AVStream *out_stream = avformat_new_stream(ofmt_ctx, in_stream->codec->codec);
        videoindex_v = 0;
        if (!out_stream) {
            printf("Failed allocating output stream\n");
            ret = AVERROR_UNKNOWN;
            goto end;
        }
        videoindex_out = out_stream->index;
        //Copy the settings of AVCodecContext
        if (avcodec_copy_context(out_stream->codec, in_stream->codec) < 0) {
            printf("Failed to copy context from input to output stream codec context\n");
            goto end;
        }
        out_stream->codec->codec_tag = 0;
    } else {
        printf("Not a video file\n");
        ret = AVERROR_UNKNOWN;
        goto end;
    }

    printf("==========Output Information==========\n");
    av_dump_format(ofmt_ctx, 0, out_filename, 1);
    printf("======================================\n");

    //Open output file
    if (!(ofmt->flags & AVFMT_NOFILE)) {
        if (avio_open(&ofmt_ctx->pb, out_filename, AVIO_FLAG_WRITE) < 0) {
            printf("Could not open output file '%s'", out_filename);
            goto end;
        }
    }

    if (avformat_write_header(ofmt_ctx, NULL) < 0) {
        printf("Error occurred when opening output file\n");
        goto end;
    }

    //write in the content of video frame by frame
    while (1) {
        AVFormatContext *ifmt_ctx;
        int stream_index = 0;
        AVStream *in_stream, *out_stream;

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
                        //Parameters
                        pkt->pts =
                                (double) (frame_index * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
                        pkt->pts /= 2;
                        pkt->dts = pkt->pts;
                        pkt->duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
                        frame_index++;
                    }
//                    cur_pts_v=pkt->pts;
                    break;
                }
            } while (av_read_frame(ifmt_ctx, pkt) >= 0);
        } else {
            break;
        }

        //Convert PTS/DTS
        pkt->pts = av_rescale_q_rnd(pkt->pts, in_stream->time_base, out_stream->time_base, avRounding);
        pkt->dts = av_rescale_q_rnd(pkt->dts, in_stream->time_base, out_stream->time_base, avRounding);
        pkt->duration = av_rescale_q(pkt->duration, in_stream->time_base, out_stream->time_base);
        pkt->pos = -1;
        pkt->stream_index = stream_index;

        printf("Write 1 Packet. size:%5d\tpts:%lld\n", pkt->size, pkt->pts);
        //Write
        if (av_interleaved_write_frame(ofmt_ctx, pkt) < 0) {
            printf("Error muxing packet\n");
            break;
        }
        //释放，引用计数-1 为0释放空间
        av_packet_unref(pkt);
    }

    av_write_trailer(ofmt_ctx);
    av_packet_free(&pkt);

    end:
    avformat_close_input(&ifmt_ctx_v);
    if (ofmt_ctx && !(ofmt->flags & AVFMT_NOFILE))
        avio_close(ofmt_ctx->pb);
    avformat_free_context(ofmt_ctx);
    av_packet_free(&pkt);
    if (ret < 0 && ret != AVERROR_EOF) {
        printf("Error occurred.\n");
        return -1;
    }
    return 0;
}

int main(int argc, char *argv[]) {
    printf("configure=%s \n", avcodec_configuration());

//    if (argc < 2) {
//        printf("请输入文件路径!");
//        return EXIT_FAILURE;
//    }

//    const char * inpath = argv[1];
//    const char * outpath = argv[2];
//    const char *inpath = "https://clipres.yishihui.com/test/noBFrame.mp4";
//    const char *inpath = "https://clipres.yishihui.com/longvideo/material/video/vpc/20210624/e88d408b90bb4c64aeb470d440019e051624515940174";
    const char *inpath = "/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/test.h264";
//    const char *inpath = "/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/test.h265";
    const char *outpath = "/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/test.mp4";

    FFmpegMuxer *ffmpegMuxer = new FFmpegMuxer();
    TEMPO_LIST list;

    TempoValue value;
    value.vTempo = 0.5;
    value.start = 0;
    value.end = 2 * 1000;
    list.push_back(value);

    TempoValue value2;
    value2.vTempo = 4;
    value2.start = 2 * 1000;
    value2.end = 10 * 1000;
    list.push_back(value2);

    TempoValue value3;
    value3.vTempo = 1.9;
    value3.start = 13 * 1000;
    value3.end = 17 * 1000;
    list.push_back(value3);

    TempoValue value4;
    value4.vTempo = 2.5;
    value4.start = 17 * 1000;
    value4.end = 27 * 1000;
    list.push_back(value4);

    TempoValue value5;
    value5.vTempo = 3.0;
    value5.start = 27 * 1000;
    value5.end = 1*60 * 1000;
    list.push_back(value5);

    TempoValue value6;
    value6.vTempo = 0.2;
    value6.start = 1*60 * 1000;
    value6.end = 2*60 * 1000;
    list.push_back(value6);

//    list.clear();
    if (ffmpegMuxer->InitFFmpeg(inpath, outpath) == SUCCESS) {
        if (ffmpegMuxer->StartMuxer(1.0, 1.0, list) == SUCCESS) {
            ffmpegMuxer->CloseMuxer();
        }
    }
    delete ffmpegMuxer;
    ffmpegMuxer = NULL;
    return 1;
    //初始化网络库(可以打开 rtmp、rtsp、http 等协议的流媒体体视频)
    avformat_network_init();

    //参数设置
    AVDictionary *opts = NULL;
    //设置rtsp流已tcp协议打开
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    //网络延时时间
    av_dict_set(&opts, "max_delay", "1000", 0);

    MP4Muxer *mp4Muxer = new MP4Muxer();
    mp4Muxer->Start(outpath);
    mp4Muxer->setAVTempo(1.0, 0.1);
    //解封装上下文
    AVFormatContext *ic = NULL;
    int re = avformat_open_input(
            &ic,
            inpath,
            0,  // 0表示自动选择解封器
            &opts //参数设置，比如rtsp的延时时间
    );
    if (re != 0) {
        char buf[1024] = {0};
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "open " << inpath << " failed! :" << buf << endl;
        return -1;
    }
    cout << "open " << inpath << " success! " << endl;

    //获取流信息
    re = avformat_find_stream_info(ic, 0);

    //输出上下文
    AVFormatContext *oc = NULL;
    avformat_alloc_output_context2(&oc, NULL, "mp4", outpath);

    if (re < 0) {
        char buf[1024] = {0};
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "avformat_alloc_output_context2 " << outpath << " failed! :" << buf << endl;
        return -1;
    }

    //总时长 毫秒
    int totalMs = ic->duration / (AV_TIME_BASE / 1000);
    cout << "totalMs = " << totalMs << endl;

    //打印视频流详细信息
    av_dump_format(ic, 0, inpath, 0);

    //音视频索引，读取时区分音视频
    int videoStream = 0;
    int audioStream = 1;



    //获取音视频流信息 （遍历，函数获取）
    for (int i = 0; i < ic->nb_streams; i++) {
        AVStream *as = ic->streams[i];
        cout << "codec_id = " << as->codecpar->codec_id << endl;
        cout << "format = " << as->codecpar->format << endl;

        //音频 AVMEDIA_TYPE_AUDIO
        if (as->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioStream = i;
            cout << i << "-音频信息" << endl;
            cout << "sample_rate = " << as->codecpar->sample_rate << endl;
            //AVSampleFormat;
            cout << "channels = " << as->codecpar->channels << endl;
            //一帧数据？？ 单通道样本数
            cout << "frame_size = " << as->codecpar->frame_size << endl;

        }
            //视频 AVMEDIA_TYPE_VIDEO
        else if (as->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream = i;
            cout << i << "-视频信息" << endl;
            cout << "width=" << as->codecpar->width << endl;
            cout << "height=" << as->codecpar->height << endl;
            cout << "bitrate=" << as->codecpar->bit_rate << endl;
            cout << "format=" << as->codecpar->format << endl;
            cout << "video fps = " << r2d(as->avg_frame_rate) << endl;
        }
    }

    //获取视频流
    videoStream = av_find_best_stream(ic, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    audioStream = av_find_best_stream(ic, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);

    //malloc AVPacket并初始化
    AVPacket *pkt = av_packet_alloc();

    for (;;) {
        int re = av_read_frame(ic, pkt);
        if (re < 0) {
            //循环播放
            cout << "==============================end==============================" << endl;
            cout << "==============================end==============================" << endl;
            cout << "==============================end==============================" << endl;
            break;
        }

        //转换为毫秒，方便做同步
        cout << "pkt->pts ms = " << pkt->pts * (r2d(ic->streams[pkt->stream_index]->time_base) * 1000) << endl;

        //解码时间
        cout << "pkt->dts = " << pkt->dts << endl;
        if (pkt->stream_index == videoStream) {
            cout << "图像" << endl;
            mp4Muxer->AppendVideo(pkt->data, pkt->size);
        }
        if (pkt->stream_index == audioStream) {
            cout << "音频" << endl;
        }
        //释放，引用计数-1 为0释放空间
        av_packet_unref(pkt);

    }
    mp4Muxer->Stop();
    if (pkt)av_packet_free(&pkt);
    //释放封装上下文，并且把ic置0
    if (ic) avformat_close_input(&ic);

    pkt = NULL;
    ic = NULL;
    return 1;
}