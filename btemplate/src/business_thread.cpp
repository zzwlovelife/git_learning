#include "bthread/business_thread.h"
#include "bthread/message_bus.h"

#include <QMetaObject>
#include <spdlog/spdlog.h>

namespace bthread {

// 统一的 spdlog 日志名（与 main.cpp 中 initLog 注册的名称一致）
static const char* kLoggerName = "app";

// 安全获取 logger，不存在时返回 nullptr（调用方自行兜底）
static std::shared_ptr<spdlog::logger> appLogger() {
    return spdlog::get(kLoggerName);
}

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

void BusinessThread::logDebug(const QString& msg) {
    // 1. 子线程直接写 spdlog（文件+控制台，_mt sink 线程安全，低延迟、保序）
    if (auto l = appLogger()) {
        l->debug("[{}] {}", name_.toStdString(), msg.toStdString());
    }
    // 2. 同时发信号给主线程 UI 显示（不重复写 spdlog）
    emit logMessage(static_cast<int>(LogLevel::Debug), name_, msg);
}

void BusinessThread::logInfo(const QString& msg) {
    if (auto l = appLogger()) {
        l->info("[{}] {}", name_.toStdString(), msg.toStdString());
    }
    emit logMessage(static_cast<int>(LogLevel::Info), name_, msg);
}

void BusinessThread::logWarn(const QString& msg) {
    if (auto l = appLogger()) {
        l->warn("[{}] {}", name_.toStdString(), msg.toStdString());
    }
    emit logMessage(static_cast<int>(LogLevel::Warn), name_, msg);
}

void BusinessThread::logError(const QString& msg) {
    if (auto l = appLogger()) {
        l->error("[{}] {}", name_.toStdString(), msg.toStdString());
    }
    emit logMessage(static_cast<int>(LogLevel::Error), name_, msg);
    emit error(name_, msg);
}

} // namespace bthread
