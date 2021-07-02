//
// Created by 阳坤 on 2021/6/21.
//

#ifndef YKAVSTUDYPLATFORM_DECODE_H
#define YKAVSTUDYPLATFORM_DECODE_H

#include <iostream>
#include <thread>

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

/**
 * 睡眠
 * @param ms
 */
void YSleep(long ms) {
    chrono::milliseconds du(ms);
    this_thread::sleep_for(du);
}

#endif //YKAVSTUDYPLATFORM_DECODE_H
