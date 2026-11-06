#ifndef DBCLIENT_DB_RESULT_H
#define DBCLIENT_DB_RESULT_H

#include <QString>
#include <QSqlQuery>

namespace dbclient {

// 统一的执行结果，所有操作返回该类型
struct DBCLIENT_API DbResult {
    bool ok = false;
    QString errorText;
    QString sqlState;        // ODBC SQLSTATE，用于错误分类
    int nativeErrorCode = 0;
    qint64 affectedRows = 0; // executeUpdate 的影响行数
    QVariant lastInsertId;   // 自增主键回填（达梦可能无效）
    QSqlQuery query;         // 查询结果集（非查询时为空）

    static DbResult success(QSqlQuery q = QSqlQuery()) {
        DbResult r;
        r.ok = true;
        r.query = std::move(q);
        if (r.query.isActive()) {
            r.affectedRows = r.query.numRowsAffected();
            r.lastInsertId = r.query.lastInsertId();
        }
        return r;
    }

    static DbResult failure(const QString& text, const QString& state = QString(),
                            int code = 0) {
        DbResult r;
        r.ok = false;
        r.errorText = text;
        r.sqlState = state;
        r.nativeErrorCode = code;
        return r;
    }
};

} // namespace dbclient

#endif // DBCLIENT_DB_RESULT_H
