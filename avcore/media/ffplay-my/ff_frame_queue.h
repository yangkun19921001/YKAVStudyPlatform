//
// Created by 阳坤 on 2021/11/29.
//

#ifndef YKAVSTUDYPLATFORM_FF_FRAME_QUEUE_H
#define YKAVSTUDYPLATFORM_FF_FRAME_QUEUE_H

#include "ff_tools.h"

/**
 * 初始化 FrameQueue
 * @param f 原始数据
 * @param pktq 编码数据
 * @param max_size 最大缓存
 * @param keep_last
 * @return
 */
int ff_frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last);

/**
 * 销毁队列中的帧
 * @param f
 * @return
 */
int ff_frame_queue_destory(FrameQueue *f);

/**
 * 释放缓存帧的引用
 * @param vp
 */
void ff_frame_queue_unref_item(Frame *vp);

/**
 * 发送唤醒的信号
 * @param f
 */
void ff_frame_queue_signal(FrameQueue *f);

/**
 * 获取队列当前Frame, 在调用该函数前先调用 ff_frame_queue_nb_remaining 确保有frame可读
 * @param f
 * @return
 */
Frame *ff_frame_queue_peek(FrameQueue *f);

/**
 * 获取当前Frame的下一Frame, 此时要确保queue里面至少有2个Frame
 * @param f
 * @return  不管什么时候调用，返回来肯定不是 NULL
 */
Frame *ff_frame_queue_peek_next(FrameQueue *f);

/**
 * 获取 last frame
 * 当rindex_shown=0时，和frame_queue_peek效果一样
 * 当rindex_shown=1时，读取的是已经显示过的frame
 * @param f
 * @return
 */
Frame *ff_frrame_queue_peek_last(FrameQueue *f);

/**
 * 获取可写指针
 * @param f
 * @return
 */
Frame *ff_frame_queue_peek_writable(FrameQueue *f);

/**
 * 获取可读指针
 * @param f
 * @return
 */
Frame *ff_frame_queue_peek_readable(FrameQueue *f);

/**
 * 更新写指针
 * @param f
 */
void ff_frame_queue_push(FrameQueue *f);

/**
 * 释放当前帧，冰冰更新索引 rindex
 * 当keep_last为1, rindex_show为0时不去更新rindex,也不释放当前frame
 * @param f
 */
void ff_frame_queue_next(FrameQueue *f);

/**
 * 返回队列中为显示的帧数
 * @param f
 * @return
 */
int ff_frame_queue_nb_remaining(FrameQueue *f);

/**
 * 返回最后帧显示的位置
 * @param f
 * @return
 */
int64_t ff_frame_queue_last_pos(FrameQueue *f);

#endif //YKAVSTUDYPLATFORM_FF_FRAME_QUEUE_H
