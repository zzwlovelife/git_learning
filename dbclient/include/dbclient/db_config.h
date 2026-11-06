#ifndef DBCLIENT_DB_CONFIG_H
#define DBCLIENT_DB_CONFIG_H

#include <QString>
#include <QVariant>

namespace dbclient {

// 数据库客户端配置
struct DBCLIENT_API DbClientConfig {
    QString qtDriver = "QODBC";      // Qt SQL 驱动：QODBC（达梦/MySQL/PG 等）或 QSQLITE
    // ODBC 驱动名（Windows 用 {DM8 ODBC DRIVER}，Linux 用 DM8 ODBC DRIVER）
    QString driver = "DM8 ODBC DRIVER";
    QString host = "127.0.0.1";
    int port = 5236;
    QString user = "SYSDBA";
    QString password = "SYSDBA";
    QString database;            // 可选，留空时不指定
    QString charset = "UTF8";    // 显式指定字符集，避免中文乱码

    int maxConnections = 10;     // 连接池最大连接数
    int acquireTimeoutMs = 3000; // 获取连接超时（毫秒）
    int idleTimeoutSec = 300;    // 空闲连接超过该秒数则回收
    int connectTimeoutSec = 5;   // 单次连接超时（秒）
    int maxRetries = 3;          // 瞬时错误最大重试次数

    // 生成 ODBC 无 DSN 连接串
    QString toConnectionString() const {
        QString s = QStringLiteral("Driver=%1;Server=%2;Port=%3;UID=%4;PWD=%5;CHARSET=%6;")
                        .arg(driver.isEmpty() ? QStringLiteral("{DM8 ODBC DRIVER}") : driver,
                             host, QString::number(port), user, password, charset);
        if (!database.isEmpty()) {
            s += QStringLiteral("DATABASE=%1;").arg(database);
        }
        return s;
    }
};

} // namespace dbclient

#endif // DBCLIENT_DB_CONFIG_H
