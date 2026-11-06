#include "scoped_connection.h"
#include "connection_pool.h"

namespace dbclient {

ScopedConnection::ScopedConnection(ConnectionPool& pool, int timeoutMs)
    : pool_(pool) {
    connName_ = pool.acquire(timeoutMs);
    if (!connName_.isEmpty()) {
        db_ = QSqlDatabase::database(connName_, false);
    }
}

ScopedConnection::~ScopedConnection() {
    if (!connName_.isEmpty()) {
        // 归还前如果连接已失效，由 release/pool 内部处理
        pool_.release(connName_);
    }
}

} // namespace dbclient
