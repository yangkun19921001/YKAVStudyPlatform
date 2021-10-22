
#include "encode_aac.h"

int AACEncoder::EncodeFrame(AVCodecContext *pCodecCtx, AVFrame *audioFrame) {
    int ret = avcodec_send_frame(pCodecCtx, audioFrame);
    printf("avcodec_send_frame=%d \n",ret);
    if (ret < 0) {
        char buf[1024] = {0};
        av_strerror(ret, buf, sizeof(buf) - 1);
        printf("Could't send frame %s\n",buf);
        return -1;
    }
    while (avcodec_receive_packet(pCodecCtx, &audioPacket) == 0) {
        audioPacket.stream_index = audioStream->index;
//        ret = av_interleaved_write_frame(pFormatCtx, &audioPacket);
        printf("pts=%lld \n",audioPacket.size);
        av_packet_unref(&audioPacket);
    }
    return ret;
}

int AACEncoder::EncodeStart() {
    //1.注册所有组件
    av_register_all();
    //2.获取输出文件的上下文环境
    pFormatCtx = avformat_alloc_context();
    //4.新建音频流
    audioStream = avformat_new_stream(pFormatCtx, NULL);
    if (audioStream == NULL) {
        return -1;
    }
    //5.寻找编码器并打开编码器
    pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if (pCodec == NULL) {
        printf("Could not find encoder");
        return -1;
    }

    //6.分配编码器并设置参数
    pCodecCtx = audioStream->codec;
    pCodecCtx->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    pCodecCtx->codec_id = AV_CODEC_ID_AAC;
    pCodecCtx->codec_type = AVMEDIA_TYPE_AUDIO;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_FLTP;
    pCodecCtx->sample_rate = 44100;
    pCodecCtx->channel_layout = AV_CH_LAYOUT_MONO;
    pCodecCtx->channels = av_get_channel_layout_nb_channels(pCodecCtx->channel_layout);
    pCodecCtx->bit_rate = 128000;

    //7.打开音频编码器
    int result = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (result < 0) {
        printf("Could't open encoder");
        return -1;
    }

    audioFrame = av_frame_alloc();
    audioFrame->nb_samples = pCodecCtx->frame_size;
    audioFrame->format = pCodecCtx->sample_fmt;

    bufferSize = av_samples_get_buffer_size(NULL, pCodecCtx->channels, pCodecCtx->frame_size,
                                            pCodecCtx->sample_fmt, 1);
    audioBuffer = (uint8_t *) av_malloc(bufferSize);
    avcodec_fill_audio_frame(audioFrame, pCodecCtx->channels, pCodecCtx->sample_fmt,
                             (const uint8_t *) audioBuffer, bufferSize, 1);

    av_new_packet(&audioPacket, bufferSize);

    //9.用于音频转码
    swr = swr_alloc();
    av_opt_set_channel_layout(swr, "in_channel_layout", AV_CH_LAYOUT_STEREO, 0);
    av_opt_set_channel_layout(swr, "out_channel_layout", AV_CH_LAYOUT_MONO, 0);
    av_opt_set_int(swr, "in_sample_rate", 44100, 0);
    av_opt_set_int(swr, "out_sample_rate", 44100, 0);
    av_opt_set_sample_fmt(swr, "in_sample_fmt", AV_SAMPLE_FMT_S16, 0);
    av_opt_set_sample_fmt(swr, "out_sample_fmt", AV_SAMPLE_FMT_FLTP, 0);
    swr_init(swr);
    return 0;
}

int AACEncoder::EncodeBuffer(const unsigned char *pcmBuffer, int len) {
//    uint8_t *outs[2];
//    audioFrame->data[0] = new uint8_t[len/2];
//    memcpy(outs[0] ,pcmBuffer,len);
//    outs[1] = new uint8_t[len];
    int count = swr_convert(swr,&audioFrame->data[0], audioFrame->linesize[0], &pcmBuffer, len);
    if (count >= 0) {
        EncodeFrame(pCodecCtx, audioFrame);
    } else {
        char errorMessage[1024] = {0};
        av_strerror(len, errorMessage, sizeof(errorMessage));
        printf("error message: %s", errorMessage);
    }

//    delete outs[0];
//    delete outs[1];
    return 0;
}

int AACEncoder::EncodeStop() {
    EncodeFrame(pCodecCtx, NULL);
    av_free(audioFrame);
    av_free(audioBuffer);
    avformat_free_context(pFormatCtx);
    return 0;
}


