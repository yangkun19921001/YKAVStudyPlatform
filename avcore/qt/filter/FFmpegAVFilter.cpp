//
// Created by ÑôÀ¤ on 2021/7/29.
//

// You may need to build the project (run Qt uic code generator) to get "ui_FFmpegAVFilter.h" resolved

#include "FFmpegAVFilter.h"
#include "ui_FFmpegAVFilter.h"


FFmpegAVFilter::FFmpegAVFilter(QWidget *parent) :
        QWidget(parent), ui(new Ui::FFmpegAVFilter) {
    ui->setupUi(this);
}

FFmpegAVFilter::~FFmpegAVFilter() {
    delete ui;
}

