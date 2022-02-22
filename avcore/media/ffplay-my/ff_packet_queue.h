//
// Created by 阳坤 on 2021/11/29.
//

#ifndef YKAVSTUDYPLATFORM_FF_PACKET_QUEUE_H
#define YKAVSTUDYPLATFORM_FF_PACKET_QUEUE_H

#include "ff_tools.h"

static AVPacket flush_pkt;

/**
 * 初始化各个字段的值
 * @param q
 * @return
 */
int ff_packet_queue_init(PacketQueue *q);

/**
 * 将已经存在的节点清除
 * @param q
 */
void ff_packet_queue_flush(PacketQueue *q);

/**
 * 消息队列，释放内存
 * @param q
 */
void ff_packet_queue_destory(PacketQueue *q);

/**
 * 中止队列
 * @param q
 */
void ff_packet_queue_abort(PacketQueue *q);

/**
 * 启动队列
 * @param q
 */
void ff_packet_queue_start(PacketQueue *q);

/**
 * 从队列中取一个节点
 * @param q
 * @param pkt
 * @param block
 * @param serial
 * @return
 */
int ff_packet_queue_get(PacketQueue *q, AVPacket *pkt, int block, int *serial);

/**
 * 存放一个节点
 * @param q
 * @param pkt
 * @return
 */
int ff_packet_queue_put(PacketQueue *q, AVPacket *pkt);

/**
 * 放入一个空包
 * @param q
 * @param starat_index
 * @return
 */
int ff_packet_queue_put_nullpacket(PacketQueue *q, int starat_index);

/**
 * 存节点
 * @param q
 * @param pkt
 * @return
 */
int ff_packet_queue_put_private(PacketQueue *q, AVPacket *pkt);

#endif //YKAVSTUDYPLATFORM_FF_PACKET_QUEUE_H
