#ifndef DBCLIENT_CONNECTION_POOL_H
#define DBCLIENT_CONNECTION_POOL_H

#include <QObject>
#include <QHash>
#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <QSqlDatabase>
#include <QElapsedTimer>

#include <memory>

namespace dbclient {

class ConnectionPool : public QObject {
    Q_OBJECT
public:
    explicit ConnectionPool(QString qtDriver, QString databaseName,
                            int maxConnections, int acquireTimeoutMs,
                            int idleTimeoutSec, int connectTimeoutSec,
                            std::function<void(int, const QString&)> logCb,
                            QObject* parent = nullptr);
    ~ConnectionPool() override;

    // 获取一个连接（线程安全）；返回空字符串表示超时或失败
    QString acquire(int timeoutMs = -1);

    // 归还连接（必须在取连接的同一线程调用）
    void release(const QString& connName);

    // 关闭并销毁所有连接
    void shutdown();

    int totalCount() const { return totalConnections_.loadRelaxed(); }

private:
    // 在当前线程创建一个新连接
    QString createConnection();
    // 校验连接是否可用（isOpen + SELECT 1）
    bool ping(const QString& connName);
    // 在当前线程销毁连接
    void destroyConnection(const QString& connName);

    void log(int level, const QString& msg);

    QString qtDriver_;
    QString dbName_;
    int maxConnections_;
    int acquireTimeoutMs_;
    int idleTimeoutSec_;
    int connectTimeoutSec_;
    std::function<void(int, const QString&)> logCb_;

    mutable QMutex mutex_;
    QWaitCondition condition_;

    // 每个线程维护自己的空闲连接队列（Qt 要求连接只能在创建线程使用）
    QHash<Qt::HANDLE, QQueue<QString>> freePerThread_;
    QHash<QString, qint64> lastUsedMs_;   // 连接名 -> 最近归还时间
    QAtomicInt totalConnections_;
    QAtomicInt inUse_;
    bool closed_ = false;
    QAtomicInt seq_;
};

} // namespace dbclient

#endif // DBCLIENT_CONNECTION_POOL_H
