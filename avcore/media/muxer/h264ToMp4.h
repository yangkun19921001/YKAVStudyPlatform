//
// Created by 阳坤 on 2021/6/29.
//

#ifndef YKAVSTUDYPLATFORM_H264TOMP4_H
#define YKAVSTUDYPLATFORM_H264TOMP4_H


#include <stdio.h>
extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
AVStream *add_stream(AVFormatContext *oc, AVCodec **codec, enum AVCodecID codec_id);
void open_video(AVFormatContext *oc, AVCodec *codec, AVStream *st);
int CreateMp4(const char* filename);
void WriteVideo(void* data, int nLen,AVStream* avRational);
void CloseMp4();


#endif //YKAVSTUDYPLATFORM_H264TOMP4_H
