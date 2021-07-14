//
// Created by 阳坤 on 2021/7/14.
//

#include "TestWidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    TestWidget w;
    w.show();
    w.readSampleFile();
    w.player();

    return a.exec();
}
