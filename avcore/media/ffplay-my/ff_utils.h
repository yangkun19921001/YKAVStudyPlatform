//
// Created by 阳坤 on 2022/2/24.
//

#ifndef YKAVSTUDYPLATFORM_FF_UTILS_H
#define YKAVSTUDYPLATFORM_FF_UTILS_H

#include <libavutil/dict.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <SDL_rect.h>

int check_stream_specifier(AVFormatContext *s, AVStream *st, const char *spec);

void exit_program(int ret);

AVDictionary *filter_codec_opts(AVDictionary *opts, enum AVCodecID codec_id,
                                AVFormatContext *s, AVStream *st, const AVCodec *codec);

AVDictionary **setup_find_stream_info_opts(AVFormatContext *s,
                                           AVDictionary *codec_opts);


int is_realtime(AVFormatContext *s);

void print_error(const char *filename, int err);




#endif //YKAVSTUDYPLATFORM_FF_UTILS_H
