#ifndef DBCLIENT_ERROR_CLASSIFIER_H
#define DBCLIENT_ERROR_CLASSIFIER_H

#include <QString>

namespace dbclient {

// 基于 ODBC SQLSTATE 的错误分类
// 参考：https://en.wikipedia.org/wiki/SQLSTATE
namespace ErrorClassifier {

// 是否为瞬时/连接类错误，可重试
inline bool isRetryable(const QString& sqlState) {
    if (sqlState.isEmpty()) return false;
    const QString cls = sqlState.left(2);
    // 08: connection exception
    // 40: transaction rollback
    // 53: insufficient resources
    // HY000 general error (some transient)
    // 01: warning (not retryable usually, but harmless)
    return cls == "08" || cls == "40" || cls == "53";
}

// 是否为连接错误（需要重建连接后重试）
inline bool isConnectionError(const QString& sqlState) {
    return sqlState.left(2) == "08";
}

// 严重错误，不应重试
inline bool isPermanent(const QString& sqlState) {
    if (sqlState.isEmpty()) return true;
    const QString cls = sqlState.left(2);
    // 23: integrity constraint
    // 42: syntax error or access rule
    // 44: with check option
    return cls == "23" || cls == "42" || cls == "44";
}

} // namespace ErrorClassifier

} // namespace dbclient

#endif // DBCLIENT_ERROR_CLASSIFIER_H
