//
// Created by 阳坤 on 2021/7/4.
//

#ifndef YKAVSTUDYPLATFORM_QYUVWIDGET_H
#define YKAVSTUDYPLATFORM_QYUVWIDGET_H

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QGLShaderProgram>
#include <QDebug>
#include <QTimer>
#include <QException>
#include <QOpenGLShader>
#include <QOpenGLTexture>
#include <QFile>
#include <fstream>
#include <QThread>
//自动加双引号
#define GET_STR(x) #x
#define A_VER 0
#define T_VER 1



//传递顶点和材质坐标
//顶点
static const GLfloat VER[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f
};

//材质
static const GLfloat TEX[] = {
        0.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
};


class QYUVWidget : public QOpenGLWidget, protected QOpenGLFunctions {
Q_OBJECT

public:
    QYUVWidget(QWidget *);

    ~QYUVWidget();

    //初始化数据大小
    void InitDrawBufSize(uint64_t size);

    //绘制
    void DrawVideoFrame(unsigned char *data, int frameWidth, int frameHeight);


protected:
    //刷新显示
    void paintGL() override;

    //初始化 gl
    void initializeGL() override;

    //窗口尺寸发生变化
    void resizeGL(int w, int h) override;



private:
    struct QYUVWidgetImpl;
    QScopedPointer<QYUVWidgetImpl> impl;

private:
    //shader中yuv变量地址
    GLuint unis[3] = {0};
    //openg的 texture地址
    GLuint texs[3] = {0};

    //材质内存空间
    unsigned char *datas[3] = {0};

    int width = 1920;
    int height = 1080;

};

class QYUVException : public QException {
public:
    void reise() const { throw *this; }

    QYUVException *clone() const { return new QYUVException(*this); }

};


struct QYUVWidget::QYUVWidgetImpl
{
    QYUVWidgetImpl()
            : mBufYuv(nullptr)
            , mFrameSize(0)
    {}

    GLvoid*                 mBufYuv;
    int                     mFrameSize;

    QOpenGLShader*          mVShader;
    QOpenGLShader*          mFShader;
    QOpenGLShaderProgram*   mShaderProgram;

    QOpenGLTexture*         mTextureY;
    QOpenGLTexture*         mTextureU;
    QOpenGLTexture*         mTextureV;

    GLuint                  id_y, id_u, id_v;
    int                     textureUniformY, textureUniformU, textureUniformV;
    GLsizei                 mVideoW, mVideoH;


    unsigned char*              mBuffer;
    unsigned                    mSize;

};

#endif //YKAVSTUDYPLATFORM_QYUVWIDGET_H
