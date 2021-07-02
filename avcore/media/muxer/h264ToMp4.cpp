//
// Created by 阳坤 on 2021/6/29.
//

#include "h264ToMp4.h"

int ptsInc = 0;
int waitkey = 1;;
int STREAM_FRAME_RATE = 25;
AVFormatContext *m_pOc;
int vi;

bool isIdrFrame2(uint8_t *buf, int len) {

    switch (buf[0] & 0x1f) {
        case 7: // SPS
            return true;
        case 8: // PPS
            return true;
        case 5:
            return true;
        case 1:
            return false;

        default:
            return false;
            break;
    }

    return false;
}

bool isIdrFrame1(uint8_t *buf, int size) {
    //主要是解析idr前面的sps pps
//    static bool found = false;
//    if(found){ return true;}

    int last = 0;
    for (int i = 2; i <= size; ++i) {
        if (i == size) {
            if (last) {
                bool ret = isIdrFrame2(buf + last, i - last);
                if (ret) {
                    //found = true;
                    return true;
                }
            }
        } else if (buf[i - 2] == 0x00 && buf[i - 1] == 0x00 && buf[i] == 0x01) {
            if (last) {
                int size = i - last - 3;
                if (buf[i - 3]) ++size;
                bool ret = isIdrFrame2(buf + last, size);
                if (ret) {
                    //found = true;
                    return true;
                }
            }
            last = i + 1;
        }
    }
    return false;

}


/* Add an output stream */
AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id) {
    AVCodecContext *c;
    AVStream *st;
    /* find the encoder */
    *codec = avcodec_find_encoder(codec_id);
    if (!*codec) {
        printf("could not find encoder for '%s' \n", avcodec_get_name(codec_id));
        exit(1);
    }
    st = avformat_new_stream(oc, *codec);
    if (!st) {
        printf("could not allocate stream \n");
        exit(1);
    }
    st->id = oc->nb_streams - 1;
    c = st->codec;
    vi = st->index;
    switch ((*codec)->type) {
        case AVMEDIA_TYPE_AUDIO:
            printf("AVMEDIA_TYPE_AUDIO\n");
            c->sample_fmt = (*codec)->sample_fmts ? (*codec)->sample_fmts[0] : AV_SAMPLE_FMT_FLTP;
            c->bit_rate = 64000;
            c->sample_rate = 44100;
            c->channels = 2;
            break;
        case AVMEDIA_TYPE_VIDEO:
            printf("AVMEDIA_TYPE_VIDEO\n");
            c->codec_id = AV_CODEC_ID_H264;
            c->bit_rate = 0;
            c->width = 1080;
            c->height = 720;
            c->time_base.den = 50;
            c->time_base.num = 1;
            c->gop_size = 1;
            c->pix_fmt = AV_PIX_FMT_YUV420P;
            if (c->codec_id == AV_CODEC_ID_MPEG2VIDEO) {
                c->max_b_frames = 2;
            }
            if (c->codec_id == AV_CODEC_ID_MPEG1VIDEO) {
                c->mb_decision = 2;
            }
            break;
        default:
            break;
    }
    if (oc->oformat->flags & AVFMT_GLOBALHEADER) {
        c->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    return st;
}

void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st) {
    int ret;
    AVCodecContext *c = st->codec;
    /* open the codec */
    ret = avcodec_open2(c, codec, NULL);
    if (ret < 0) {
        printf("could not open video codec");
        //exit(1);
    }
}

AVStream *m_pVideoSt;

int CreateMp4(const char *filename) {
    int ret; // 成功返回0，失败返回1
    const char *pszFileName = filename;
    AVOutputFormat *fmt;
    AVCodec *video_codec;
//    AVStream *m_pVideoSt;
    av_register_all();
    avformat_alloc_output_context2(&m_pOc, NULL, NULL, pszFileName);
    if (!m_pOc) {
        printf("Could not deduce output format from file extension: using MPEG. \n");
        avformat_alloc_output_context2(&m_pOc, NULL, "mpeg", pszFileName);
    }
    if (!m_pOc) {
        return 1;
    }
    fmt = m_pOc->oformat;
    if (fmt->video_codec != AV_CODEC_ID_NONE) {
        printf("1111111111111111add_stream\n");
        m_pVideoSt = add_stream(m_pOc, &video_codec, fmt->video_codec);
    }
    if (m_pVideoSt) {
        printf("1111111111111111open_video\n");
        open_video(m_pOc, video_codec, m_pVideoSt);
    }
    printf("==========Output Information==========\n");
    av_dump_format(m_pOc, 0, pszFileName, 1);
    printf("======================================\n");
    /* open the output file, if needed */
    if (!(fmt->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_pOc->pb, pszFileName, AVIO_FLAG_WRITE);
        if (ret < 0) {
            printf("could not open %s\n", pszFileName);
            return 1;
        }
    }
    /* Write the stream header, if any */
    ret = avformat_write_header(m_pOc, NULL);
    if (ret < 0) {
        printf("Error occurred when opening output file");
        return 1;
    }
}

/* write h264 data to mp4 file

 * 创建mp4文件返回2；写入数据帧返回0 */
void WriteVideo(void *data, int nLen, AVStream* in_stream) {
    int ret;
    if (0 > vi) {
        printf("vi less than 0\n");
        //return -1;
    }
    AVStream *pst = m_pOc->streams[vi];
    //printf("vi=====%d\n",vi);
    // Init packet
    AVPacket pkt;
    // 我的添加，为了计算pts
    AVCodecContext *c = pst->codec;
    av_init_packet(&pkt);
    int isI = isIdrFrame1((uint8_t *) data, nLen);
    printf("isIFrame is %d\n", isI);
    pkt.flags |= isI ? AV_PKT_FLAG_KEY : 0;
    pkt.stream_index = pst->index;
    pkt.data = (uint8_t *) data;
    pkt.size = nLen;
    // Wait for key frame
    if (waitkey) {
        if (0 == (pkt.flags & AV_PKT_FLAG_KEY)) {
            return;
        } else
            waitkey = 0;
    }


    if (pkt.pts == AV_NOPTS_VALUE) {
        //Write PTS
        AVRational time_base1 = in_stream->time_base;
        //Duration between 2 frames (us)
        int64_t calc_duration = (double) AV_TIME_BASE / av_q2d(in_stream->r_frame_rate);
        //Parameters
        pkt.pts = (double) (ptsInc * calc_duration) / (double) (av_q2d(time_base1) * AV_TIME_BASE);
        //tempo
        pkt.pts = (double) pkt.pts * 0.25;
        pkt.dts = pkt.pts;
        pkt.duration = (double) calc_duration / (double) (av_q2d(time_base1) * AV_TIME_BASE);
        ptsInc++;
    }


    //Convert PTS/DTS
    pkt.pts = av_rescale_q_rnd(pkt.pts, in_stream->time_base, m_pVideoSt->time_base,
                               (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.dts = av_rescale_q_rnd(pkt.dts, in_stream->time_base, m_pVideoSt->time_base,
                               (AVRounding) (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
    pkt.duration = av_rescale_q(pkt.duration, in_stream->time_base, m_pVideoSt->time_base);
    pkt.stream_index = 0;
    pkt.pos = -1;

    printf("pkt.size=%d\n", pkt.size);
    ret = av_interleaved_write_frame(m_pOc, &pkt);
    if (ret < 0) {
        printf("cannot write frame");
    }

//    ptsInc++;
}

void CloseMp4() {
    waitkey = -1;
    vi = -1;
    if (m_pOc)
        av_write_trailer(m_pOc);
    if (m_pOc && !(m_pOc->oformat->flags & AVFMT_NOFILE))
        avio_close(m_pOc->pb);
    if (m_pOc) {
        avformat_free_context(m_pOc);
        m_pOc = NULL;
    }
}