//
// Created by 阳坤 on 2021/7/4.
//

#include "pcmplay.h"

PCMPlay::PCMPlay() {
}

void PCMPlay::init(const char *pcm_path, int sampleRate, int channelCount, int sampleSize) {
    QFile f(pcm_path);
    if (!f.open(QIODevice::ReadOnly)) {
        exit(0);
    }
    m_buffer = f.readAll();
    m_pos = 0;
}

void PCMPlay::start() {
    open(QIODevice::ReadOnly);
}

void PCMPlay::stop() {
    m_pos = 0;
    close();
}

qint64 PCMPlay::readData(char *data, qint64 maxlen) {
    if (m_pos >= m_buffer.size())
        return 0;
    //loop
    qint64 total = 0;
    if (!m_buffer.isEmpty()) {
        while (maxlen - total > 0) {
            const qint64 chunk = qMin((m_buffer.size() - m_pos), maxlen - total);
            memcpy(data + total, m_buffer.constData() + m_pos, chunk);
            m_pos = (m_pos + chunk) % m_buffer.size();
            qDebug() << "m_pos--->" << m_pos << endl;
            total += chunk;
        }
    }
    return maxlen;


//播一遍
//    int len;
//    //计算未播放的数据的长度.
//    len = (m_pos+maxlen) > m_buffer.size() ? (m_buffer.size() - m_pos) : maxlen;
//    memcpy(data, m_buffer.data()+m_pos, len); //把要播放的pcm数据存入声卡缓冲区里.
//    m_pos += len; //更新已播放的数据长度.
//    return len;

}

qint64 PCMPlay::writeData(const char *data, qint64 len) {
    Q_UNUSED(data);
    Q_UNUSED(len);
    qDebug() << "writeData--->len=" << len << endl;
    return 0;
}

qint64 PCMPlay::bytesAvailable() const {
    return m_buffer.size() + QIODevice::bytesAvailable();
}

