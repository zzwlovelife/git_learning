#ifndef DBCLIENT_SCOPED_CONNECTION_H
#define DBCLIENT_SCOPED_CONNECTION_H

#include <QSqlDatabase>
#include <QString>

namespace dbclient {

class ConnectionPool;

// RAII 连接句柄：从池中取出，析构时自动归还
class ScopedConnection {
public:
    explicit ScopedConnection(ConnectionPool& pool, int timeoutMs = -1);
    ~ScopedConnection();

    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection& operator=(const ScopedConnection&) = delete;

    QSqlDatabase& db() { return db_; }
    bool valid() const { return !connName_.isEmpty() && db_.isOpen(); }

private:
    ConnectionPool& pool_;
    QString connName_;
    QSqlDatabase db_;
};

} // namespace dbclient

#endif // DBCLIENT_SCOPED_CONNECTION_H
