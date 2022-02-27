//
// Created by 阳坤 on 2022/2/20.
//


#ifndef YKAVSTUDYPLATFORM_FF_DECODER_H
#define YKAVSTUDYPLATFORM_FF_DECODER_H

#include "ff_tools.h"

/**
 * 解码器初始化
 * @param is
 * @return
 */
int ff_decoder_init(struct VideoState *is);

/**
 *  解码器参数初始化
 * @param d
 * @param avctx
 * @param queue
 * @param empty_queue_cond
 */
void ff_decoder_parameters_init(Decoder *d, AVCodecContext *avctx, PacketQueue *queue, SDL_cond *empty_queue_cond);

/**
 * open a given stream. Return 0 if OK
 * @brief stream_component_open
 * @param is
 * @param stream_index 流索引
 * @return Return 0 if OK
 */
int ff_stream_component_open(struct VideoState *is,int stream_index);

/**
 * 创建解码线程, audio/video有独立的线程
 */
int ff_decoder_start(Decoder *d, int (*fn)(void *), const char *thread_name, void* arg);

#endif //YKAVSTUDYPLATFORM_FF_DECODER_H
