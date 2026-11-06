#include "bthread/business_thread.h"
#include "bthread/message_bus.h"

#include <QMetaObject>
#include <spdlog/spdlog.h>

namespace bthread {

// =============================================================================
// BusinessThread 实现 - 业务线程模板基类
// =============================================================================

// 统一的 spdlog 日志名（与 main.cpp 中 initLog 注册的名称一致）
static const char* kLoggerName = "app";

// 安全获取 logger，不存在时返回 nullptr（调用方自行兜底）
static std::shared_ptr<spdlog::logger> appLogger() {
    return spdlog::get(kLoggerName);
}

// -----------------------------------------------------------------------------
// 构造：连接内部消息投递信号到 onMessage 槽（QueuedConnection）
// 这样 postMessage() 发出的 _messagePosted 信号会被投递到本线程事件队列，
// 由本线程的事件循环取出执行 onMessage，保证 handleMessage 在本线程执行。
// -----------------------------------------------------------------------------
BusinessThread::BusinessThread(const QString& name, QObject* parent)
    : QThread(parent), name_(name) {
    connect(this, &BusinessThread::_messagePosted,
            this, &BusinessThread::onMessage, Qt::QueuedConnection);
}

// 析构：优雅停止并等待线程退出
BusinessThread::~BusinessThread() {
    stopGracefully();
    wait();
}

// 注入消息总线并订阅其广播（跨线程自动 QueuedConnection）
void BusinessThread::setMessageBus(MessageBus* bus) {
    bus_ = bus;
    if (bus_) {
        connect(bus_, &MessageBus::messagePublished,
                this, &BusinessThread::onMessage, Qt::QueuedConnection);
    }
}

// 由其他线程调用：向本线程投递消息（经 _messagePosted 信号转入本线程）
void BusinessThread::postMessage(const Message& msg) {
    emit _messagePosted(msg);
}

// 优雅停止：清运行标志 + 退出事件循环（exec() 返回）
void BusinessThread::stopGracefully() {
    running_.storeRelaxed(0);
    quit();
}

// -----------------------------------------------------------------------------
// run() - 线程入口，按固定生命周期顺序执行钩子：
//   onInit() -> started 信号 -> logInfo -> onStart() -> exec() -> onStop() -> onCleanup()
// exec() 是事件循环，处理消息队列、定时器、QueuedConnection 信号；
// stopGracefully() 调用 quit() 后 exec() 返回，继续执行清理钩子。
// -----------------------------------------------------------------------------
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

// 消息处理入口：转交给子类重写的 handleMessage
void BusinessThread::onMessage(const Message& msg) {
    handleMessage(msg);
}

// 广播消息到总线（所有订阅线程都会收到）
void BusinessThread::broadcast(const QString& type, const QVariant& data) {
    if (bus_) {
        bus_->publish(Message(type, name_, data));
    }
}

// -----------------------------------------------------------------------------
// 混合日志：子线程直接写 spdlog + 发信号给 UI
// spdlog 的 _mt sink 内部加锁，多线程直接调用安全，且保证同一线程内日志有序。
// 信号仅用于 UI 显示，不重复写 spdlog。
// -----------------------------------------------------------------------------
void BusinessThread::logDebug(const QString& msg) {
    if (auto l = appLogger()) {
        l->debug("[{}] {}", name_.toStdString(), msg.toStdString());
    }
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
