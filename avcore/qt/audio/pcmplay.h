//
// Created by 阳坤 on 2021/7/4.
//

#ifndef YKAVSTUDYPLATFORM_PCMPLAY_H
#define YKAVSTUDYPLATFORM_PCMPLAY_H

#include <QIODevice>
#include <QDebug>
#include <QAudioFormat>
#include <QAudioOutput>
#include <QApplication>
#include <QFile>
#include <QThread>

/**
 * pcm 播放
 */
class PCMPlay : public QIODevice {
Q_OBJECT

public:
    PCMPlay();

    void init(const char *pcm_path, int sampleRate, int channelCount, int sampleSize);

    void start();

    void stop();

    qint64 readData(char *data, qint64 maxlen) override;

    qint64 writeData(const char *data, qint64 len) override;

    qint64 bytesAvailable() const override;

private:
    qint64 m_pos = 0;
    QByteArray m_buffer;
};


#endif //YKAVSTUDYPLATFORM_PCMPLAY_H
