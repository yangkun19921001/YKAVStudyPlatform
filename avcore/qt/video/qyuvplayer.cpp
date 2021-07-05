//
// Created by 阳坤 on 2021/7/4.
//

// You may need to build the project (run Qt uic code generator) to get "ui_QYUVPlayer.h" resolved

#include "qyuvplayer.h"
#include "ui_QYUVPlayer.h"


QYUVPlayer::QYUVPlayer(QWidget *parent) :
        QWidget(parent), ui(new Ui::QYUVPlayer) {
    ui->setupUi(this);
}

QYUVPlayer::~QYUVPlayer() {
    delete ui;
}

