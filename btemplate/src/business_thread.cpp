#include "bthread/business_thread.h"
#include "bthread/message_bus.h"

#include <QMetaObject>

namespace bthread {

BusinessThread::BusinessThread(const QString& name, QObject* parent)
    : QThread(parent), name_(name) {
    // 内部信号投递到自己的 onMessage 槽（QueuedConnection，保证在本线程执行）
    connect(this, &BusinessThread::_messagePosted,
            this, &BusinessThread::onMessage, Qt::QueuedConnection);
}

BusinessThread::~BusinessThread() {
    stopGracefully();
    wait();
}

void BusinessThread::setMessageBus(MessageBus* bus) {
    bus_ = bus;
    if (bus_) {
        // 订阅总线广播（跨线程时自动 QueuedConnection）
        connect(bus_, &MessageBus::messagePublished,
                this, &BusinessThread::onMessage, Qt::QueuedConnection);
    }
}

void BusinessThread::postMessage(const Message& msg) {
    emit _messagePosted(msg);
}

void BusinessThread::stopGracefully() {
    running_.storeRelaxed(0);
    quit();
}

void BusinessThread::run() {
    if (!onInit()) {
        emit error(name_, "onInit failed");
        return;
    }
    running_.storeRelaxed(1);
    emit started(name_);
    logInfo(name_ + " thread started");
    onStart();
    exec();  // 事件循环：处理消息队列与定时器
    onStop();
    onCleanup();
    running_.storeRelaxed(0);
    emit stopped(name_);
    logInfo(name_ + " thread stopped");
}

void BusinessThread::onMessage(const Message& msg) {
    handleMessage(msg);
}

void BusinessThread::broadcast(const QString& type, const QVariant& data) {
    if (bus_) {
        bus_->publish(Message(type, name_, data));
    }
}

void BusinessThread::logInfo(const QString& msg) {
    emit log(name_, msg);
}

void BusinessThread::logError(const QString& msg) {
    emit error(name_, msg);
    emit log(name_, "[ERROR] " + msg);
}

} // namespace bthread
