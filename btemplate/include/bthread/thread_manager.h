#ifndef BTHREAD_THREAD_MANAGER_H
#define BTHREAD_THREAD_MANAGER_H

#include <QObject>
#include <QList>
#include <QString>

#include "business_thread.h"
#include "message_bus.h"

namespace bthread {

// =============================================================================
// ThreadManager - 业务线程管理器
// -----------------------------------------------------------------------------
// 常驻主线程，负责：
//   1. 创建并持有 MessageBus（消息总线）
//   2. 注册业务线程，并把总线注入每个线程
//   3. 统一 startAll() / stopAll()（逆序停止）
//   4. 计数已启动线程，发出 allStarted / allStopped 信号
//   5. 转发各线程的 logMessage 信号到 threadLogMessage（供 UI 显示）
// 管理器不持有线程所有权，线程由调用方（主线程）管理生命周期。
// =============================================================================
class ThreadManager : public QObject {
    Q_OBJECT
public:
    explicit ThreadManager(QObject* parent = nullptr);
    ~ThreadManager() override;

    // 注册一个业务线程（管理器不持有所有权，由调用方管理生命周期）
    void addThread(BusinessThread* thread);

    // 获取消息总线
    MessageBus* bus() const { return bus_; }

    // 按注册顺序启动所有线程
    void startAll();

    // 逆序优雅停止所有线程
    void stopAll();

    // 是否已全部启动
    bool allRunning() const;

    // 按名称查找线程
    BusinessThread* findThread(const QString& name) const;

signals:
    void allStarted();
    void allStopped();
    // 线程日志：仅供 UI 显示使用（spdlog 已由子线程直接写入，此处不重复写）
    // level: 0=Debug 1=Info 2=Warn 3=Error
    void threadLogMessage(int level, const QString& name, const QString& msg);

private:
    QList<BusinessThread*> threads_;
    MessageBus* bus_ = nullptr;
    int runningCount_ = 0;
};

} // namespace bthread

#endif // BTHREAD_THREAD_MANAGER_H
