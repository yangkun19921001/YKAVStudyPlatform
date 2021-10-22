#include <memory>
#include <functional>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <thread>

#include "../src/aloop.h"

using namespace aloop;
using namespace std;


void AsyncPostExample() {
    auto looper = ALooper::create();
    looper->start();

    class MyHandler : public AHandler {
    protected:
        void onMessageReceived(const std::shared_ptr<AMessage> &msg) {
            printf("receive %d\n", msg->what());
        }
    };

    shared_ptr<AHandler> handler(new MyHandler);
    looper->registerHandler(handler);
    AMessage::create(6, handler)->post(3000*1000);
    usleep(100 * 10000000);
    looper->stop();
}

void SyncPostExample() {
    auto looper = ALooper::create();
    looper->start();

    class MyHandler : public AHandler {
    protected:
        void onMessageReceived(const std::shared_ptr<AMessage> &msg) {
            printf("receive %d\n", msg->what());

            shared_ptr<AReplyToken> replyToken;
            if (msg->senderAwaitsResponse(&replyToken)) {
                auto response = AMessage::create();
                response->setInt32("extra", 1);
                response->postReply(replyToken);
            }
        }
    };

    shared_ptr<AHandler> handler(new MyHandler);
    looper->registerHandler(handler);

    auto response = AMessage::createNull();
    if (AMessage::create(1, handler)->postAndAwaitResponse(&response) == OK) {
        int extra = 0;
        if (response->findInt32("extra", &extra)) {
            printf("response %d\n", extra);
        }
    }

    looper->stop();
}


void NotifyExample() {
    auto looper = ALooper::create();
    looper->start();

    class WorkerHandler : public AHandler {
    public:
        WorkerHandler(shared_ptr<AMessage> notify) : mNotify(notify) {
        }

    protected:
        void onMessageReceived(const std::shared_ptr<AMessage> &msg) {
            printf("recv work: %d\n", msg->what());
            auto notify = mNotify->dup();
            notify->setInt32("id", msg->what());
            notify->post();
        }

    private:
        shared_ptr<AMessage> mNotify;
    };

    class ListenHandler : public AHandler {
    protected:
        void onMessageReceived(const std::shared_ptr<AMessage> &msg) {
            int32_t id;
            msg->findInt32("id", &id);
            printf("recv notify: %d, %d\n", msg->what(), id);
        }
    };
    int32_t WHAT_FOO = 1;
    int32_t WHAT_NOTIFY = 2;
    shared_ptr<ListenHandler> listener(new ListenHandler);
    looper->registerHandler(listener);
    auto notify = AMessage::create(WHAT_NOTIFY, listener);

    shared_ptr<WorkerHandler> worker(new WorkerHandler(notify));
    looper->registerHandler(worker);

    auto fooMsg = AMessage::create(WHAT_FOO, worker);
    fooMsg->post();

    usleep(100 * 1000);
    looper->stop();
}

void FuncCallbackExample() {
    auto looper = ALooper::create();
    looper->start();

    using Callback = function<void(const shared_ptr<AMessage> &)>;
    class FuncHandler : public AHandler {
    protected:
        void onMessageReceived(const shared_ptr<AMessage> &msg) {
            shared_ptr<Callback> callback;
            msg->findObject("callback", &callback);

            (*callback)(msg);
        }
    };

    shared_ptr<FuncHandler> handler(new FuncHandler);
    looper->registerHandler(handler);

    auto msg = AMessage::create(1, handler);
    Callback callback = [](const shared_ptr<AMessage> msg) {
        printf("handle callback msg:%d\n", msg->what());
    };

    msg->setObject("callback", shared_ptr<Callback>(new Callback(callback)));
    msg->post();

    usleep(100 * 1000);
    looper->stop();
}

void RunOnCustomThreadExample() {
    auto looper = ALooper::create();
    thread mythd([looper] {
        printf("mythd begin\n");
        printf("alter thread attr here\n");
        looper->start(true);
        printf("mythd end\n");
        printf("clean up here after loop stop\n");
    });

    usleep(100 * 1000);
    printf("stop looper");
    looper->stop();
    mythd.join();
}

int main(int argc, char *argv[]) {
    setPrintFunc([](int level, const char *msg) {
        printf("[TestPrint][%d] %s\n", level, msg);
    });
    AsyncPostExample();
//    SyncPostExample();
//    NotifyExample();
//    FuncCallbackExample();
//    RunOnCustomThreadExample();
    return 0;
}