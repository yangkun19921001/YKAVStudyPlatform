//
// Created by 阳坤 on 2021/6/30.
//

#ifndef YKAVSTUDYPLATFORM_PQMUXER_H
#define YKAVSTUDYPLATFORM_PQMUXER_H

#define ERROR 0
#define SUCCESS 1
#define VTEMPO "VTempo" //视频速率
#define STARTUS "start" //开始时间戳
#define ENDUS "end" //结束时间戳

#include <list>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}

using namespace std;
typedef struct TempoValue {
    double vTempo;//视频速率
    int64_t start;//开始时间戳-微妙
    int64_t end;//结束时间戳-微妙
};

typedef list<TempoValue> TEMPO_LIST;

/**
 * @anchor yangkun
 * h264 or h265 or other video encode format  muxer to mp4
 */
class FFmpegMuxer {

public:
    FFmpegMuxer();

    ~ FFmpegMuxer();

public:
    /**
     * 初始化 ffmpeg
     * @param inpath 输入文件路径，可以以 file 也可以是网络流
     * @param outpath
     * @return 1 is ok
     */
    virtual int InitFFmpeg(const char *inpath, const char *outpath);

    /**
     * 开始封装
     * @param vTempo 视频倍数 0~ ？小于 1 慢速 ， 大于 1 快速
     * @param aTempo todo 预留字段 音频倍数 0~ ？小于 1 慢速 ， 大于 1 快速
     * @return 1 is ok
     */
    virtual int StartMuxer(double aTempo, double vTempo, TEMPO_LIST tempoList);

    /**
    * 释放 ffmpeg 解封装
    * @return
    */
    virtual int CloseMuxer();

protected:
    //输出格式
    AVOutputFormat *ofmt = NULL;
    //输入格式上下文
    AVFormatContext *ifmt_ctx_v = NULL;
    //输出格式上下文
    AVFormatContext *ofmt_ctx = NULL;
    //读取出来的压缩数据包
    AVPacket *pkt = NULL;
    //视频输入输出轨道索引
    int videoindex_v = -1, videoindex_out = -1, ret = -1;
    //设置视频速率
    double videoTempo;
    //设置音频速率 todo 预留字段
    double audioTempo;
};

#endif //YKAVSTUDYPLATFORM_PQMUXER_H
