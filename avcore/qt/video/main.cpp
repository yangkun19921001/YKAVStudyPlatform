//
// Created by 阳坤 on 2021/7/4.
//

#include "qyuvplayer.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QYUVPlayer w;
    w.show();
    return a.exec();
}