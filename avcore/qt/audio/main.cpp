//
// Created by 阳坤 on 2021/7/4.
//

#include "QAudioPlayer.h"
#include "pcmplay.h"
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("Audio Output Test");
    AudioTest audio("/Users/devyk/Data/Project/sample/github_code/YKAVStudyPlatform/temp/out2.pcm"
    ,44100,2,16);
    audio.show();

    return app.exec();
}