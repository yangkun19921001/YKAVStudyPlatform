//
// Created by 阳坤 on 2021/11/29.
//
#include "ff_frame_queue.h"

void ff_frame_queue_unref_item(Frame *pFrame);

int ff_frame_queue_init(FrameQueue *f, PacketQueue *pktq, int max_size, int keep_last) {
    int i;
    memset(f, 0, sizeof(FrameQueue));
    //创建互斥锁
    if (!(f->mutex = SDL_CreateMutex())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateMutex(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    //创建互斥条件变量
    if (!(f->cond = SDL_CreateCond())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        return AVERROR(ENOMEM);
    }
    f->pktq = pktq;
    f->max_size = FFMIN(max_size, FRAME_QUEUE_SIZE);
    f->keep_last = !!keep_last;
    for (i = 0; i < f->max_size; i++)
        if (!(f->queue[i].frame = av_frame_alloc())) // 分配AVFrame结构体
            return AVERROR(ENOMEM);
    return 0;
}

int ff_frame_queue_destory(FrameQueue *f) {
    int i;
    for (int j = 0; j < f->max_size; ++j) {
        Frame *vp = &f->queue[j];
        // 释放对vp->frame中的数据缓冲区的引用，注意不是释放frame对象本身
        ff_frame_queue_unref_item(vp);
        //释放对象
        av_frame_free(&vp->frame);
    }
    SDL_DestroyMutex(f->mutex);
    SDL_DestroyCond(f->cond);
    return 0;
}

void ff_frame_queue_unref_item(Frame *vp) {
    av_frame_unref(vp->frame);    /* 释放数据 */
    avsubtitle_free(&vp->sub);
}

void ff_frame_queue_signal(FrameQueue *f) {
    SDL_LockMutex(f->mutex);
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}


Frame *ff_frame_queue_peek(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

Frame *ff_frame_queue_peek_next(FrameQueue *f) {
    return &f->queue[(f->rindex + f->rindex_shown + 1) % f->max_size];
}

Frame *ff_frrame_queue_peek_last(FrameQueue *f) {
    return &f->queue[f->rindex];
}

Frame *ff_frame_queue_peek_writable(FrameQueue *f) {
    SDL_LockMutex(f->mutex);
    while (f->size >= f->max_size &&
           !f->pktq->abort_request)//检查是否退出
    {
        SDL_CondWait(f->cond, f->mutex);//等待
    }
    SDL_UnlockMutex(f->mutex);
    if (f->pktq->abort_request)return NULL;//退出锁，检查是否要退出
    return &f->queue[f->windex];
}

Frame *ff_frame_queue_peek_readable(FrameQueue *f) {
    SDL_LockMutex(f->mutex);
    while (f->size - f->rindex_shown <= 0 &&
           !f->pktq->abort_request)//检查是否退出
    {
        SDL_CondWait(f->cond, f->mutex);
    }
    SDL_UnlockMutex(f->mutex);
    if (f->pktq->abort_request) return NULL;
    return &f->queue[(f->rindex + f->rindex_shown) % f->max_size];
}

void ff_frame_queue_push(FrameQueue *f) {
    if (++f->windex == f->max_size)
        f->windex = 0;
    SDL_LockMutex(f->mutex);
    f->size++;
    SDL_CondSignal(f->cond);    // 当_readable在等待时则可以唤醒
    SDL_UnlockMutex(f->mutex);
}

void ff_frame_queue_next(FrameQueue *f) {
    if (f->keep_last && !f->rindex_shown) {
        f->rindex_shown = 1; // 第一次进来没有更新，对应的frame就没有释放
        return;
    }
    ff_frame_queue_unref_item(&f->queue[f->rindex]);
    if (++f->rindex == f->max_size)
        f->rindex = 0;
    SDL_LockMutex(f->mutex);
    f->size--;
    SDL_CondSignal(f->cond);
    SDL_UnlockMutex(f->mutex);
}

int ff_frame_queue_nb_remaining(FrameQueue *f) {
    return f->size - f->rindex_shown;	// 注意这里为什么要减去f->rindex_shown
}

int64_t ff_frame_queue_last_pos(FrameQueue *f) {
    Frame *fp = &f->queue[f->rindex];
    if (f->rindex_shown && fp->serial == f->pktq->serial)
        return fp->pos;
    else
        return -1;
}


