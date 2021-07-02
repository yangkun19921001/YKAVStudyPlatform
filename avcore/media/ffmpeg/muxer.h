//
// Created by 阳坤 on 2021/6/29.
//

#ifndef YKAVSTUDYPLATFORM_MUXER_H
#define YKAVSTUDYPLATFORM_MUXER_H


#include <iostream>
#include <thread>
#include <string>
#include <mutex>

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavutil/avutil.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
}
using namespace std;

static double r2d(AVRational r) {
    return r.den == 0 ? 0 : (double) r.num / (double) r.den;
}


// 用于存储流数据的队列项结构
typedef struct _CE_QUEUE_ITEM_T_ {
    // 申请的缓冲区指针
    uint8_t *Bytes;
    // 申请的缓冲区大小
    int TotalSize;
    // 入队填充的数据字节数
    int EnqueueSize;
    // 出队读取的数据字节数
    int DequeueSize;
} CEQueueItem;

// 一个简单的队列结构, 用于暂存流数据
typedef struct _CE_QUEUE_T_ {
    // 队列项指针
    CEQueueItem *Items;
    // 队列的标识, 使用者可以自定义标识进行队列区分
    uint8_t Flag;
    // 队列项个数
    int Count;
    // 出队计数索引
    int DequeueIndex;
    // 入队索引
    int EnqueueIndex;
} CESimpleQueue;

class MP4Muxer {
public:
    MP4Muxer();

    ~MP4Muxer();

    /*
    * Comments: 混流器初始化 实例化后首先调用该方法进行内部参数的初始化
    * Param aFileName: 要保存的文件名
    * @Return 是否成功
    */
    bool Start(std::string aFileName);

    /*
    * Comments: 追加一帧AAC数据到混流器
    * Param aBytes: 要追加的字节数据
    * Param aSize: 追加的字节数
    * @Return 成功与否
    */
    bool AppendAudio(uint8_t *aBytes, int aSize);

    /*
    * Comments: 追加一帧H264视频数据到混流器
    * Param aBytes: 要追加的字节数据
    * Param aSize: 追加的字节数
    * @Return 成功与否
    */
    bool AppendVideo(uint8_t *aBytes, int aSize);

    /*
    * Comments: 关闭并释放输出格式上下文
    * Param : None
    * @Return None
    */
    void Stop(void);

    /*
    * Comments: 设置音视频的播放速率
    * Param : aTempo 音频速率
    * Param : vTempo 视频速率
    * @Return None
    */
    void setAVTempo(double aTempo=1.0, double vTempo=1.0);

private:
    /*
    * Comments: 打开AAC输入上下文
    * Param : None
    * @Return 0成功, 其他失败
    */
    int OpenAudio(bool aNew = false);

    /*
    * Comments: 写入AAC数据帧到文件
    * Param : None
    * @Return void
    */
    void WriteAudioFrame(void);

    /*
    * Comments: 打开H264视频流
    * Param : None
    * @Return 0成功, 其他失败
    */
    int OpenVideo(void);

    /*
    * Comments: 写入H264视频帧到文件
    * Param : None
    * @Return void
    */
    void WriteVideoFrame(double tempo = 1.0);

private:
    // 要保存的MP4文件路径
    std::string mFileName;
    // 输入AAC音频上下文
    AVFormatContext *mInputAudioContext;
    // 输入H264视频上下文
    AVFormatContext *mInputVideoContext;
    // 输出媒体格式上下文
    AVFormatContext *mOutputFrmtContext;
    // AAC音频位流筛选上下文
    AVBitStreamFilterContext *mAudioFilter;
    // H264视频位流筛选上下文
    AVBitStreamFilterContext *mVideoFilter;
    // 流媒体数据包
    AVPacket mPacket;
    // AAC数据缓存区
    uint8_t *mAudioBuffer;
    // AAC数据读写队列
    CESimpleQueue *mAudioQueue;
    // H264数据缓冲区
    uint8_t *mVideoBuffer;
    // H264数据读写队列
    CESimpleQueue *mVideoQueue;
    // AAC的回调读取是否启动成功
    bool mIsStartAudio;
    // H264的回调读取是否启动成功
    bool mIsStartVideo;
    // 音频在mInputAudioContext->streams流中的索引
    int mAudioIndex;
    // 视频在mInputVideoContext->streams流中的索引
    int mVideoIndex;
    // 输出流中音频帧的索引
    int mAudioOutIndex;
    // 输出流视频帧的索引
    int mVideoOutIndex;
    // 输出帧索引计数
    int mFrameIndex;

    // 线程操作互斥锁
    mutex mLock;

    //设置视频速率
    double mVideoTempo;

};

#endif //YKAVSTUDYPLATFORM_MUXER_H
