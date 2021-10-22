//
// Created by —Ù¿§ on 2021/7/29.
//

#ifndef YKAVSTUDYPLATFORM_FFMPEGAVFILTER_H
#define YKAVSTUDYPLATFORM_FFMPEGAVFILTER_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class FFmpegAVFilter; }
QT_END_NAMESPACE

class FFmpegAVFilter : public QWidget {
Q_OBJECT

public:
    explicit FFmpegAVFilter(QWidget *parent = nullptr);

    ~FFmpegAVFilter() override;

private:
    Ui::FFmpegAVFilter *ui;
};


#endif //YKAVSTUDYPLATFORM_FFMPEGAVFILTER_H
