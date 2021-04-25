//
// Created by DevYK on 2021/4/21.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
    NALU_TYPE_SLICE = 1,
    NALU_TYPE_DPA = 2,
    NALU_TYPE_DPB = 3,
    NALU_TYPE_DPC = 4,
    NALU_TYPE_IDR = 5,
    NALU_TYPE_SEI = 6,
    NALU_TYPE_SPS = 7,
    NALU_TYPE_PPS = 8,
    NALU_TYPE_AUD = 9,
    NALU_TYPE_EOSEQ = 10,
    NALU_TYPE_EOSTREAM = 11,
    NALU_TYPE_FILL = 12,
} NaluType;

typedef enum {
    NALU_PRIORITY_DISPOSABLE = 0,
    NALU_PRIRITY_LOW = 1,
    NALU_PRIORITY_HIGH = 2,
    NALU_PRIORITY_HIGHEST = 3
} NaluPriority;


typedef struct {
    int startcodeprefix_len;      //! 4 for parameter sets and first slice in picture, 3 for everything else (suggested)
    unsigned len;                 //! Length of the NAL unit (Excluding the start code, which does not belong to the NALU)
    unsigned max_size;            //! Nal Unit Buffer size
    int forbidden_bit;            //! should be always FALSE
    int nal_reference_idc;        //! NALU_PRIORITY_xxxx
    int nal_unit_type;            //! NALU_TYPE_xxxx
    char *buf;                    //! contains the first byte followed by the EBSP
} NALU_t;

FILE *h264bitstream = NULL;                //!< the bit stream file

int info2 = 0, info3 = 0;

/**
 *
 * @param Buf 输入流
 * @return 1 is 起始码
 */
static int FindStartCode2(unsigned char *Buf) {
    if (Buf[0] == 0 && Buf[1] == 0 && Buf[2] == 1) return 1; //0x000001?
    else return 0;
}

/**
 * @param Buf 输入流
 * @return 1 is 起始码
 */
static int FindStartCode3(unsigned char *Buf) {
    if (Buf[0] == 0 && Buf[1] == 0 && Buf[2] == 0 && Buf[3] == 1) return 1;//0x00000001?
    else return 0;
}


int GetAnnexbNALU(NALU_t *nalu) {
    int pos = 0;
    int StartCodeFound, rewind;
    unsigned char *Buf;

    //nalu->max_size: 要被分配的元素个数
    // sizeof(char) :  元素的大小
    if ((Buf = (unsigned char *) calloc(nalu->max_size, sizeof(char))) == NULL)
        printf("GetAnnexbNALU: Could not allocate Buf memory\n");

    nalu->startcodeprefix_len = 3;
    //查看是否最少长度小于 3，如果没有起始位那么是不对的
    //__size: 读取的每个元素的大小，以字节为单位
    //__nitems: 元素的个数，每个元素的大小为 __size 字节
    if (3 != fread(Buf, 1, 3, h264bitstream)) {
        free(Buf);
        return 0;
    }


    //看看是不是启始码0x00 00 01
    info2 = FindStartCode2(Buf);
    if (info2 != 1) {
        if (1 != fread(Buf + 3, 1, 1, h264bitstream)) {//再读一位
            free(Buf);
            return 0;
        }
        //看看是不是启始码0x00 00 00 01
        info3 = FindStartCode3(Buf);
        if (info3 != 1) {//不是 0x 00 00 00 01 那么就不是 h.264 数据
            free(Buf);
            return -1;
        } else {//第四位是
            pos = 4;
            nalu->startcodeprefix_len = 4;
        }
    } else {//第三位是
        nalu->startcodeprefix_len = 3;
        pos = 3;
    }
//    printf("Buf[0]=%d, Buf[1]=%d, Buf[2]=%d , Buf[3]=%d \n", Buf[0], Buf[1], Buf[2], Buf[3]);
    StartCodeFound = 0;
    info2 = 0;
    info3 = 0;

    while (!StartCodeFound) {
        if (feof(h264bitstream)) {
            nalu->len = (pos - 1) - nalu->startcodeprefix_len;
            memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);
            nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
            nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
            nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
            free(Buf);
            return pos - 1;
        }
        //读取下一个字符，强转为 int 返回
        Buf[pos++] = fgetc(h264bitstream);
        //&Buf[pos - 4]：对地址向前偏移 4 位
        info3 = FindStartCode3(&Buf[pos - 4]);
        if (info3 != 1)
            //&Buf[pos - 4]：对地址向前偏移 3 位
            info2 = FindStartCode2(&Buf[pos - 3]);
        //如果找到起始码 那么就 退出
        StartCodeFound = (info2 == 1 || info3 == 1);
    }

    //向后退 3 or 4 byte
    rewind = (info3 == 1) ? -4 : -3;

    /**
     * rewind: 相对 whence 的偏移量，以字节为单位
     * whence: 表示开始添加偏移 offset 的位置。它一般指定为下列常量之一：
     *
     * SEEK_SET: 文件的开头
     * SEEK_CUR: 文件指针的当前位置
     * SEEK_END: 文件的末尾
     */
    //对于当前位置 rewind 的偏移
    if (0 != fseek(h264bitstream, rewind, SEEK_CUR)) {
        free(Buf);
        printf("GetAnnexbNALU: Cannot fseek in the bit stream file");
    }

    nalu->len = (pos + rewind) - nalu->startcodeprefix_len;
    memcpy(nalu->buf, &Buf[nalu->startcodeprefix_len], nalu->len);//
    nalu->forbidden_bit = nalu->buf[0] & 0x80; //1 bit
    nalu->nal_reference_idc = nalu->buf[0] & 0x60; // 2 bit
    nalu->nal_unit_type = (nalu->buf[0]) & 0x1f;// 5 bit
    free(Buf);
    return (pos + rewind);
}

/**
 * Analysis H.264 Bitstream
 * @param url    Location of input H.264 bitstream file.
 */
int h264_parser(char *url) {

    NALU_t *n;
    int buffersize = 100000;

    FILE *myout = stdout;

    h264bitstream = fopen(url, "rb+");
    if (h264bitstream == NULL) {
        printf("Open file error\n");
        return 0;
    }

    n = (NALU_t *) calloc(1, sizeof(NALU_t));
    if (n == NULL) {
        printf("Alloc NALU Error\n");
        return 0;
    }

    n->max_size = buffersize;
    n->buf = (char *) calloc(buffersize, sizeof(char));
    if (n->buf == NULL) {
        free(n);
        printf("AllocNALU: n->buf");
        return 0;
    }

    int data_offset = 0;
    int nal_num = 0;
    printf("-----+-------- NALU Table ------+---------+\n");
    printf(" NUM |    POS  |    IDC |  TYPE |   LEN   |\n");
    printf("-----+---------+--------+-------+---------+\n");

    while (!feof(h264bitstream)) {
        int data_lenth;
        data_lenth = GetAnnexbNALU(n);

        char type_str[20] = {0};
        switch (n->nal_unit_type) {
            case NALU_TYPE_SLICE:
                sprintf(type_str, "SLICE");
                break;
            case NALU_TYPE_DPA:
                sprintf(type_str, "DPA");
                break;
            case NALU_TYPE_DPB:
                sprintf(type_str, "DPB");
                break;
            case NALU_TYPE_DPC:
                sprintf(type_str, "DPC");
                break;
            case NALU_TYPE_IDR:
                sprintf(type_str, "IDR");
                break;
            case NALU_TYPE_SEI:
                sprintf(type_str, "SEI");
                break;
            case NALU_TYPE_SPS:
                sprintf(type_str, "SPS");
                break;
            case NALU_TYPE_PPS:
                sprintf(type_str, "PPS");
                break;
            case NALU_TYPE_AUD:
                sprintf(type_str, "AUD");
                break;
            case NALU_TYPE_EOSEQ:
                sprintf(type_str, "EOSEQ");
                break;
            case NALU_TYPE_EOSTREAM:
                sprintf(type_str, "EOSTREAM");
                break;
            case NALU_TYPE_FILL:
                sprintf(type_str, "FILL");
                break;
        }
        char idc_str[20] = {0};
        switch (n->nal_reference_idc >> 5) {
            case NALU_PRIORITY_DISPOSABLE:
                sprintf(idc_str, "DISPOS");
                break;
            case NALU_PRIRITY_LOW:
                sprintf(idc_str, "LOW");
                break;
            case NALU_PRIORITY_HIGH:
                sprintf(idc_str, "HIGH");
                break;
            case NALU_PRIORITY_HIGHEST:
                sprintf(idc_str, "HIGHEST");
                break;
        }

        fprintf(myout, "%5d| %8d| %7s| %6s| %8d|\n", nal_num, data_offset, idc_str, type_str, n->len);

        data_offset = data_offset + data_lenth;

        nal_num++;
    }

    //Free
    if (n) {
        if (n->buf) {
            free(n->buf);
            n->buf = NULL;
        }
        free(n);
    }
    return 0;
}


int main() {
    return h264_parser("/Users/devyk/Data/Project/sample/github_code/AVSample/file/123.h264");
}
