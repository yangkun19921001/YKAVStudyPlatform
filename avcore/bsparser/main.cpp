//
// Created by 阳坤 on 2021/6/9.
//

#include "NalParse.h"

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>



int main(int argc, char *argv[]) {
    FILE *outFile;

    if (argc < 2) {
        printf("请输入文件路径!");
        return EXIT_FAILURE;
    }
    CNalParser *parser = new CNalParser();
    long start = parser->getCurSystemTime();
    char *inPath = argv[1];
    char *outPath = argv[2];
    printf("inpath:%s outpath:%s \n", inPath, outPath);

    outFile = fopen(outPath, "w+");

    parser->init(inPath);
    vector<NALU_t> nalu;
    parser->probeNALU(nalu, 1000000);
    for (int i = 0; i < nalu.size(); ++i) {
        NALU_t item = nalu[i];
        printf("序号=%d , 文件偏移=%d , 帧类型=%d , NaluType=%d ,起始码长度=%d ,编码格式=%s \n", item.num, item.offset, item.sliceType,
               item.nalType, item.startcodeLen, item.type == 0 ? "H.264" : "H.265"
        );
        fprintf(outFile, "序号=%d , 文件偏移=%d , 帧类型=%d , NaluType=%d ,起始码长度=%d ,编码格式=%s \n", item.num, item.offset,
                item.sliceType,
                item.nalType, item.startcodeLen, item.type == 0 ? "H.264" : "H.265");
    }
    parser->release();
    if (outFile){
        fclose(outFile);
        outFile = NULL;
    }
    long cost_time = parser->getCurSystemTime() - start;
    printf("cost time %lld \n", cost_time);
    return 1;
}
