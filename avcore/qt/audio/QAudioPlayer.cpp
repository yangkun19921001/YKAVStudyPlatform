//
// Created by 阳坤 on 2021/7/3.
//


#include "QAudioPlayer.h"

AudioTest::AudioTest(const char *pcm_path, int sampleRate, int channelCount, int sampleSize)
        : m_pushTimer(new QTimer(this)), inpath(pcm_path), sampleSize(sampleSize), channelCount(channelCount),
          sampleRate(sampleRate) {
    initializeWindow();
    initializeAudio(QAudioDeviceInfo::defaultOutputDevice());
}

AudioTest::~AudioTest() {
    m_pushTimer->stop();
}

void AudioTest::initializeWindow() {
    QWidget *window = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout;

    m_deviceBox = new QComboBox(this);
    const QAudioDeviceInfo &defaultDeviceInfo = QAudioDeviceInfo::defaultOutputDevice();
    m_deviceBox->addItem(defaultDeviceInfo.deviceName(), QVariant::fromValue(defaultDeviceInfo));
    for (auto &deviceInfo: QAudioDeviceInfo::availableDevices(QAudio::AudioOutput)) {
        if (deviceInfo != defaultDeviceInfo)
            m_deviceBox->addItem(deviceInfo.deviceName(), QVariant::fromValue(deviceInfo));
    }
    connect(m_deviceBox, QOverload<int>::of(&QComboBox::activated), this, &AudioTest::deviceChanged);
    layout->addWidget(m_deviceBox);

    m_modeButton = new QPushButton(this);
    connect(m_modeButton, &QPushButton::clicked, this, &AudioTest::toggleMode);
    layout->addWidget(m_modeButton);

    m_suspendResumeButton = new QPushButton(this);
    connect(m_suspendResumeButton, &QPushButton::clicked, this, &AudioTest::toggleSuspendResume);
    layout->addWidget(m_suspendResumeButton);

    QHBoxLayout *volumeBox = new QHBoxLayout;
    m_volumeLabel = new QLabel;
    m_volumeLabel->setText(tr("Volume:"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(100);
    m_volumeSlider->setSingleStep(10);
    connect(m_volumeSlider, &QSlider::valueChanged, this, &AudioTest::volumeChanged);
    volumeBox->addWidget(m_volumeLabel);
    volumeBox->addWidget(m_volumeSlider);
    layout->addLayout(volumeBox);

    window->setLayout(layout);

    setCentralWidget(window);
    window->show();
}

#include "pcmplay.h"

void AudioTest::initializeAudio(const QAudioDeviceInfo &audioDeviceInfo) {
    QAudioFormat format;
    format.setSampleRate(this->sampleRate);
    format.setChannelCount(this->channelCount);
    format.setSampleSize(this->sampleSize);
    format.setCodec("audio/pcm");
    format.setByteOrder(QAudioFormat::LittleEndian);
    format.setSampleType(QAudioFormat::SignedInt);
    QAudioDeviceInfo info(audioDeviceInfo);
    bool audioDeviceOk = info.isFormatSupported(format);
    if (!audioDeviceOk) {
        qWarning() << "Default format not supported - trying to use nearest";
        format = info.nearestFormat(format);
    }

    m_pcmplay.reset(new PCMPlay());
    m_audioOutput.reset(new QAudioOutput(audioDeviceInfo, format));
    m_pcmplay->init(this->inpath, this->sampleRate, this->channelCount, this->sampleSize);
    m_pcmplay->start();

    qreal initialVolume = QAudio::convertVolume(m_audioOutput->volume(),
                                                QAudio::LinearVolumeScale,
                                                QAudio::LogarithmicVolumeScale);
    m_volumeSlider->setValue(qRound(initialVolume * 100));

    toggleMode();

}

void AudioTest::deviceChanged(int index) {
    m_pcmplay->stop();
    m_audioOutput->stop();
    m_audioOutput->disconnect(this);
    initializeAudio(m_deviceBox->itemData(index).value<QAudioDeviceInfo>());
}

void AudioTest::volumeChanged(int value) {
    qreal linearVolume = QAudio::convertVolume(value / qreal(100),
                                               QAudio::LogarithmicVolumeScale,
                                               QAudio::LinearVolumeScale);

    m_audioOutput->setVolume(linearVolume);
}

void AudioTest::toggleMode() {
    m_pushTimer->stop();
    m_audioOutput->stop();
    toggleSuspendResume();
    if (m_pullMode) {
        //switch to pull mode (QAudioOutput pulls from PCMPlay as needed)
        m_modeButton->setText(tr("Enable push mode"));
        m_audioOutput->start(m_pcmplay.data());
    } else {
        //switch to push mode (periodically push to QAudioOutput using a timer)
        m_modeButton->setText(tr("Enable pull mode"));
        auto io = m_audioOutput->start();
        m_pushTimer->disconnect();

        connect(m_pushTimer, &QTimer::timeout, [this, io]() {
            if (m_audioOutput->state() == QAudio::StoppedState)
                return;
            QByteArray buffer(1024 * 5, 0);
            int chunks = m_audioOutput->bytesFree() / m_audioOutput->periodSize();
            while (chunks) {
                const qint64 len = m_pcmplay->read(buffer.data(), m_audioOutput->periodSize());
                if (len)
                    io->write(buffer.data(), len);
                if (len != m_audioOutput->periodSize())
                    break;
                --chunks;
            }
        });

        m_pushTimer->start(20);
    }

    m_pullMode = !m_pullMode;
}

void AudioTest::toggleSuspendResume() {
    if (m_audioOutput->state() == QAudio::SuspendedState || m_audioOutput->state() == QAudio::StoppedState) {
        m_audioOutput->resume();
        m_suspendResumeButton->setText(tr("Suspend recording"));
    } else if (m_audioOutput->state() == QAudio::ActiveState) {
        m_audioOutput->suspend();
        m_suspendResumeButton->setText(tr("Resume playback"));
    } else if (m_audioOutput->state() == QAudio::IdleState) {
        // no-op
    }
}

//int main2(int argc, char *argv[]) {
int main2() {
//    QCoreApplication a(argc, argv);
#if (QT_VERSION > QT_VERSION_CHECK(5, 0, 0))
    QAudioFormat fmt;
    fmt.setSampleRate(44100);
    fmt.setSampleSize(16);
    fmt.setChannelCount(2);
    fmt.setCodec("audio/pcm");
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::UnSignedInt);
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    bool audioDeviceOk = info.isFormatSupported(fmt);
    if (audioDeviceOk) {
        QAudioOutput *out = new QAudioOutput(fmt);
        QIODevice *io = out->start(); //开始播放
        io->open(QIODevice::ReadOnly);
        int size = out->periodSize();
        char *buf = new char[size];

        QByteArray ba;
        QFile f("/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/out.pcm");
        if (!f.open(QIODevice::ReadOnly))
            exit(0);
        ba = f.readAll();
        f.close();

        FILE *fp = fopen("/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/out.pcm", "rb");
        while (!feof(fp)) {
            QAudio::State s = out->state();
            if (out->bytesFree() < size) {
                QThread::msleep(1);
                continue;
            }
            int len = fread(buf, 1, size, fp);
            if (len <= 0)break;
            io->write(buf, len);
        }
        fclose(fp);
        delete buf;
        buf = 0;
    } else {
        qDebug() << "Raw audio format not supported by backend, cannot play audio.";
    }
#endif
//    return a.exec();
    return 1;
}