//
// Created by 阳坤 on 2021/7/4.
//

#ifndef YKAVSTUDYPLATFORM_QYUVWIDGET_H
#define YKAVSTUDYPLATFORM_QYUVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QDebug>
#include <QTimer>

static const GLfloat GET_VER[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f
};



class QYUVWidget : public QOpenGLWidget, protected QOpenGLFunctions {
Q_OBJECT

public:
    QYUVWidget(QWidget *);

    ~QYUVWidget();

protected:
    //刷新显示
    void paintGL() override;

    //初始化 gl
    void initializeGL() override;

    //窗口尺寸发生变化
    void resizeGL(int w, int h) override;
};


#endif //YKAVSTUDYPLATFORM_QYUVWIDGET_H
