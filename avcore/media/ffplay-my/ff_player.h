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

void set_default_window_size(int width, int height, AVRational sar);


// 4.2.0 和4.0有区别
/**
 * @brief 将帧宽高按照sar最大适配到窗口
 * @param rect      获取到的显示位置和宽高
 * @param scr_xleft 窗口显示起始x位置,这里说的是内部显示的坐标, 不是窗口在整个屏幕的起始位置
 * @param scr_ytop  窗口显示起始y位置
 * @param scr_width 窗口宽度
 * @param scr_height窗口高度
 * @param pic_width 显示帧宽度
 * @param pic_height显示帧高度
 * @param pic_sar   显示帧宽高比
 */
void calculate_display_rect(SDL_Rect *rect,
                            int scr_xleft, int scr_ytop, int scr_width, int scr_height,
                            int pic_width, int pic_height, AVRational pic_sar);
#endif //YKAVSTUDYPLATFORM_FF_PLAYER_H
