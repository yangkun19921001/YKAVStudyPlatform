//
// Created by 阳坤 on 2021/6/21.
//
#include "decode.h"

int main(int argc, char *argv[]) {
    printf("configure=%s \n", avcodec_configuration());

//    if (argc < 2) {
//        printf("请输入文件路径!");
//        return EXIT_FAILURE;
//    }

//    const char * inpath = argv[1];
//    const char * outpath = argv[2];
//    const char *inpath = "https://clipres.yishihui.com/test/noBFrame.mp4";
    const char *inpath = "/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/clip.mp4";
    const char *outpath = "/Users/devyk/Downloads/0.mp4";

    //初始化网络库(可以打开 rtmp、rtsp、http 等协议的流媒体体视频)
    avformat_network_init();

    //参数设置
    AVDictionary *opts = NULL;
    //设置rtsp流已tcp协议打开
    av_dict_set(&opts, "rtsp_transport", "tcp", 0);
    //网络延时时间
    av_dict_set(&opts, "max_delay", "1000", 0);

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
            //1024 * 2 * 2 = 4096  fps = sample_rate/frame_size

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

    //找到视频解码器
    AVCodec *vcodec = avcodec_find_decoder(ic->streams[videoStream]->codecpar->codec_id);
    if (!vcodec) {
        cout << "can't find the codec id" << ic->streams[videoStream]->codecpar->codec_id << endl;
        return -1;
    }
    cout << "find video the AVCodec id = " << ic->streams[videoStream]->codecpar->codec_id << endl;

    //创建视频解码器上下文
    AVCodecContext *vctx = avcodec_alloc_context3(vcodec);
    //配置解码器上下文参数
    avcodec_parameters_to_context(vctx, ic->streams[videoStream]->codecpar);
    //配置解码线程
    vctx->thread_count = 8;
    //打开解码器上下文
    re = avcodec_open2(vctx, 0, 0);
    if (re != 0) {
        char buf[1024] = {0};
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "video avcodec_open2 failed!" << buf << endl;
        return -1;
    }
    cout << "video avcodec_open2 success!" << endl;

    //找到音频解码器
    AVCodec *acodec = avcodec_find_decoder(ic->streams[audioStream]->codecpar->codec_id);
    if (!acodec) {
        cout << "can't find the codec id" << ic->streams[audioStream]->codecpar->codec_id << endl;
        return -1;
    }
    cout << "find audio the AVCodec id = " << ic->streams[audioStream]->codecpar->codec_id << endl;

    //创建解码器上下文
    AVCodecContext *actx = avcodec_alloc_context3(acodec);
    //配置解码器上下文参数
    avcodec_parameters_to_context(actx, ic->streams[audioStream]->codecpar);
    //配置解码线程
    vctx->thread_count = 8;
    //打开解码器上下文
    re = avcodec_open2(actx, 0, 0);
    if (re != 0) {
        char buf[1024] = {0};
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "audio avcodec_open2 failed!" << buf << endl;
        return -1;
    }
    cout << "audio avcodec_open2 success!" << endl;

    //malloc AVPacket并初始化
    AVPacket *pkt = av_packet_alloc();
    //接收解码的原始数据
    AVFrame *frame = av_frame_alloc();

    //像素格式转换上下文
    SwsContext *vsctx = NULL;
    //原始像素格式转换为 rgb 的存储
    unsigned char *rgb = NULL;
    //音频重采样
    SwrContext *asctx = swr_alloc();
    //设置重采样参数
    asctx = swr_alloc_set_opts(asctx //重采样上下文
            , av_get_default_channel_layout(2)//输出声道格式
            , AV_SAMPLE_FMT_S16 //输出声音样本格式
            , 48000 //输出采样率
            , av_get_default_channel_layout(actx->channels)//输入通道数
            , actx->sample_fmt                            //输入声音样本格式
            , actx->sample_rate, 0, 0  //输入音频采样率
    );
    //初始化采样上下文
    re = swr_init(asctx);
    if (re != 0) {
        char buf[1024] = {0};
        av_strerror(re, buf, sizeof(buf) - 1);
        cout << "audio swr_init failed!" << buf << endl;
        return -1;
    }
    //重采样之后存入的数据
    unsigned char *pcm = NULL;
    for (;;) {
        int re = av_read_frame(ic, pkt);
        if (re != 0) {
            //循环播放
            cout << "==============================end==============================" << endl;
            cout << "==============================end==============================" << endl;
            cout << "==============================end==============================" << endl;
//          YSleep(3000);
            int ms = 3000; //三秒位置 根据时间基数（分数）转换
            long long pos = (double) ms / (double) 1000 / r2d(ic->streams[videoStream]->time_base);
            re = av_seek_frame(ic, videoStream, pos, AVSEEK_FLAG_BACKWARD | AVSEEK_FLAG_FRAME);
            if (re >= 0) {
                cout << "av_seek_frame success   " << endl;
            }
            continue;
//            break;
        }

        cout << "pkt->size = " << pkt->size << endl;
        //显示的时间
        cout << "pkt->pts = " << pkt->pts << endl;

        //转换为毫秒，方便做同步
        cout << "pkt->pts ms = " << pkt->pts * (r2d(ic->streams[pkt->stream_index]->time_base) * 1000) << endl;

        //解码时间
        cout << "pkt->dts = " << pkt->dts << endl;
        AVCodecContext *avcc = NULL;
        if (pkt->stream_index == videoStream) {
            cout << "图像" << endl;
            avcc = vctx;
        }
        if (pkt->stream_index == audioStream) {
            cout << "音频" << endl;
            avcc = actx;
        }

        //解码视频
        //发送 packet 到解码线程
        re = avcodec_send_packet(avcc, pkt);
        //释放，引用计数-1 为0释放空间
        av_packet_unref(pkt);
        if (re != 0) {
            char buf[1024] = {0};
            av_strerror(re, buf, sizeof(buf) - 1);
            cout << "video avcodec_send_packet failed!" << buf << endl;
            continue;
        }

        //一次 send 可能对于多次 receive
        for (;;) {
            re = avcodec_receive_frame(avcc, frame);
            if (re != 0)break;

            switch (frame->format) {
                case AV_PIX_FMT_YUV420P:
                    cout << "avcodec_receive_frame format = AV_PIX_FMT_YUV420P" << " " << frame->linesize[0] << endl;
                    break;
                case AV_PIX_FMT_NV12:
                    cout << "avcodec_receive_frame format = AV_PIX_FMT_NV12" << " " << frame->linesize[0] << endl;
                    break;
                case AV_PIX_FMT_NV21:
                    cout << "avcodec_receive_frame format = AV_PIX_FMT_NV21" << " " << frame->linesize[0] << endl;
                    break;
                default:
                    break;
            }
            cout << "avcodec_receive_frame " << frame->width << " - " << frame->height << endl;
            cout << "avcodec_receive_frame sample_rate= " << frame->sample_rate << endl;
            cout << "avcodec_receive_frame channels= " << frame->channels << endl;

            switch (frame->pict_type) {
                case AV_PICTURE_TYPE_NONE:
                    cout << "avcodec_receive_frame pict_type= AV_PICTURE_TYPE_NONE" << endl;
                    break;
                case AV_PICTURE_TYPE_I:
                    cout << "avcodec_receive_frame pict_type= AV_PICTURE_TYPE_I" << endl;
                    break;
                case AV_PICTURE_TYPE_P:
                    cout << "avcodec_receive_frame pict_type= AV_PICTURE_TYPE_P" << endl;
                    break;
                case AV_PICTURE_TYPE_B:
                    cout << "avcodec_receive_frame pict_type= AV_PICTURE_TYPE_B" << endl;
                    break;
            }

            if (avcc == vctx) {
                const int in_width = frame->width;
                const int in_height = frame->height;
                const int out_width = in_width / 2;
                const int out_height = in_height / 2;
                /**
                 * @param context : 缩放上下文，如果为 NULL,那么内部会进行创建，
                 * 如果已经存在，参数也没有发生变化，那么就直接返回当前，否者释放缩放上下文，重新创建。
                 * @param srcW : 输入的宽
                 * @param srcH : 输入的高
                 * @param srcFormat : 输入的格式
                 * @param dstW : 输出的宽
                 * @param dstH : 输出的高
                 * @param dstFormat : 输出的格式
                 * @param flags : 提供了一系列的算法，快速线性，差值，矩阵，不同的算法性能也不同，快速线性算法性能相对较高。只针对尺寸的变换。
                 * @param srcFilter : 输入过滤器
                 * @param dstFilter : 输出过滤器
                 * @param param : 这个跟 flags 算法相关，一般传入 O
                 */
                vsctx = sws_getCachedContext(
                        vsctx,//传入NULL 会新创建
                        in_width, in_height, (AVPixelFormat) frame->format, //输入的宽高，格式
                        out_width, out_height, AV_PIX_FMT_RGBA, //输出的宽高，格式
                        SWS_BILINEAR, //尺寸变换的算法
                        0, 0, 0
                );
                if (vsctx) {
                    cout << "像素格式尺寸转换上下文创建或者获取成功！" << endl;
                    cout << "in_width=" << in_width << endl;
                    cout << "in_height=" << in_height << endl;
                    cout << "out_width=" << out_width << endl;
                    cout << "out_height=" << out_height << endl;
                    if (!rgb) rgb = new unsigned char[out_width * out_height * 4];
                    uint *data[2] = {0};
                    data[0] = (uint *) rgb;
                    data[1] = NULL;
                    int lines[2] = {0};
                    lines[0] = out_width * 4;
                    lines[1] = NULL;
                    /**
                     * @param c         缩放上下文
                     * @param srcSlice  YUV 切换数据可以是指针，也可以是数组
                     * @param srcStride 对应 YUV 一行的大小
                     * @param srcSliceY 这个用不到传入 0 即可
                     * @param srcSliceH YUV 的高
                     * @param dst       输出的像素格式数据
                     * @param dstStride 输出的像素格式数据的大小
                     * @return          返回转换后的高
                     */
                    //the height of the output slice
                    re = sws_scale(vsctx,
                                   frame->data, //输入数据
                                   frame->linesize,//输入行大小
                                   0,
                                   frame->height,//输出高度
                                   (uint8_t *const *) (data), //输出数据
                                   lines//输出大小
                    );
                    cout << "sws_scale success! return height of the output slice =" << re << endl;
                } else {
                    cout << "像素格式尺寸转换上下文创建或者获取失败！" << endl;
                }
            } else if (avcc == actx) {//音频
                uint8_t *data[2] = {0};
                if (!pcm) pcm = new uint8_t[frame->nb_samples * 16 / 8 * 2];
                data[0] = {pcm};
                int len = swr_convert(asctx, data //输出的数据
                        , frame->nb_samples //输出
                        , (const uint8_t **) frame->data //输入的音频数据
                        , frame->nb_samples //输入
                );
                if (len >= 0) {
                    cout << "swr_convert success return len = " << len << endl;
                } else {
                    cout << "swr_convert failed return len = " << len << endl;
                }
            }
        }

        //释放，引用计数-1 为0释放空间
        av_frame_unref(frame);
//        YSleep(20);
    }
    if (vsctx)sws_freeContext(vsctx);
    if (asctx)swr_close(asctx);
    if (asctx)swr_free(&asctx);
    if (pkt)av_packet_free(&pkt);
    if (frame)av_frame_free(&frame);
    //释放封装上下文，并且把ic置0
    if (ic) avformat_close_input(&ic);

    vsctx = NULL;
    asctx = NULL;
    pkt = NULL;
    frame = NULL;
    ic = NULL;
    return 1;
}

