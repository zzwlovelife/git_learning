#ifndef BTHREAD_THREAD_MANAGER_H
#define BTHREAD_THREAD_MANAGER_H

#include <QObject>
#include <QList>
#include <QString>

#include "business_thread.h"
#include "message_bus.h"

namespace bthread {

// 线程管理器：负责注册、启动、停止所有业务线程，并提供消息总线
// 管理器本身常驻主线程
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
    void threadLog(const QString& name, const QString& msg);

private:
    QList<BusinessThread*> threads_;
    MessageBus* bus_ = nullptr;
    int runningCount_ = 0;
};

} // namespace bthread

#endif // BTHREAD_THREAD_MANAGER_H
