#include <dbclient/db_client.h>
#include "db_client_p.h"

#include <QSqlQuery>
#include <QSqlError>
#include <QThread>

#include "scoped_connection.h"
#include "error_classifier.h"

namespace dbclient {

DbClient::DbClient(const DbClientConfig& config)
    : d_ptr(new DbClientPrivate) {
    Q_D(DbClient);
    d->config = config;
}

DbClient::~DbClient() {
    close();
}

void DbClient::setLogger(LogCallback cb) {
    Q_D(DbClient);
    d->logCb = std::move(cb);
}

bool DbClient::open() {
    Q_D(DbClient);
    if (d->opened) return true;

    d->log(Info, QStringLiteral("[dbclient] opening pool, driver=%1, db=%2")
                     .arg(d->config.qtDriver).arg(d->config.database));

    // 根据 Qt 驱动选择 databaseName：QODBC 用完整连接串，其他用库名/路径
    QString dbName = d->config.qtDriver == "QODBC"
                         ? d->config.toConnectionString()
                         : (d->config.database.isEmpty()
                                ? QStringLiteral(":memory:")
                                : d->config.database);

    d->pool = std::make_unique<ConnectionPool>(
        d->config.qtDriver,
        dbName,
        d->config.maxConnections,
        d->config.acquireTimeoutMs,
        d->config.idleTimeoutSec,
        d->config.connectTimeoutSec,
        [d](int lvl, const QString& m) { d->log(lvl, m); });

    // 连通性测试：取一个连接并 ping
    ScopedConnection conn(*d->pool, d->config.connectTimeoutSec * 1000);
    if (!conn.valid()) {
        d->log(Error, QStringLiteral("[dbclient] connectivity check failed, pool will not open"));
        d->pool->shutdown();
        d->pool.reset();
        return false;
    }

    d->opened = true;
    d->log(Info, QStringLiteral("[dbclient] pool opened, maxConnections=%1")
                     .arg(d->config.maxConnections));
    return true;
}

void DbClient::close() {
    Q_D(DbClient);
    if (d->pool) {
        d->pool->shutdown();
        d->pool.reset();
    }
    d->opened = false;
    d->log(Info, QStringLiteral("[dbclient] pool closed"));
}

bool DbClient::isOpen() const {
    Q_D(const DbClient);
    return d->opened;
}

DbResult DbClientPrivate::runWithRetry(const std::function<DbResult(QSqlDatabase&)>& fn) {
    int attempt = 0;
    while (true) {
        ScopedConnection conn(*pool, config.acquireTimeoutMs);
        if (!conn.valid()) {
            return DbResult::failure(QStringLiteral("failed to acquire connection"));
        }

        DbResult r = fn(conn.db());
        if (r.ok) return r;

        if (attempt < config.maxRetries && ErrorClassifier::isRetryable(r.sqlState)) {
            int backoff = 100 * (1 << attempt);  // 100, 200, 400, ...
            log(DbClient::Warn,
                QStringLiteral("[dbclient] attempt %1 failed (sqlstate=%2, %3), retry in %4ms")
                    .arg(attempt).arg(r.sqlState, r.errorText).arg(backoff));
            QThread::msleep(static_cast<unsigned long>(backoff));
            ++attempt;
            continue;
        }

        log(DbClient::Error,
            QStringLiteral("[dbclient] operation failed (sqlstate=%1, %2)")
                .arg(r.sqlState, r.errorText));
        return r;
    }
}

DbResult DbClient::execute(const QString& sql, const QVariantList& params) {
    Q_D(DbClient);
    return d->runWithRetry([&](QSqlDatabase& db) -> DbResult {
        QSqlQuery q(db);
        if (!q.prepare(sql)) {
            return DbResult::failure(q.lastError().text(),
                                     q.lastError().nativeErrorCode(), 0);
        }
        for (const QVariant& v : params) {
            q.addBindValue(v);
        }
        if (!q.exec()) {
            return DbResult::failure(q.lastError().text(),
                                     q.lastError().nativeErrorCode(), 0);
        }
        return DbResult::success(std::move(q));
    });
}

DbResult DbClient::executeUpdate(const QString& sql, const QVariantList& params) {
    Q_D(DbClient);
    return d->runWithRetry([&](QSqlDatabase& db) -> DbResult {
        QSqlQuery q(db);
        if (!q.prepare(sql)) {
            return DbResult::failure(q.lastError().text(),
                                     q.lastError().nativeErrorCode(), 0);
        }
        for (const QVariant& v : params) {
            q.addBindValue(v);
        }
        if (!q.exec()) {
            return DbResult::failure(q.lastError().text(),
                                     q.lastError().nativeErrorCode(), 0);
        }
        DbResult r = DbResult::success(std::move(q));
        d->log(Info, QStringLiteral("[dbclient] executeUpdate affected rows=%1").arg(r.affectedRows));
        return r;
    });
}

DbResult DbClient::transaction(const std::function<DbResult(QSqlDatabase&)>& fn) {
    Q_D(DbClient);
    return d->runWithRetry([&](QSqlDatabase& db) -> DbResult {
        if (!db.transaction()) {
            return DbResult::failure(QStringLiteral("transaction begin failed: %1")
                                         .arg(db.lastError().text()));
        }

        DbResult r;
        bool commit = false;
        try {
            r = fn(db);
            commit = r.ok;
        } catch (const std::exception& e) {
            r = DbResult::failure(QStringLiteral("exception in transaction: %1").arg(e.what()));
            commit = false;
        } catch (...) {
            r = DbResult::failure(QStringLiteral("unknown exception in transaction"));
            commit = false;
        }

        if (commit) {
            if (!db.commit()) {
                return DbResult::failure(QStringLiteral("commit failed: %1")
                                             .arg(db.lastError().text()));
            }
        } else {
            db.rollback();
            d->log(Warn, QStringLiteral("[dbclient] transaction rolled back: %1").arg(r.errorText));
        }
        return r;
    });
}

DbResult DbClient::batchInsert(const QString& sql, const QList<QVariantList>& rows) {
    Q_D(DbClient);
    if (rows.isEmpty()) {
        return DbResult::success();
    }

    return d->runWithRetry([&](QSqlDatabase& db) -> DbResult {
        QSqlQuery q(db);
        if (!q.prepare(sql)) {
            return DbResult::failure(q.lastError().text(),
                                     q.lastError().nativeErrorCode(), 0);
        }

        const int cols = rows.first().size();
        QList<QVariantList> columns(cols);
        for (const QVariantList& row : rows) {
            for (int i = 0; i < cols; ++i) {
                columns[i].append(row.value(i));
            }
        }
        for (int i = 0; i < cols; ++i) {
            q.addBindValue(columns[i]);
        }

        if (q.execBatch()) {
            DbResult r = DbResult::success(std::move(q));
            d->log(Info, QStringLiteral("[dbclient] batchInsert rows=%1").arg(rows.size()));
            return r;
        }

        // execBatch 失败时退回逐条执行（部分 ODBC 驱动对批处理支持不佳）
        d->log(Warn, QStringLiteral("[dbclient] execBatch failed (%1), fallback to row-by-row")
                       .arg(q.lastError().text()));
        qint64 total = 0;
        for (const QVariantList& row : rows) {
            QSqlQuery q2(db);
            if (!q2.prepare(sql)) {
                return DbResult::failure(q2.lastError().text(),
                                         q2.lastError().nativeErrorCode(), 0);
            }
            for (const QVariant& v : row) q2.addBindValue(v);
            if (!q2.exec()) {
                return DbResult::failure(q2.lastError().text(),
                                         q2.lastError().nativeErrorCode(), 0);
            }
            total += q2.numRowsAffected();
        }
        DbResult r = DbResult::success();
        r.affectedRows = total;
        return r;
    });
}

} // namespace dbclient
