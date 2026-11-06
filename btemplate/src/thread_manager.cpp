#include "bthread/thread_manager.h"

namespace bthread {

ThreadManager::ThreadManager(QObject* parent)
    : QObject(parent), bus_(new MessageBus(this)) {
}

ThreadManager::~ThreadManager() {
    stopAll();
}

void ThreadManager::addThread(BusinessThread* thread) {
    thread->setMessageBus(bus_);
    threads_.append(thread);
    // 转发日志信号到管理器统一出口
    connect(thread, &BusinessThread::log,
            this, &ThreadManager::threadLog, Qt::QueuedConnection);
    connect(thread, &BusinessThread::started, this, [this](const QString&) {
        if (++runningCount_ == threads_.size()) emit allStarted();
    });
    connect(thread, &BusinessThread::stopped, this, [this](const QString&) {
        if (--runningCount_ <= 0) emit allStopped();
    });
}

void ThreadManager::startAll() {
    for (auto* t : threads_) {
        t->start();
    }
}

void ThreadManager::stopAll() {
    // 逆序停止（依赖后启动的先停）
    for (int i = threads_.size() - 1; i >= 0; --i) {
        threads_[i]->stopGracefully();
    }
    for (auto* t : threads_) {
        t->wait();
    }
}

bool ThreadManager::allRunning() const {
    for (auto* t : threads_) {
        if (!t->isRunning()) return false;
    }
    return !threads_.isEmpty();
}

BusinessThread* ThreadManager::findThread(const QString& name) const {
    for (auto* t : threads_) {
        if (t->name() == name) return t;
    }
    return nullptr;
}

} // namespace bthread
