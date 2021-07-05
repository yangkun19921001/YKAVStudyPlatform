//
// Created by 阳坤 on 2021/7/4.
//

#ifndef YKAVSTUDYPLATFORM_QYUVPLAYER_H
#define YKAVSTUDYPLATFORM_QYUVPLAYER_H

#include <QWidget>


QT_BEGIN_NAMESPACE
namespace Ui { class QYUVPlayer; }
QT_END_NAMESPACE

class QYUVPlayer : public QWidget {
Q_OBJECT

public:
    explicit QYUVPlayer(QWidget *parent = nullptr);

    ~QYUVPlayer() override;

private:
    Ui::QYUVPlayer *ui;
};


#endif //YKAVSTUDYPLATFORM_QYUVPLAYER_H
