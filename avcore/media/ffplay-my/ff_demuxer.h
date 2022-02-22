//
// Created by 阳坤 on 2022/2/20.
//

#ifndef YKAVSTUDYPLATFORM_FF_DEMUXER_H
#define YKAVSTUDYPLATFORM_FF_DEMUXER_H

#include "ff_tools.h"

/**
 * 解封装初始化
 * @return 返回 视频状态维护
 */
int ff_demuxer_init(VideoState *is);

/**
 * 关闭封装器
 * @param is
 */
void ff_demuxer_close(VideoState *is);

/**
 * 开始解封装
 * @param is
 */
int ff_demuxer_start(VideoState *is);

/**
 * 读取帧线程
 * @param arg
 * @return
 */
static int ff_demuxer_read_thread(void *arg);

#endif //YKAVSTUDYPLATFORM_FF_DEMUXER_H
