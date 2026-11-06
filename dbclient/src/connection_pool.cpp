#include "connection_pool.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QThread>
#include <QDateTime>
#include <QDebug>

namespace dbclient {

ConnectionPool::ConnectionPool(QString qtDriver, QString databaseName,
                               int maxConnections, int acquireTimeoutMs,
                               int idleTimeoutSec, int connectTimeoutSec,
                               std::function<void(int, const QString&)> logCb,
                               QObject* parent)
    : QObject(parent),
      qtDriver_(std::move(qtDriver)),
      dbName_(std::move(databaseName)),
      maxConnections_(maxConnections),
      acquireTimeoutMs_(acquireTimeoutMs),
      idleTimeoutSec_(idleTimeoutSec),
      connectTimeoutSec_(connectTimeoutSec),
      logCb_(std::move(logCb)) {}

ConnectionPool::~ConnectionPool() {
    shutdown();
}

void ConnectionPool::log(int level, const QString& msg) {
    if (logCb_) {
        logCb_(level, msg);
    } else {
        qWarning().noquote() << msg;
    }
}

QString ConnectionPool::createConnection() {
    const QString name = QStringLiteral("dbclient_%1_%2")
                             .arg(reinterpret_cast<quintptr>(QThread::currentThreadId()), 0, 16)
                             .arg(seq_.fetchAndAddRelaxed(1));
    QSqlDatabase db = QSqlDatabase::addDatabase(qtDriver_, name);
    db.setDatabaseName(dbName_);

    if (qtDriver_ == "QODBC") {
        db.setConnectOptions(
            QStringLiteral("SQL_ATTR_LOGIN_TIMEOUT=%1").arg(connectTimeoutSec_));
    }

    if (!db.open()) {
        log(4, QStringLiteral("[pool] open connection failed: %1").arg(db.lastError().text()));
        QSqlDatabase::removeDatabase(name);
        return QString();
    }

    // 达梦 ODBC 下显式设置编码为 UTF8
    if (qtDriver_ == "QODBC") {
        QSqlQuery q(db);
        q.exec(QStringLiteral("SET NAMES UTF8"));
    }

    totalConnections_.fetchAndAddRelaxed(1);
    log(1, QStringLiteral("[pool] created connection %1 (total=%2)")
              .arg(name).arg(totalConnections_.loadRelaxed()));
    return name;
}

bool ConnectionPool::ping(const QString& connName) {
    QSqlDatabase db = QSqlDatabase::database(connName, false);
    if (!db.isOpen()) {
        return false;
    }
    QSqlQuery q(db);
    if (!q.exec(QStringLiteral("SELECT 1"))) {
        log(3, QStringLiteral("[pool] ping failed for %1: %2")
                   .arg(connName, q.lastError().text()));
        return false;
    }
    return true;
}

void ConnectionPool::destroyConnection(const QString& connName) {
    {
        QSqlDatabase db = QSqlDatabase::database(connName, false);
        if (db.isOpen()) {
            db.close();
        }
    }
    QSqlDatabase::removeDatabase(connName);
    totalConnections_.fetchAndAddRelaxed(-1);
    log(1, QStringLiteral("[pool] destroyed connection %1 (total=%2)")
              .arg(connName).arg(totalConnections_.loadRelaxed()));
}

QString ConnectionPool::acquire(int timeoutMs) {
    if (timeoutMs < 0) timeoutMs = acquireTimeoutMs_;
    QElapsedTimer t; t.start();

    while (true) {
        {
            QMutexLocker locker(&mutex_);
            if (closed_) {
                return QString();
            }
            Qt::HANDLE tid = QThread::currentThreadId();
            QQueue<QString>& free = freePerThread_[tid];

            // 从当前线程的空闲队列取
            while (!free.isEmpty()) {
                QString name = free.dequeue();
                lastUsedMs_.remove(name);
                // 检查是否过期
                locker.unlock();
                if (ping(name)) {
                    inUse_.fetchAndAddRelaxed(1);
                    return name;
                }
                // 失效则销毁并继续尝试
                destroyConnection(name);
                locker.relock();
            }

            // 无空闲：若未达上限则新建
            if (totalConnections_.loadRelaxed() < maxConnections_) {
                locker.unlock();
                QString name = createConnection();
                if (!name.isEmpty()) {
                    inUse_.fetchAndAddRelaxed(1);
                    return name;
                }
                // 创建失败（如网络不通/驱动缺失），退避后重试，直到超时
                if (t.elapsed() >= timeoutMs) {
                    log(4, QStringLiteral("[pool] acquire timeout (create failed)"));
                    return QString();
                }
                QThread::msleep(300);
                continue;
            }

            // 达上限：等待归还
            if (t.elapsed() >= timeoutMs) {
                log(4, QStringLiteral("[pool] acquire timeout (max=%1 reached)")
                           .arg(maxConnections_));
                return QString();
            }
            condition_.wait(&mutex_, timeoutMs - static_cast<unsigned long>(t.elapsed()));
        }
    }
}

void ConnectionPool::release(const QString& connName) {
    if (connName.isEmpty()) return;
    inUse_.fetchAndAddRelaxed(-1);
    {
        QMutexLocker locker(&mutex_);
        Qt::HANDLE tid = QThread::currentThreadId();
        freePerThread_[tid].enqueue(connName);
        lastUsedMs_[connName] = QDateTime::currentMSecsSinceEpoch();
    }
    condition_.wakeOne();
}

void ConnectionPool::shutdown() {
    {
        QMutexLocker locker(&mutex_);
        closed_ = true;
    }
    condition_.wakeAll();

    // 销毁所有连接（注意：只能在创建线程销毁；这里尽力而为）
    // 由于线程可能已退出，逐个尝试从当前线程 removeDatabase 会触发 Qt 警告，
    // 因此仅记录剩余计数，由进程退出时 Qt 自动清理。
    log(1, QStringLiteral("[pool] shutdown, remaining connections=%1")
              .arg(totalConnections_.loadRelaxed()));
}

} // namespace dbclient
