//
// Created by 阳坤 on 2021/7/14.
//

// You may need to build the project (run Qt uic code generator) to get "ui_TestWidget.h" resolved

#include <QGridLayout>
#include "TestWidget.h"


TestWidget::TestWidget(QWidget *parent) :
        QWidget(parent), impl(new TestWidgetImpl()) {
//    setMinimumSize(FRAME_WIDTH/2,FRAME_HEIGHT/2);
    impl->ui->setupUi(this);
    auto* wallLayout = new QGridLayout;
    for (int i = 0; i < 1; ++i)
    {
        for (int j = 0; j < 1; ++j)
        {
            auto* player = new QYUVWidget(this);
            wallLayout->addWidget(player, i, j);
            impl->mPlayers.append(player);
        }
    }

    wallLayout->setSpacing(2);
    wallLayout->setMargin(0);
    impl->ui->video->setLayout(wallLayout);
//    impl->ui->video->setMinimumSize(FRAME_WIDTH/3,FRAME_HEIGHT/3);
}

TestWidget::~TestWidget() {
    delete impl->ui;
}

void TestWidget::readSampleFile() {
    if (!QFile::exists(YUV_FILE_PATH))
    {
        qDebug() << "No sample File!";
        return;
    }

    impl->mSize = QFile(YUV_FILE_PATH).size();
    impl->mBuffer = new unsigned char[impl->mSize];

    std::ifstream yuv_file(YUV_FILE_PATH, std::ios::binary);
    yuv_file.read(reinterpret_cast<char*>(impl->mBuffer), impl->mSize);

    if (yuv_file.tellg() < impl->mSize)
        qWarning() << "YUV file read error, could read only: " << yuv_file.tellg();
}

void TestWidget::player() {
    const int SKIP_COUNT = 3;


    int frameNumber = 0;
    int frameCount = impl->mSize / FRAME_SIZE;

    for (int i = 0; i < impl->mPlayers.count(); ++i)
        impl->mPlayers[i]->InitDrawBufSize(FRAME_SIZE);

    while (frameNumber < frameCount) {
        unsigned char *runner = impl->mBuffer;
        runner += frameNumber * FRAME_SIZE;
        for (int i = 0; i < impl->mPlayers.count(); ++i) {
            if (i * SKIP_COUNT < frameNumber) {
                impl->mPlayers[i]->DrawVideoFrame(runner, FRAME_WIDTH, FRAME_HEIGHT);
            }
        }
        frameNumber++;

        QThread::msleep(40);
        qApp->processEvents();
    }
}
