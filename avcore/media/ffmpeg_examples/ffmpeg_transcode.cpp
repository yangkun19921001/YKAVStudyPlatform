// CGaveFile.cpp : ¶¨Òå¿ØÖÆÌ¨Ó¦ÓÃ³ÌÐòµÄÈë¿Úµã¡£
//
/* Copyright [c] 2018-2028 By www.chungen90.com Allrights Reserved
   This file give a simple example of transcoding TS file to
   Hls. Any questions, you can join QQ group for help, QQ
   Group number:127903734 or 766718184.
*/
#include <string>
#include <memory>
#include <thread>
#include <iostream>
#include <time.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
}

#include <stdio.h>


using namespace std;

AVFormatContext *inputContext = nullptr;
AVFormatContext *outputContext;
int64_t lastReadPacktTime;

static int interrupt_cb(void *ctx) {
    int timeout = 10;
//    if(av_gettime() - lastReadPacktTime > timeout *1000 *1000)
//    {
//        return -1;
//    }
    return 0;
}

int OpenInput(string inputUrl) {
    inputContext = avformat_alloc_context();
//    lastReadPacktTime = av_gettime();
//    inputContext->interrupt_callback.callback = interrupt_cb;
    int ret = avformat_open_input(&inputContext, inputUrl.c_str(), nullptr, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Input file open input failed\n");
        return ret;
    }
    ret = avformat_find_stream_info(inputContext, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Find input file stream inform failed\n");
    } else {
        av_log(NULL, AV_LOG_FATAL, "Open input file  %s success\n", inputUrl.c_str());
    }
    return ret;
}

shared_ptr<AVPacket> ReadPacketFromSource() {
    shared_ptr<AVPacket> packet(static_cast<AVPacket *>(av_malloc(sizeof(AVPacket))), [&](AVPacket *p) {
        av_packet_free(&p);
        av_freep(&p);
    });
    av_init_packet(packet.get());
//    lastReadPacktTime = av_gettime();
    int ret = av_read_frame(inputContext, packet.get());
    if (ret >= 0) {
        return packet;
    } else {
        return nullptr;
    }
}

void av_packet_rescale_ts(AVPacket *pkt, AVRational src_tb, AVRational dst_tb) {
    if (pkt->pts != AV_NOPTS_VALUE)
        pkt->pts = av_rescale_q(pkt->pts, src_tb, dst_tb);
    if (pkt->dts != AV_NOPTS_VALUE)
        pkt->dts = av_rescale_q(pkt->dts, src_tb, dst_tb);
    if (pkt->duration > 0)
        pkt->duration = av_rescale_q(pkt->duration, src_tb, dst_tb);
}

int WritePacket(shared_ptr<AVPacket> packet) {
    if (packet.get()->stream_index == AVMEDIA_TYPE_VIDEO)
    {
        printf("");
    } else {
        printf("");
    }
    auto inputStream = inputContext->streams[packet->stream_index];
    auto outputStream = outputContext->streams[packet->stream_index];
    av_packet_rescale_ts(packet.get(), inputStream->time_base, outputStream->time_base);
    return av_interleaved_write_frame(outputContext, packet.get());
}

int OpenOutput(string outUrl) {

    int ret = avformat_alloc_output_context2(&outputContext, nullptr, "hls", outUrl.c_str());
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open output context failed\n");
        goto Error;
    }

    ret = avio_open2(&outputContext->pb, outUrl.c_str(), AVIO_FLAG_WRITE, nullptr, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "open avio failed");
        goto Error;
    }
    av_opt_set(outputContext->priv_data, "hls_list_size", "0", AV_OPT_SEARCH_CHILDREN);

    for (int i = 0; i < inputContext->nb_streams; i++) {
        AVStream *stream = avformat_new_stream(outputContext, inputContext->streams[i]->codec->codec);
        ret = avcodec_copy_context(stream->codec, inputContext->streams[i]->codec);
        if (ret < 0) {
            av_log(NULL, AV_LOG_ERROR, "copy coddec context failed");
            goto Error;
        }
    }

    ret = avformat_write_header(outputContext, nullptr);
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "format write header failed");
        goto Error;
    }

    av_log(NULL, AV_LOG_FATAL, " Open output file success %s\n", outUrl.c_str());
    return ret;
    Error:
    if (outputContext) {
        for (int i = 0; i < outputContext->nb_streams; i++) {
            avcodec_close(outputContext->streams[i]->codec);
        }
        avformat_close_input(&outputContext);
    }
    return ret;
}

void CloseInput() {
    if (inputContext != nullptr) {
        avformat_close_input(&inputContext);
    }
}

void CloseOutput() {
    if (outputContext != nullptr) {
        for (int i = 0; i < outputContext->nb_streams; i++) {
            AVCodecContext *codecContext = outputContext->streams[i]->codec;
            avcodec_close(codecContext);
        }
        avformat_close_input(&outputContext);
    }
}

void Init() {
    av_register_all();
    avfilter_register_all();
    avformat_network_init();
    av_log_set_level(AV_LOG_ERROR);
}

int main(int argc, char *argv[]) {
    Init();
    int ret = OpenInput("/Users/devyk/Data/Project/piaoquan/PQMedia/temp/2.mp4");
    if (ret >= 0) {
        ret = OpenOutput("/Users/devyk/Data/Project/piaoquan/PQMedia/temp/2222-.m3u8");
    }
    if (ret < 0) goto Error;

    while (true) {
        auto packet = ReadPacketFromSource();
        if (packet) {
            ret = WritePacket(packet);
            if (ret >= 0) {
                cout << "WritePacket Success!" << endl;
            } else {
                cout << "WritePacket failed!" << endl;
            }
        } else {
            break;
        }
    }


    Error:
    CloseInput();
    CloseOutput();
//    while (true) {
//        this_thread::sleep_for(chrono::seconds(100));
//    }
    return 0;
}




/*
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersink.h>
#include <libavfilter/buffersrc.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/fifo.h>
#include <libavutil/opt.h>
#include <libavutil/pixdesc.h>
#include <libswresample/swresample.h>
}
#include <stdio.h>

#include <iostream>

#define BUF_SIZE_20K 2048000
#define BUF_SIZE_1K 1024000

static AVFormatContext *ifmt_ctx;
static AVFormatContext *ofmt_ctx;
static SwrContext *pSwrCtx = NULL;
static int in_sample_rate = 48000;
static int in_channels = 2;
static int64_t in_channel_layout = av_get_default_channel_layout(in_channels);
static enum AVSampleFormat in_sample_fmt = AV_SAMPLE_FMT_FLTP;
AVBitStreamFilterContext *aacbsfc = NULL;

void initSwr(int audio_index) {
  if (NULL == pSwrCtx) {
    pSwrCtx = swr_alloc();
  }
#if LIBSWRESAMPLE_VERSION_MINOR >= 17  // ���ݰ汾��ͬ��ѡ���ʵ�����
  av_opt_set_int(pSwrCtx, "ich",
                 ifmt_ctx->streams[audio_index]->codec->channels, 0);
  av_opt_set_int(pSwrCtx, "och",
                 ofmt_ctx->streams[audio_index]->codec->channels, 0);
  av_opt_set_int(pSwrCtx, "in_sample_rate",
                 ifmt_ctx->streams[audio_index]->codec->sample_rate, 0);
  av_opt_set_int(pSwrCtx, "out_sample_rate",
                 ofmt_ctx->streams[audio_index]->codec->sample_rate, 0);
  av_opt_set_sample_fmt(pSwrCtx, "in_sample_fmt",
                        ifmt_ctx->streams[audio_index]->codec->sample_fmt, 0);
  av_opt_set_sample_fmt(pSwrCtx, "out_sample_fmt",
                        ofmt_ctx->streams[audio_index]->codec->sample_fmt, 0);

#else
  pSwrCtx = swr_alloc_set_opts(
      NULL, ofmt_ctx->streams[audio_index]->codec->channel_layout,
      ofmt_ctx->streams[audio_index]->codec->sample_fmt,
      ofmt_ctx->streams[audio_index]->codec->sample_rate,
      ifmt_ctx->streams[audio_index]->codec->channel_layout,
      ifmt_ctx->streams[audio_index]->codec->sample_fmt,
      ifmt_ctx->streams[audio_index]->codec->sample_rate, 0, NULL);
#endif

  swr_init(pSwrCtx);
}

static void setup_array(uint8_t *out[32], AVFrame *in_frame,
                        enum AVSampleFormat format, int samples) {
  if (av_sample_fmt_is_planar(format)) {
    int i;
    int plane_size = av_get_bytes_per_sample((format)) * samples;
    in_frame->data[0] + i *plane_size;
    for (i = 0; i < in_frame->channels; i++) {
      out[i] = in_frame->data[i];
    }
  } else {
    out[0] = in_frame->data[0];
  }
}

int TransSample(AVFrame *in_frame, AVFrame *out_frame, int audio_index) {
  int ret;
  int max_dst_nb_samples = 4096;
  int64_t src_nb_samples = in_frame->nb_samples;
  out_frame->pts = in_frame->pts;
  uint8_t *paudiobuf;
  int decode_size, input_size, len;
  if (pSwrCtx != NULL) {
    out_frame->nb_samples = av_rescale_rnd(
        swr_get_delay(pSwrCtx,
                      ofmt_ctx->streams[audio_index]->codec->sample_rate) +
            src_nb_samples,
        ofmt_ctx->streams[audio_index]->codec->sample_rate, in_sample_rate,
        AV_ROUND_UP);

    ret = av_samples_alloc(
        out_frame->data, &out_frame->linesize[0],
        ofmt_ctx->streams[audio_index]->codec->channels, out_frame->nb_samples,
        ofmt_ctx->streams[audio_index]->codec->sample_fmt, 0);

    if (ret < 0) {
      av_log(NULL, AV_LOG_WARNING,
             "[%s.%d %s() Could not allocate samples Buffer\n", __FILE__,
             __LINE__, __FUNCTION__);
      return -1;
    }

    max_dst_nb_samples = out_frame->nb_samples;

    uint8_t *m_ain[32];
    setup_array(m_ain, in_frame, in_sample_fmt, src_nb_samples);

    len = swr_convert(pSwrCtx, out_frame->data, out_frame->nb_samples,
                      (const uint8_t **)in_frame->data, src_nb_samples);
    if (len < 0) {
      char errmsg[BUF_SIZE_1K];
      av_strerror(len, errmsg, sizeof(errmsg));
      av_log(NULL, AV_LOG_WARNING, "[%s:%d] swr_convert!(%d)(%s)", __FILE__,
             __LINE__, len, errmsg);
      return -1;
    }
  } else {
    printf("pSwrCtx with out init!\n");
    return -1;
  }
  return 0;
}
static int open_input_file(const char *filename) {
  int ret;
  unsigned int i;
  ifmt_ctx = NULL;
  if ((ret = avformat_open_input(&ifmt_ctx, filename, NULL, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot open input file\n");
    return ret;
  }
  if ((ret = avformat_find_stream_info(ifmt_ctx, NULL)) < 0) {
    av_log(NULL, AV_LOG_ERROR, "Cannot find stream information\n");
    return ret;
  }
  for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    AVStream *stream;
    AVCodecContext *codec_ctx;
    stream = ifmt_ctx->streams[i];
    codec_ctx = stream->codec;
    */
/* Reencode video & audio and remux subtitles etc. *//*

    if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO ||
        codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO) {
      */
/* Open decoder *//*

      ret = avcodec_open2(codec_ctx, avcodec_find_decoder(codec_ctx->codec_id),
                          NULL);
      if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open decoder for stream #%u\n",
               i);
        return ret;
      }
    }
  }
  av_dump_format(ifmt_ctx, 0, filename, 0);
  return 0;
}
static int open_output_file(const char *filename) {
  AVStream *out_stream;
  AVCodecContext *enc_ctx;
  AVCodec *encoder;
  int ret;
  unsigned int i;
  ofmt_ctx = NULL;
  avformat_alloc_output_context2(&ofmt_ctx, NULL, "hls", filename);
  if (!ofmt_ctx) {
    av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
    return AVERROR_UNKNOWN;
  }
  {
    out_stream = avformat_new_stream(ofmt_ctx, NULL);
    out_stream->index = 0;
    if (!out_stream) {
      av_log(NULL, AV_LOG_ERROR, "Failed allocating output stream\n");
      return AVERROR_UNKNOWN;
    }

    enc_ctx = out_stream->codec;
    enc_ctx->codec_type = AVMEDIA_TYPE_AUDIO;

    encoder = avcodec_find_encoder(AV_CODEC_ID_AAC);
    enc_ctx->sample_rate = in_sample_rate;
    enc_ctx->channel_layout = in_channel_layout;
    enc_ctx->channels = in_channels;
    enc_ctx->sample_fmt = encoder->sample_fmts[0];
    AVRational ar = {1, enc_ctx->sample_rate};
    enc_ctx->time_base = ar;

    ret = avcodec_open2(enc_ctx, encoder, NULL);
    if (ret < 0) {
      av_log(NULL, AV_LOG_ERROR, "Cannot open audio encoder for stream \n");
      return ret;
    }

    av_opt_set(ofmt_ctx->priv_data, "preset", "superfast", 0);
    av_opt_set(ofmt_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set_int(ofmt_ctx->priv_data, "hls_time", 5, AV_OPT_SEARCH_CHILDREN);
    // av_opt_set_int(ofmt_ctx->priv_data,"hls_list_size", 10,
    // AV_OPT_SEARCH_CHILDREN);

    if (ofmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
      enc_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  av_dump_format(ofmt_ctx, 0, filename, 1);

  // if (!(ofmt_ctx->oformat->flags & AVFMT_NOFILE)) {
  ret = avio_open(&ofmt_ctx->pb, filename, AVIO_FLAG_WRITE);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Could not open output file '%s'", filename);
    return ret;
  }
  // }
  */
/* init muxer, write output file header *//*

  ret = avformat_write_header(ofmt_ctx, NULL);
  if (ret < 0) {
    av_log(NULL, AV_LOG_ERROR, "Error occurred when opening output file\n");
    return ret;
  }
  return 0;
}
static int encode_write_frame(AVFrame *filt_frame, unsigned int stream_index,
                              int *got_frame) {
  static int a_total_duration = 0;
  static int v_total_duration = 0;
  int ret;
  int got_frame_local;
  AVPacket enc_pkt;
  int (*enc_func)(AVCodecContext *, AVPacket *, const AVFrame *, int *) =
      avcodec_encode_audio2;

  if (!got_frame) got_frame = &got_frame_local;

  */
/* encode filtered frame *//*

  enc_pkt.data = NULL;
  enc_pkt.size = 0;
  av_init_packet(&enc_pkt);
  ret = enc_func(ofmt_ctx->streams[stream_index]->codec, &enc_pkt, filt_frame,
                 got_frame);
  if (ret < 0) return ret;
  if (!(*got_frame)) return 0;
  // if (ifmt_ctx->streams[stream_index]->codec->codec_type !=
  // AVMEDIA_TYPE_VIDEO)
  // av_bitstream_filter_filter(aacbsfc, ofmt_ctx->streams[stream_index]->codec,
  // NULL, &enc_pkt.data, &enc_pkt.size, enc_pkt.data, enc_pkt.size, 0);

  */
/* prepare packet for muxing *//*

  enc_pkt.stream_index = stream_index;
  av_packet_rescale_ts(&enc_pkt,
                       ofmt_ctx->streams[stream_index]->codec->time_base,
                       ofmt_ctx->streams[stream_index]->time_base);

  if (ofmt_ctx->streams[stream_index]->codec->codec_type !=
      AVMEDIA_TYPE_VIDEO) {
    enc_pkt.pts = enc_pkt.dts = a_total_duration;
    a_total_duration +=
        av_rescale_q(filt_frame->nb_samples,
                     ofmt_ctx->streams[stream_index]->codec->time_base,
                     ofmt_ctx->streams[stream_index]->time_base);
  }
  // printf("v_total_duration: %d, a_total_duration: %d\n", v_total_duration,
  // a_total_duration);
  av_log(NULL, AV_LOG_DEBUG, "Muxing frame\n");
  */
/* mux encoded frame *//*

  ret = av_interleaved_write_frame(ofmt_ctx, &enc_pkt);
  return ret;
}

static int flush_encoder(unsigned int stream_index) {
  int ret;
  int got_frame;
  AVPacket enc_pkt;

  if (!(ofmt_ctx->streams[stream_index]->codec->codec->capabilities &
        AV_CODEC_CAP_DELAY))
    return 0;

  while (1) {
    enc_pkt.data = NULL;
    enc_pkt.size = 0;
    av_init_packet(&enc_pkt);

    av_log(NULL, AV_LOG_INFO, "Flushing stream #%u encoder\n", stream_index);
    ret = encode_write_frame(NULL, stream_index, &got_frame);
    if (ret < 0) break;
    if (!got_frame) return 0;
  }
  return ret;
}
int main(int argc, char **argv) {
    int ret;
    AVPacket packet;
    packet.data = NULL;
    packet.size = 0;
    AVFrame *frame = NULL;
    enum AVMediaType type;
    unsigned int stream_index = 0;
    unsigned int i;
    int got_frame;
    int (*dec_func)(AVCodecContext *, AVFrame *, int *, const AVPacket *);
    av_register_all();
    avfilter_register_all();
    if ((ret = open_input_file("/Users/devyk/Data/Project/piaoquan/PQMedia/temp/2.mp4")) < 0) {
//      goto end;
        return 0;
    }
    if ((ret = open_output_file("/Users/devyk/Data/Project/piaoquan/PQMedia/temp/transcode-temp-.m3u8")) < 0) {
//      goto end;
        return 0;
    }

    aacbsfc = av_bitstream_filter_init("aac_adtstoasc");
    */
/* read all packets *//*


    int flag = 1;

    while (1) {
        if ((ret = av_read_frame(ifmt_ctx, &packet)) < 0) break;
        stream_index = packet.stream_index;
        type = ifmt_ctx->streams[packet.stream_index]->codec->codec_type;
        av_log(NULL, AV_LOG_DEBUG, "Demuxer gave frame of stream_index %u\n",
               stream_index);

        av_log(NULL, AV_LOG_DEBUG, "Going to reencode&filter the frame\n");
        frame = av_frame_alloc();
        if (!frame) {
            ret = AVERROR(ENOMEM);
            break;
        }
        av_packet_rescale_ts(&packet, ifmt_ctx->streams[stream_index]->time_base,
                             ifmt_ctx->streams[stream_index]->codec->time_base);
        dec_func = (type == AVMEDIA_TYPE_VIDEO) ? avcodec_decode_video2
                                                : avcodec_decode_audio4;
        ret = dec_func(ifmt_ctx->streams[stream_index]->codec, frame, &got_frame,
                       &packet);
        if (ret < 0) {
            av_frame_free(&frame);
            av_log(NULL, AV_LOG_ERROR, "Decoding failed\n");
            break;
        }
        if (got_frame) {
            frame->pts = frame->pkt_pts;
            if (type == AVMEDIA_TYPE_VIDEO) {
                // ret = encode_write_frame(frame, stream_index, NULL);
            } else {
                if (flag) {
                    initSwr(stream_index);
                    flag = 0;
                }

                AVFrame *frame_out = av_frame_alloc();
                if (0 != TransSample(frame, frame_out, stream_index)) {
                    av_log(NULL, AV_LOG_ERROR, "convert audio failed\n");
                    ret = -1;
                }
                ret = encode_write_frame(frame_out, stream_index, NULL);
                av_frame_free(&frame_out);
            }
            av_frame_free(&frame);
            if (ret < 0)
                break;
//                goto end;
        } else {
            av_frame_free(&frame);
        }
        av_free_packet(&packet);
    }
    */
/* flush  encoders *//*

    // for (i = 0; i < ifmt_ctx->nb_streams; i++) {
    // ret = flush_encoder(i);
    // if (ret < 0) {
    // av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
    // goto end;
    // }
    // }
    av_log(NULL, AV_LOG_ERROR, "Flushing encoder failed\n");
    av_write_trailer(ofmt_ctx);

//end:
    av_write_trailer(ofmt_ctx);
    av_free_packet(&packet);
    if (frame) av_frame_free(&frame);
    av_bitstream_filter_close(aacbsfc);
    for (i = 0; i < ifmt_ctx->nb_streams; i++) {
        avcodec_close(ifmt_ctx->streams[i]->codec);
        if (ofmt_ctx && ofmt_ctx->nb_streams > i && ofmt_ctx->streams[i] &&
            ofmt_ctx->streams[i]->codec)
            avcodec_close(ofmt_ctx->streams[i]->codec);

        avformat_close_input(&ifmt_ctx);

        if (ofmt_ctx && !(ofmt_ctx->oformat->flags & AVFMT_NOFILE))
            avio_closep(&ofmt_ctx->pb);
        avformat_free_context(ofmt_ctx);

        return ret ? 1 : 0;
    }
}*/
