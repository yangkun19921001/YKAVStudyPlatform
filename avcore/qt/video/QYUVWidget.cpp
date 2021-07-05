//
// Created by 阳坤 on 2021/7/4.
//

#include "QYUVWidget.h"

QYUVWidget::~QYUVWidget() {

}

QYUVWidget::QYUVWidget(QWidget *parent) : QOpenGLWidget(parent) {

}


void QYUVWidget::initializeGL() {
    qDebug() << "initializeGL";
}

void QYUVWidget::paintGL() {
}

void QYUVWidget::resizeGL(int w, int h) {

}
