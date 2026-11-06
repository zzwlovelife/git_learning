#include "bthread/thread_manager.h"

namespace bthread {

// =============================================================================
// ThreadManager 实现 - 业务线程管理器
// =============================================================================

// 构造：创建消息总线（以 this 为 parent，随管理器析构而释放）
ThreadManager::ThreadManager(QObject* parent)
    : QObject(parent), bus_(new MessageBus(this)) {
}

// 析构：停止所有线程
ThreadManager::~ThreadManager() {
    stopAll();
}

// -----------------------------------------------------------------------------
// addThread - 注册业务线程
// 1. 把消息总线注入线程（线程订阅总线广播）
// 2. 转发线程的 logMessage 到管理器统一出口（供 UI 显示）
// 3. 统计 started/stopped 数量，全部完成时发出 allStarted/allStopped
// -----------------------------------------------------------------------------
void ThreadManager::addThread(BusinessThread* thread) {
    thread->setMessageBus(bus_);
    threads_.append(thread);
    // 转发日志信号到管理器统一出口（仅供 UI 显示，不写 spdlog）
    connect(thread, &BusinessThread::logMessage,
            this, &ThreadManager::threadLogMessage, Qt::QueuedConnection);
    connect(thread, &BusinessThread::started, this, [this](const QString&) {
        if (++runningCount_ == threads_.size()) emit allStarted();
    });
    connect(thread, &BusinessThread::stopped, this, [this](const QString&) {
        if (--runningCount_ <= 0) emit allStopped();
    });
}

// 按注册顺序启动所有线程
void ThreadManager::startAll() {
    for (auto* t : threads_) {
        t->start();
    }
}

// 逆序优雅停止所有线程（后启动的先停，避免依赖断裂），然后等待全部退出
void ThreadManager::stopAll() {
    for (int i = threads_.size() - 1; i >= 0; --i) {
        threads_[i]->stopGracefully();
    }
    for (auto* t : threads_) {
        t->wait();
    }
}

// 检查是否所有线程都在运行
bool ThreadManager::allRunning() const {
    for (auto* t : threads_) {
        if (!t->isRunning()) return false;
    }
    return !threads_.isEmpty();
}

// 按名称查找线程
BusinessThread* ThreadManager::findThread(const QString& name) const {
    for (auto* t : threads_) {
        if (t->name() == name) return t;
    }
    return nullptr;
}

} // namespace bthread
