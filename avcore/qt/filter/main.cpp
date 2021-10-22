//
// Created by —Ù¿§ on 2021/7/29.
//

#include <QApplication>
#include "FFmpegAVFilter.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    FFmpegAVFilter filter;
    filter.show();
    return a.exec();
}