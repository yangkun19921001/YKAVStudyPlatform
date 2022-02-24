//
// Created by 阳坤 on 2021/11/29.
//

#ifndef YKAVSTUDYPLATFORM_FF_TOOLS_H
#define YKAVSTUDYPLATFORM_FF_TOOLS_H

#include <inttypes.h>
#include <math.h>
#include <limits.h>
#include <signal.h>
#include <stdint.h>

#include "libavutil/avstring.h"
#include "libavutil/eval.h"
#include "libavutil/mathematics.h"
#include "libavutil/pixdesc.h"
#include "libavutil/imgutils.h"
#include "libavutil/dict.h"
#include "libavutil/parseutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/avassert.h"
#include "libavutil/time.h"
#include "libavformat/avformat.h"
#include "libavdevice/avdevice.h"
#include "libswscale/swscale.h"
#include "libavutil/opt.h"
#include "libavcodec/avfft.h"
#include "libswresample/swresample.h"

# include "libavfilter/avfilter.h"
# include "libavfilter/buffersink.h"
# include "libavfilter/buffersrc.h"
#include "ff_player.h"


#include <SDL.h>
#include <SDL_thread.h>

#include <assert.h>
#include <libavutil/fifo.h>

#include "ff_error.h"
#include "ff_utils.h"
#include "ff_options.h"

#define VIDEO_PICTURE_QUEUE_SIZE    3       // 图像帧缓存数量
#define SUBPICTURE_QUEUE_SIZE        16      // 字幕帧缓存数量
#define SAMPLE_QUEUE_SIZE           9       // 采样帧缓存数量
#define FRAME_QUEUE_SIZE FFMAX(SAMPLE_QUEUE_SIZE, FFMAX(VIDEO_PICTURE_QUEUE_SIZE, SUBPICTURE_QUEUE_SIZE))


#define SAMPLE_ARRAY_SIZE (8 * 65536)

typedef struct MyAVPacketList {
    AVPacket pkt; //解封装后的数据
    struct MyAVPacketList *next;  //下一个节点
    int serial;//播放序列
} MyAVPacketList;


typedef struct PacketQueue {
    MyAVPacketList *first_pkt, *last_pkt;  // 队首，队尾指针
    int nb_packets; // 包数量，也就是队列元素数量
    int size;       // 队列所有元素的数据大小总和
    int64_t duration;// 队列所有元素的数据播放持续时间
    int abort_request;// 用户退出请求标志
    int serial; // 播放序列号，和MyAVPacketList的serial作用相同，但改变的时序稍微有点不同
    SDL_mutex *mutex;// 用于维持PacketQueue的多线程安全(SDL_mutex可以按pthread_mutex_t理解）
    SDL_cond *cond;// 用于维持PacketQueue的多线程安全(SDL_mutex可以按pthread_mutex_t理解）
} PacketQueue;

typedef struct AudioParams {
    int freq;                   // 采样率
    int channels;               // 通道数
    int64_t channel_layout;         // 通道布局，比如2.1声道，5.1声道等
    enum AVSampleFormat fmt;            // 音频采样格式，比如AV_SAMPLE_FMT_S16表示为有符号16bit深度，交错排列模式。
    int frame_size;             // 一个采样单元占用的字节数（比如2通道时，则左右通道各采样一次合成一个采样单元）
    int bytes_per_sec;          // 一秒时间的字节数，比如采样率48Khz，2 channel，16bit，则一秒48000*2*16/8=192000
} AudioParams;

// 这里讲的系统时钟 是通过av_gettime_relative()获取到的时钟，单位为微妙
typedef struct Clock {
    double pts;            // 时钟基础, 当前帧(待播放)显示时间戳，播放后，当前帧变成上一帧
    // 当前pts与当前系统时钟的差值, audio、video对于该值是独立的
    double pts_drift;      // clock base minus time at which we updated the clock
    // 当前时钟(如视频时钟)最后一次更新时间，也可称当前时钟时间
    double last_updated;   // 最后一次更新的系统时钟
    double speed;          // 时钟速度控制，用于控制播放速度
    // 播放序列，所谓播放序列就是一段连续的播放动作，一个seek操作会启动一段新的播放序列
    int serial;             // clock is based on a packet with this serial
    int paused;             // = 1 说明是暂停状态
    // 指向packet_serial
    int *queue_serial;      /* pointer to the current packet queue serial, used for obsolete clock detection */
} Clock;

// 用于缓存解码后的数据
typedef struct Frame {
    AVFrame *frame;         // 指向数据帧
    AVSubtitle sub;            // 用于字幕
    int serial;             // 帧序列，在seek的操作时serial会变化
    double pts;            // 时间戳，单位为秒
    double duration;       // 该帧持续时间，单位为秒
    int64_t pos;            // 该帧在输入文件中的字节位置
    int width;              // 图像宽度
    int height;             // 图像高读
    int format;             // 对于图像为(enum AVPixelFormat)，
    // 对于声音则为(enum AVSampleFormat)
    AVRational sar;            // 图像的宽高比（16:9，4:3...），如果未知或未指定则为0/1
    int uploaded;           // 用来记录该帧是否已经显示过？
    int flip_v;             // =1则垂直翻转， = 0则正常播放
} Frame;

/* 这是一个循环队列，windex是指其中的首元素，rindex是指其中的尾部元素. */
typedef struct FrameQueue {
    Frame queue[FRAME_QUEUE_SIZE];        // FRAME_QUEUE_SIZE  最大size, 数字太大时会占用大量的内存，需要注意该值的设置
    int rindex;                         // 读索引。待播放时读取此帧进行播放，播放后此帧成为上一帧
    int windex;                         // 写索引
    int size;                           // 当前总帧数
    int max_size;                       // 可存储最大帧数
    int keep_last;                      // = 1说明要在队列里面保持最后一帧的数据不释放，只在销毁队列的时候才将其真正释放
    int rindex_shown;                   // 初始化为0，配合keep_last=1使用
    SDL_mutex *mutex;                     // 互斥量
    SDL_cond *cond;                      // 条件变量
    PacketQueue *pktq;                      // 数据包缓冲队列
} FrameQueue;

/**
 *音视频同步方式，缺省以音频为基准
 */
enum {
    AV_SYNC_AUDIO_MASTER,                   // 以音频为基准
    AV_SYNC_VIDEO_MASTER,                   // 以视频为基准
    AV_SYNC_EXTERNAL_CLOCK,                 // 以外部时钟为基准，synchronize to an external clock */
};

/**
 * 解码器封装
 */
typedef struct Decoder {
    AVPacket *pkt;              //
    PacketQueue *queue;         // 数据包队列
    AVCodecContext *avctx;     // 解码器上下文
    int pkt_serial;         // 包序列
    int finished;           // =0，解码器处于工作状态；=非0，解码器处于空闲状态
    int packet_pending;     // =0，解码器处于异常状态，需要考虑重置解码器；=1，解码器处于正常状态
    SDL_cond *empty_queue_cond;  // 检查到packet队列空时发送 signal缓存read_thread读取数据
    int64_t start_pts;          // 初始化时是stream的start time
    AVRational start_pts_tb;       // 初始化时是stream的time_base
    int64_t next_pts;           // 记录最近一次解码后的frame的pts，当解出来的部分帧没有有效的pts时则使用next_pts进行推算
    AVRational next_pts_tb;        // next_pts的单位
    SDL_Thread *decoder_tid;       // 线程句柄
} Decoder;

typedef struct VideoState {
    SDL_Thread *read_tid;      // 读线程句柄
    AVInputFormat *iformat;   // 指向demuxer
    int abort_request;      // =1时请求退出播放
    int force_refresh;      // =1时需要刷新画面，请求立即刷新画面的意思
    int paused;             // =1时暂停，=0时播放
    int last_paused;        // 暂存“暂停”/“播放”状态
    int queue_attachments_req;
    int seek_req;           // 标识一次seek请求
    int seek_flags;         // seek标志，诸如AVSEEK_FLAG_BYTE等
    int64_t seek_pos;       // 请求seek的目标位置(当前位置+增量)
    int64_t seek_rel;       // 本次seek的位置增量
    int read_pause_return;
    AVFormatContext *ic;        // iformat的上下文
    int realtime;           // =1为实时流

    Clock audclk;             // 音频时钟
    Clock vidclk;             // 视频时钟
    Clock extclk;             // 外部时钟

    FrameQueue pictq;          // 视频Frame队列
    FrameQueue subpq;          // 字幕Frame队列
    FrameQueue sampq;          // 采样Frame队列

    Decoder auddec;             // 音频解码器
    Decoder viddec;             // 视频解码器
    Decoder subdec;             // 字幕解码器

    int audio_stream;          // 音频流索引

    int av_sync_type;           // 音视频同步类型, 默认audio master

    double audio_clock;            // 当前音频帧的PTS+当前帧Duration
    int audio_clock_serial;     // 播放序列，seek可改变此值
    // 以下4个参数 非audio master同步方式使用
    double audio_diff_cum;         // used for AV difference average computation
    double audio_diff_avg_coef;
    double audio_diff_threshold;
    int audio_diff_avg_count;
    // end

    AVStream *audio_st;              // 音频流
    PacketQueue audioq;                 // 音频packet队列
    int audio_hw_buf_size;          // SDL音频缓冲区的大小(字节为单位)
    // 指向待播放的一帧音频数据，指向的数据区将被拷入SDL音频缓冲区。若经过重采样则指向audio_buf1，
    // 否则指向frame中的音频
    uint8_t *audio_buf;             // 指向需要重采样的数据
    uint8_t *audio_buf1;            // 指向重采样后的数据
    unsigned int audio_buf_size;     // 待播放的一帧音频数据(audio_buf指向)的大小
    unsigned int audio_buf1_size;    // 申请到的音频缓冲区audio_buf1的实际尺寸
    int audio_buf_index;            // 更新拷贝位置 当前音频帧中已拷入SDL音频缓冲区
    // 的位置索引(指向第一个待拷贝字节)
    // 当前音频帧中尚未拷入SDL音频缓冲区的数据量:
    // audio_buf_size = audio_buf_index + audio_write_buf_size
    int audio_write_buf_size;
    int audio_volume;               // 音量
    int muted;                      // =1静音，=0则正常
    struct AudioParams audio_src;           // 音频frame的参数
#if CONFIG_AVFILTER
    struct AudioParams audio_filter_src;
#endif
    struct AudioParams audio_tgt;       // SDL支持的音频参数，重采样转换：audio_src->audio_tgt
    struct SwrContext *swr_ctx;         // 音频重采样context
    int frame_drops_early;              // 丢弃视频packet计数
    int frame_drops_late;               // 丢弃视频frame计数

    enum ShowMode {
        SHOW_MODE_NONE = -1,    // 无显示
        SHOW_MODE_VIDEO = 0,    // 显示视频
        SHOW_MODE_WAVES,        // 显示波浪，音频
        SHOW_MODE_RDFT,         // 自适应滤波器
        SHOW_MODE_NB
    } show_mode;

    // 音频波形显示使用
    int16_t sample_array[SAMPLE_ARRAY_SIZE];    // 采样数组
    int sample_array_index;                     // 采样索引
    int last_i_start;                           // 上一开始
    RDFTContext *rdft;                          // 自适应滤波器上下文
    int rdft_bits;                              // 自使用比特率
    FFTSample *rdft_data;                       // 快速傅里叶采样

    int xpos;
    double last_vis_time;
    SDL_Texture *vis_texture;       // 音频Texture

    SDL_Texture *sub_texture;       // 字幕显示
    SDL_Texture *vid_texture;       // 视频显示

    int subtitle_stream;            // 字幕流索引
    AVStream *subtitle_st;          // 字幕流
    PacketQueue subtitleq;          // 字幕packet队列

    double frame_timer;             // 记录最后一帧播放的时刻
    double frame_last_returned_time;    // 上一次返回时间
    double frame_last_filter_delay;     // 上一个过滤器延时

    int video_stream;               // 视频流索引
    AVStream *video_st;             // 视频流
    PacketQueue videoq;             // 视频队列
    double max_frame_duration;      // 一帧最大间隔. above this, we consider the jump a timestamp discontinuity
    struct SwsContext *img_convert_ctx; // 视频尺寸格式变换
    struct SwsContext *sub_convert_ctx; // 字幕尺寸格式变换
    int eof;            // 是否读取结束

    char *filename;     // 文件名
    int width, height, xleft, ytop; // 宽、高，x起始坐标，y起始坐标
    int step;           // =1 步进播放模式, =0 其他模式

#if CONFIG_AVFILTER
    int vfilter_idx;
    AVFilterContext *in_video_filter;   // the first filter in the video chain
    AVFilterContext *out_video_filter;  // the last filter in the video chain
    AVFilterContext *in_audio_filter;   // the first filter in the audio chain
    AVFilterContext *out_audio_filter;  // the last filter in the audio chain
    AVFilterGraph *agraph;              // audio filter graph
#endif
    // 保留最近的相应audio、video、subtitle流的steam index
    int last_video_stream, last_audio_stream, last_subtitle_stream;

    SDL_cond *continue_read_thread; // 当读取数据队列满了后进入休眠时，可以通过该condition唤醒读线程
} VideoState;


typedef struct FFplayer{
    VideoState *is;//视频状态维护
    const char *filename;//播放地址
    AVInputFormat *iformat;//输入格式
};

/* options specified by the user */
static const char *input_filename;
static const char *window_title;
static int startup_volume = 20;  // 起始音量
extern AVDictionary *format_opts, *codec_opts, *resample_opts;
static int genpts = 0;
static int find_stream_info = 1;
static int64_t start_time = AV_NOPTS_VALUE;
static int seek_by_bytes = -1;
static int show_status = 1;
static const char* wanted_stream_spec[AVMEDIA_TYPE_NB] = {0};
static int audio_disable;
static int video_disable;
static int subtitle_disable;
static int av_sync_type = AV_SYNC_AUDIO_MASTER;
static enum ShowMode show_mode = SHOW_MODE_NONE;
static int default_width  = 640;
static int default_height = 480;
static int screen_width  = 0;
static int screen_height = 0;

#endif //YKAVSTUDYPLATFORM_FF_TOOLS_H
