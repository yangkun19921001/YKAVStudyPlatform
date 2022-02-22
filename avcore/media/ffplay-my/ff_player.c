//
// Created by 阳坤 on 2022/2/22.
//

#include "ff_player.h"


struct FFplayer *ff_player_create() {
    VideoState *is = NULL;
    struct FFplayer *fFplayer = NULL;
    if (!(is = (av_malloc(sizeof(VideoState)))))
        return NULL;
    memset(is, 0, sizeof(sizeof(VideoState)));
    if (!(fFplayer = (av_malloc(sizeof(struct FFplayer)))))
        return NULL;
    memset(fFplayer, 0, sizeof(sizeof(struct FFplayer)));
    fFplayer->is = is;
    return fFplayer;
}

void ff_player_free(struct FFplayer *player) {
    if (!player)return;
    //todo



    if (player->is) {
        av_free(player->is);
        player->is = NULL;
    }
    if (player->filename) {
        av_free((void *) player->filename);
        player->filename = NULL;
    }

    av_free(player);
    player = NULL;

}

int ff_player_init(struct FFplayer *player) {
    if (!player || !player->is || !player->filename)return AVERROR(EOBJNULL);

    //初始化帧队列
    VideoState *is = player->is;
    /* 初始化帧队列 */
    if (ff_frame_queue_init(&is->pictq, &is->videoq, VIDEO_PICTURE_QUEUE_SIZE, 1) < 0)
        goto fail;
    if (ff_frame_queue_init(&is->subpq, &is->subtitleq, SUBPICTURE_QUEUE_SIZE, 0) < 0)
        goto fail;
    if (ff_frame_queue_init(&is->sampq, &is->audioq, SAMPLE_QUEUE_SIZE, 1) < 0)
        goto fail;

    if (ff_packet_queue_init(&is->videoq) < 0 ||
        ff_packet_queue_init(&is->audioq) < 0 ||
        ff_packet_queue_init(&is->subtitleq) < 0)
        goto fail;

    //创建解封装读取帧唤醒条件变量
    if (!(is->continue_read_thread = SDL_CreateCond())) {
        av_log(NULL, AV_LOG_FATAL, "SDL_CreateCond(): %s\n", SDL_GetError());
        goto fail;
    }

    //todo 初始化音视频同步时钟

    //初始化音频音量
    if (startup_volume < 0)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d < 0, setting to 0\n", startup_volume);
    if (startup_volume > 100)
        av_log(NULL, AV_LOG_WARNING, "-volume=%d > 100, setting to 100\n", startup_volume);


    startup_volume = av_clip(startup_volume, 0, 100);
    startup_volume = av_clip(SDL_MIX_MAXVOLUME * startup_volume / 100, 0, SDL_MIX_MAXVOLUME);
    is->audio_volume = startup_volume;
    is->muted = 0;
    //todo 同步类型，默认以音频为基准
//    is->av_sync_type = av_sync_type;

    //解封装初始化
    int ret = ff_demuxer_init(is);
    if (ret != 0)
        goto fail;
    ff_demuxer_start(is)
    fail:
    ff_demuxer_close(is);
    ff_player_free(player);
    return 0;
}
