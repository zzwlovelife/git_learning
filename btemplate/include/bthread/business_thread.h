#ifndef BTHREAD_BUSINESS_THREAD_H
#define BTHREAD_BUSINESS_THREAD_H

#include <QThread>
#include <QString>
#include <QAtomicInt>

#include "message.h"

namespace bthread {

class MessageBus;

// 业务线程模板基类
// ---------------------------------------------------------------------------
// 设计要点：
// 1. 继承 QThread，run() 内启动事件循环（exec()），使信号槽跨线程通信成为可能
// 2. 生命周期钩子：onInit / onStart / onMessage / onStop / onCleanup，子类按需重写
// 3. 通信：通过 MessageBus 广播，或 postMessage() 点对点投递（均为 QueuedConnection）
// 4. 优雅停止：stopGracefully() 置位标志 + quit() 退出事件循环
// 5. 日志：混合模式
//    - 子线程直接调用 spdlog（文件+控制台，_mt sink 线程安全，低延迟、保序）
//    - 同时 emit logMessage 信号，供主线程 UI 显示（不重复写 spdlog）
// ---------------------------------------------------------------------------
class BusinessThread : public QThread {
    Q_OBJECT
public:
    explicit BusinessThread(const QString& name, QObject* parent = nullptr);
    ~BusinessThread() override;

    // 日志级别（与信号一起传递，便于 UI 按级别着色）
    enum LogLevel { Debug = 0, Info, Warn, Error };
    Q_ENUM(LogLevel)

    QString name() const { return name_; }
    bool isRunning() const { return running_.loadRelaxed() != 0; }

    // 设置消息总线（由 ThreadManager 注入）
    void setMessageBus(MessageBus* bus);

    // 由其他线程调用：向本线程投递一条消息（QueuedConnection，线程安全）
    void postMessage(const Message& msg);

    // 优雅停止：置位运行标志，退出事件循环
    void stopGracefully();

signals:
    void started(const QString& name);
    void stopped(const QString& name);
    void error(const QString& name, const QString& err);

    // 日志信号：仅供 UI 显示使用（spdlog 已由子线程直接写入，避免重复）
    // level 取值见 LogLevel 枚举
    void logMessage(int level, const QString& name, const QString& msg);

    // 内部信号：用于 postMessage 跨线程投递
    void _messagePosted(const bthread::Message& msg);

public slots:
    // 消息处理入口（在本线程事件循环中执行）
    void onMessage(const bthread::Message& msg);

protected:
    void run() override;

    // ===== 子类重写的生命周期钩子（均在本线程上下文中执行）=====
    // 初始化（返回 false 则线程启动失败）
    virtual bool onInit() { return true; }
    // 初始化完成后、进入事件循环前调用
    virtual void onStart() {}
    // 收到消息时调用
    virtual void handleMessage(const Message& msg) { Q_UNUSED(msg); }
    // 事件循环退出后调用
    virtual void onStop() {}
    // 最终清理
    virtual void onCleanup() {}

    // 子类可用：向总线广播消息
    void broadcast(const QString& type, const QVariant& data = QVariant());
    // 子类可用：记录日志（混合模式：直接写 spdlog + 发信号给 UI）
    void logDebug(const QString& msg);
    void logInfo(const QString& msg);
    void logWarn(const QString& msg);
    void logError(const QString& msg);

    QString name_;
    MessageBus* bus_ = nullptr;
    QAtomicInt running_{0};
};

} // namespace bthread

#endif // BTHREAD_BUSINESS_THREAD_H
