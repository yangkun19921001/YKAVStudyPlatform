//
// Created by 阳坤 on 2021/7/3.
//

#ifndef YKAVSTUDYPLATFORM_QAUDIOPLAYER_H
#define YKAVSTUDYPLATFORM_QAUDIOPLAYER_H


#include <QTimer>
#include <QMainWindow>
#include <QPushButton>
#include <QComboBox>
#include <QLabel>
#include <QVBoxLayout>
#include "pcmplay.h"

class AudioTest : public QMainWindow {
Q_OBJECT

public:
    AudioTest(const char *pcm_path, int sampleRate, int channelCount, int sampleSize);

    ~AudioTest();

private:
    void initializeWindow();

    void initializeAudio(const QAudioDeviceInfo &deviceInfo);

private:
    QTimer *m_pushTimer = nullptr;
    const char *inpath = nullptr;
    int sampleRate = 0;
    int channelCount = 0;
    int sampleSize = 0;

    // Owned by layout
    QPushButton *m_modeButton = nullptr;
    QPushButton *m_suspendResumeButton = nullptr;
    QComboBox *m_deviceBox = nullptr;
    QLabel *m_volumeLabel = nullptr;
    QSlider *m_volumeSlider = nullptr;

    QScopedPointer<PCMPlay> m_pcmplay;
    QScopedPointer<QAudioOutput> m_audioOutput;

    bool m_pullMode = true;

private slots:

    void toggleMode();

    void toggleSuspendResume();

    void deviceChanged(int index);

    void volumeChanged(int);
};

#endif //YKAVSTUDYPLATFORM_QAUDIOPLAYER_H
