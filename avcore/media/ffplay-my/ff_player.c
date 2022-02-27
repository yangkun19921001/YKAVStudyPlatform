//
// Created by 阳坤 on 2022/2/22.
//

#include "ff_player.h"


struct FFplayer *ff_player_create() {
    VideoState *is = NULL;
    struct FFplayer *fFplayer = NULL;
    if (!(is = (av_malloc(sizeof(VideoState)))))
        return NULL;
    memset(is, 0, sizeof(VideoState));
    if (!(fFplayer = (av_malloc(sizeof(struct FFplayer)))))
        return NULL;
    memset(fFplayer, 0, sizeof(struct FFplayer));
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
    is->av_sync_type = av_sync_type;

    //解封装初始化
    int ret = ff_demuxer_init(is);
    if (ret != 0)
        goto fail;
    ret = ff_decoder_init(is);
    if (ret < 0)
        goto fail;
    ff_demuxer_start(is);
    fail:
    ff_demuxer_close(is);
    ff_player_free(player);
    return 0;
}

int ff_player_start(long play_id) {
    int ret;
    if (play_id > 0) {
        FFplayer *player = (struct FFplayer *) play_id;
        ret =  ff_demuxer_start(player->is);
    }
    return 0;
}

void set_default_window_size(int width, int height, AVRational sar) {
    SDL_Rect rect;
    int max_width = screen_width ? screen_width : INT_MAX;
    int max_height = screen_height ? screen_height : INT_MAX;
    if (max_width == INT_MAX && max_height == INT_MAX)
        max_height = height;
    calculate_display_rect(&rect, 0, 0, max_width, max_height, width, height, sar);
    default_width = rect.w;
    default_height = rect.h;
}

void calculate_display_rect(SDL_Rect *rect,
                            int scr_xleft, int scr_ytop, int scr_width, int scr_height,
                            int pic_width, int pic_height, AVRational pic_sar) {
    AVRational aspect_ratio = pic_sar; // 比率
    int64_t width, height, x, y;

    if (av_cmp_q(aspect_ratio, av_make_q(0, 1)) <= 0)
        aspect_ratio = av_make_q(1, 1);// 如果aspect_ratio是负数或者为0,设置为1:1
    // 转成真正的播放比例
    aspect_ratio = av_mul_q(aspect_ratio, av_make_q(pic_width, pic_height));

    /* XXX: we suppose the screen has a 1.0 pixel ratio */
    // 计算显示视频帧区域的宽高
    // 先以高度为基准
    height = scr_height;
    // &~1, 取偶数宽度  1110
    width = av_rescale(height, aspect_ratio.num, aspect_ratio.den) & ~1;
    if (width > scr_width) {
        // 当以高度为基准,发现计算出来的需要的窗口宽度不足时调整为以窗口宽度为基准
        width = scr_width;
        height = av_rescale(width, aspect_ratio.den, aspect_ratio.num) & ~1;
    }
    // 计算显示视频帧区域的起始坐标（在显示窗口内部的区域）
    x = (scr_width - width) / 2;
    y = (scr_height - height) / 2;
    rect->x = scr_xleft + x;
    rect->y = scr_ytop + y;
    rect->w = FFMAX((int) width, 1);
    rect->h = FFMAX((int) height, 1);
}
