//
// Created by 阳坤 on 2021/7/4.
//


#include "QYUVWidget.h"


//顶点shader
const char *vString = GET_STR(
        attribute vec4 vertexIn;
        attribute vec2 textureIn;
        varying vec2 textureOut;
        void main(void) {
            gl_Position = vertexIn;
            textureOut = textureIn;
        }
);


//片元shader
const char *fString = GET_STR(
        varying vec2 textureOut;
        uniform sampler2D tex_y;
        uniform sampler2D tex_u;
        uniform sampler2D tex_v;
        void main(void) {
            vec3 yuv;
            vec3 rgb;
            yuv.x = texture2D(tex_y, textureOut).r;
            yuv.y = texture2D(tex_u, textureOut).r - 0.5;
            yuv.z = texture2D(tex_v, textureOut).r - 0.5;
            rgb = mat3(1.0, 1.0, 1.0,
                       0.0, -0.39465, 2.03211,
                       1.13983, -0.58060, 0.0) * yuv;
            gl_FragColor = vec4(rgb, 1.0);
        }

);

QYUVWidget::QYUVWidget(QWidget *parent) : QOpenGLWidget(parent), impl(new QYUVWidgetImpl()) {

}

QYUVWidget::~QYUVWidget() {
    delete[] reinterpret_cast<unsigned char *>(impl->mBufYuv);
}

void QYUVWidget::InitDrawBufSize(uint64_t size) {
    impl->mFrameSize = size;
    impl->mBufYuv = new unsigned char[size];
}

void QYUVWidget::DrawVideoFrame(unsigned char *data, int frameWidth, int frameHeight) {
    impl->mVideoW = frameWidth;
    impl->mVideoH = frameHeight;
    memcpy(impl->mBufYuv, data, impl->mFrameSize);
    update();
}

void QYUVWidget::initializeGL() {
    qDebug() << "initializeGL";
    //初始化 opengl
    initializeOpenGLFunctions();

    //启用深度测试
    //根据坐标的远近自动隐藏被遮住的图形（材料）
    glEnable(GL_DEPTH_TEST);

    //初始化顶点 shader obj
    impl->mVShader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    //编译顶点 shader program
    if (!impl->mVShader->compileSourceCode(vString)) {
        throw QYUVException();
    }

    //初始化片元 shader obj
    impl->mFShader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    //编译片元 shader program
    if (!impl->mFShader->compileSourceCode(fString)) {
        throw QYUVException();
    }

    //创建 shader program 容器
    impl->mShaderProgram = new QOpenGLShaderProgram(this);
    //将顶点 片元 shader 添加到程序容器中
    impl->mShaderProgram->addShader(impl->mFShader);
    impl->mShaderProgram->addShader(impl->mVShader);

    //设置顶点坐标的变量
    impl->mShaderProgram->bindAttributeLocation("vertexIn", A_VER);

    //设置材质坐标
    impl->mShaderProgram->bindAttributeLocation("textureIn", T_VER);

    //编译shader
    qDebug() << "program.link() = " << impl->mShaderProgram->link();

    qDebug() << "program.bind() = " << impl->mShaderProgram->bind();

    //从shader获取材质
    impl->textureUniformY = impl->mShaderProgram->uniformLocation("tex_y");
    impl->textureUniformU = impl->mShaderProgram->uniformLocation("tex_u");
    impl->textureUniformV = impl->mShaderProgram->uniformLocation("tex_v");

    //顶点
    glVertexAttribPointer(A_VER, 2, GL_FLOAT, 0, 0, VER);
    glEnableVertexAttribArray(A_VER);

    //材质
    glVertexAttribPointer(T_VER, 2, GL_FLOAT, 0, 0, TEX);
    glEnableVertexAttribArray(T_VER);


    //创建材质
    glGenTextures(3, texs);
    impl->id_y = texs[0];
    impl->id_u = texs[1];
    impl->id_v = texs[2];
    glClearColor(0.0, 0.0, 0.0, 0.0);

}

void QYUVWidget::paintGL() {
    glActiveTexture(GL_TEXTURE0);//激活纹理单元GL_TEXTURE0,系统里面的
    glBindTexture(GL_TEXTURE_2D, impl->id_y); //绑定y分量纹理对象id到激活的纹理单元
//    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
//    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
//    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
//    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    //创建 Y 数据纹理
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW, impl->mVideoH, 0, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                 impl->mBufYuv);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 加载 U 数据
    glActiveTexture(GL_TEXTURE1);//Activate texture unit GL_TEXTURE1
    glBindTexture(GL_TEXTURE_2D, impl->id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW / 2, impl->mVideoH / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, (char *) impl->mBufYuv + impl->mVideoW * impl->mVideoH);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //加载 V 数据
    glActiveTexture(GL_TEXTURE2);//Activate texture unit GL_TEXTURE2
    glBindTexture(GL_TEXTURE_2D, impl->id_v);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_LUMINANCE, impl->mVideoW / 2, impl->mVideoH / 2, 0, GL_LUMINANCE,
                 GL_UNSIGNED_BYTE, (char *) impl->mBufYuv + impl->mVideoW * impl->mVideoH * 5 / 4);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    //与shader uni遍历关联
    //指定y纹理要使用新值,只能用0,1,2等表示纹理单元的索引
    glUniform1i(impl->textureUniformY, 0);
    glUniform1i(impl->textureUniformU, 1);
    glUniform1i(impl->textureUniformV, 2);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    qDebug() << "paintGL";
}

void QYUVWidget::resizeGL(int w, int h) {
    qDebug() << "resizeGL " << width << ":" << height;
    glViewport(0, 0, w, h);
    update();
}





