//
// Created by —Ù¿§ on 2021/8/10.
//


#include <cstdio>
#include "encode_aac.h"


#include <stdio.h>

#define __STDC_CONSTANT_MACROS

#ifdef _WIN32
//Windows
extern "C"
{
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
};
#else
//Linux...
#ifdef __cplusplus
extern "C"
{
#endif
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#ifdef __cplusplus
};
#endif
#endif


int main() {

    FILE *in_file = NULL;                            //Raw PCM data
    int framenum = 100000;                          //Audio frame number
    int i;
    auto *audio = new AACEncoder();
    audio->EncodeStart();

    u_int8_t *frame_buf;
    int size = 4096*2;
    frame_buf = (uint8_t *) av_malloc(size);
    in_file = fopen("/Users/devyk/Data/Project/piaoquan/PQMedia/temp/4eef8efd217fb040fbf4b86f3a5fe465.pcm", "rb");
    for (i = 0; i < framenum; i++) {
        //Read PCM
        if (fread(frame_buf, 1, size, in_file) <= 0) {
            printf("Failed to read raw data! \n");
            return -1;
        } else if (feof(in_file)) {
            printf("end of! \n");
            break;
        }
        auto *data = (uint8_t *) malloc(sizeof(uint8_t *) * size);
        memcpy(data, frame_buf, size);
        audio->EncodeBuffer(data, size);
    }
    audio->EncodeStop();
    return 1;
}

