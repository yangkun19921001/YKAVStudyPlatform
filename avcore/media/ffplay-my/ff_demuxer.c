//
// Created by 阳坤 on 2022/2/20.
//

#include "ff_demuxer.h"

int ff_demuxer_init(VideoState *is) {


    fail:
    ff_demuxer_close(is);
    return 0;
}

void ff_demuxer_close(VideoState *is) {
    //todo ---

}

int ff_demuxer_start(VideoState *is) {
    if (!is)return AVERROR(EOBJNULL);
    /* 创建读线程 */
    is->read_tid = SDL_CreateThread(ff_demuxer_read_thread, "read_thread", is);
    if (!is->read_tid) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateThread(): %s\n", SDL_GetError());
        fail:
        ff_demuxer_close(is);
        return AVERROR(ECTHREAD);
    }
    return 0;
}

int ff_demuxer_read_thread(void *arg) {
    av_log(NULL, AV_LOG_DEBUG, "ff_demuxer_read_thread() in\n");
    VideoState *is = arg;
    return 0;
}
