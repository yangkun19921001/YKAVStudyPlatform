//
// Created by 阳坤 on 2022/2/22.
//

#ifndef YKAVSTUDYPLATFORM_FF_PLAYER_H
#define YKAVSTUDYPLATFORM_FF_PLAYER_H

#include "ff_tools.h"
#include "ff_frame_queue.h"
#include "ff_packet_queue.h"
#include "ff_demuxer.h"
#include "ff_decoder.h"
#include "ff_av_clock.h"

/**
 * 创建一个播放器
 * @return
 */
struct FFplayer *ff_player_create();

/**
 * 释放一个播放器
 * @param player
 */
void ff_player_free(struct FFplayer *player);

/**
 * 做一些准备工作
 * @param player
 * @return
 */
int ff_player_init(struct FFplayer *player);


#endif //YKAVSTUDYPLATFORM_FF_PLAYER_H
