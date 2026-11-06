#ifndef DBCLIENT_DB_CLIENT_H
#define DBCLIENT_DB_CLIENT_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QVariantList>
#include <functional>

#include "dbclient_global.h"
#include "db_config.h"
#include "db_result.h"

namespace dbclient {

class DbClientPrivate;

// 日志回调：level 与 spdlog 对齐（0 trace ... 5 critical）
using LogCallback = std::function<void(int level, const QString& msg)>;

// 数据库客户端：连接池 + 健康检查 + 重试 + RAII + 事务
class DBCLIENT_API DbClient {
public:
    enum LogLevel {
        Trace = 0,
        Debug = 1,
        Info = 2,
        Warn = 3,
        Error = 4,
        Critical = 5
    };

    explicit DbClient(const DbClientConfig& config);
    ~DbClient();

    DbClient(const DbClient&) = delete;
    DbClient& operator=(const DbClient&) = delete;

    // 设置日志回调（可选，默认输出到 qWarning）
    void setLogger(LogCallback cb);

    // 初始化连接池（创建最小连接数并连通性测试）
    bool open();

    // 关闭连接池，释放所有连接
    void close();

    bool isOpen() const;

    // 执行查询 SQL，返回结果集
    DbResult execute(const QString& sql, const QVariantList& params = QVariantList());

    // 执行 INSERT/UPDATE/DELETE，返回影响行数（在 result.affectedRows）
    DbResult executeUpdate(const QString& sql, const QVariantList& params = QVariantList());

    // 事务：fn 内返回 true 则 commit，返回 false 或抛异常则 rollback
    DbResult transaction(const std::function<DbResult(QSqlDatabase&)>& fn);

    // 批量插入：rows 为二维数组，每行为一条记录的字段值
    // 示例：insert into t(a,b) values(?,?)  rows 传 [[1,"x"],[2,"y"]]
    DbResult batchInsert(const QString& sql, const QList<QVariantList>& rows);

private:
    DbClientPrivate* d_ptr;
    Q_DECLARE_PRIVATE(DbClient)
};

} // namespace dbclient

#endif // DBCLIENT_DB_CLIENT_H
