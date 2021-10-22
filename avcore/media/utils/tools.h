//
// Created by 阳坤 on 2021/8/18.
//

#ifndef PQMEDIA_TOOLS_H
#define PQMEDIA_TOOLS_H


#define DEBUG 0
#include <sys/time.h>

#

#define MAX(a, b)  (((a) > (b)) ? (a) : (b))
#define MIN(a, b)  (((a) < (b)) ? (a) : (b))


static inline long getCurrentTimeMills() {
    struct timeval tv;
    gettimeofday(&tv,NULL);
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
}


#endif //PQMEDIA_TOOLS_H
