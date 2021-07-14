//
// Created by 阳坤 on 2021/7/14.
//

#ifndef YKAVSTUDYPLATFORM_TESTWIDGET_H
#define YKAVSTUDYPLATFORM_TESTWIDGET_H

#include <QWidget>
#include "QYUVWidget.h"
#include "ui_TestWidget.h"

namespace Ui {
    class TestWidget;
}

namespace
{
    // any file from http://trace.eas.asu.edu/yuv/
    const char* YUV_FILE_PATH = "/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/1920_1080.yuv";
    const unsigned FRAME_WIDTH = 1920; //video frame width, hardcoded for PoC
    const unsigned FRAME_HEIGHT = 1080; //video frame width, hardcoded for PoC
    const unsigned FRAME_SIZE = FRAME_WIDTH * FRAME_HEIGHT * 3/2;
}

class TestWidget : public QWidget {
Q_OBJECT



public:
    explicit TestWidget(QWidget *parent = nullptr);

    ~TestWidget() override;

    void readSampleFile();

    void player();

private:
    struct TestWidgetImpl;
    QScopedPointer<TestWidgetImpl> impl;
};

struct TestWidget::TestWidgetImpl
{
    TestWidgetImpl()
            :ui(new Ui::TestWidget)
    {}

    Ui::TestWidget*             ui;
    unsigned char*              mBuffer;
    unsigned                    mSize;

    QList<QYUVWidget*>       mPlayers;
};



#endif //YKAVSTUDYPLATFORM_TESTWIDGET_H
