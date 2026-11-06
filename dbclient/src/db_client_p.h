#ifndef DBCLIENT_DB_CLIENT_P_H
#define DBCLIENT_DB_CLIENT_P_H

#include <memory>
#include <dbclient/db_client.h>
#include <dbclient/db_config.h>
#include "connection_pool.h"

namespace dbclient {

class DbClientPrivate {
public:
    DbClientConfig config;
    std::unique_ptr<ConnectionPool> pool;
    LogCallback logCb;
    bool opened = false;

    void log(int level, const QString& msg) {
        if (logCb) logCb(level, msg);
    }

    // 带重试的执行器：fn 返回 DbResult；瞬时错误会重试
    DbResult runWithRetry(const std::function<DbResult(QSqlDatabase&)>& fn);
};

} // namespace dbclient

#endif // DBCLIENT_DB_CLIENT_P_H
