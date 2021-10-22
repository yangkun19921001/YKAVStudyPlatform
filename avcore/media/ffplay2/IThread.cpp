//
// Created by —Ù¿§ on 2021/7/19.
//

#include "IThread.h"

void Sleep(int mis) {
    chrono::milliseconds du(mis);
    this_thread::sleep_for(du);
}

void *Main(void *p) {
    IThread *thread = static_cast<IThread *>(p);
    thread->run();
}

bool IThread::start() {
    thread_mutex.lock();
    int ret = pthread_create(&this->tid, NULL, Main, this);
    thread_mutex.unlock();
    return ret != 0;
}

void IThread::release() {
    pthread_exit(this->tid);
}
