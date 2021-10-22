//
// Created by 阳坤 on 2021/7/19.
//

#ifndef QTFFPLAYER_ITHREAD_H
#define QTFFPLAYER_ITHREAD_H

#include <thread>
#include <vector>
#include <chrono>
#include "log4z.h"
#include <mutex>

using namespace zsummer::log4z;
using namespace std;

//sleep 毫秒
void Sleep(int mis);

void *Main(void *p);

/**
 * 统一管理线程
 */
class IThread {
public:
    //启动线程
    virtual bool start();

    //线程释放
    virtual void release();

    //子线程入口
    virtual void *run() = 0;

private:
    pthread_t tid;

    mutex thread_mutex;
};


#endif //QTFFPLAYER_ITHREAD_H
