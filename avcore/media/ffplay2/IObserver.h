//
// Created by 阳坤 on 2021/7/19.
//

#ifndef QTFFPLAYER_IOBSERVER_H
#define QTFFPLAYER_IOBSERVER_H

#include "IThread.h"

//https://github.com/imalimin/FilmKilns/tree/master/src/al_common/thread
struct AMessage {

};

/**
 * 观察者
 * 主要是观察 解码的数据
 */
class IObserver : public IThread {

public:
    int id = 0;
    /**
     * 发送数据给订阅者
     * 子类实现
     * @param data
     */
    virtual void update(AMessage data) {};

    /**
     * 订阅消息，有数据更新就通知
     * @param obs
     */
    void addRegister(IObserver *obs = 0);

    /**
     * 取消订阅
     * @param obs
     */
    void delRegister(IObserver *obs = 0);

    /**
    * 取消所有订阅
    * @param obs
    */
    void delAllRegister();

    /**
     * 发送消息
     * @param data
     */
    void sendAMessage(AMessage data);

private:
    /**
      * 通知所有的订阅者，有新的数据产生或者更新。
      * @param data
      */
    void notifyAll(AMessage data);

public:
    char *TAG = (char *) ("UnKnown %s");//观察者 TAG

protected:
    std::map<int,IObserver*> obss;
    mutex mux;//互斥锁
};


#endif //QTFFPLAYER_IOBSERVER_H
