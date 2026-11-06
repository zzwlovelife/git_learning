// dbclient C++ API 使用示例
// 演示：spdlog 日志回调接入、连接池、执行查询/更新、事务、批量插入
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <QCoreApplication>
#include <QDateTime>
#include <QSqlQuery>
#include <QSqlError>

#include <dbclient/db_client.h>

using namespace dbclient;

// 把 dbclient 的日志回调接到 spdlog
static void setupDbLogger(DbClient& db, const std::shared_ptr<spdlog::logger>& logger) {
    db.setLogger([logger](int level, const QString& msg) {
        const QByteArray ba = msg.toUtf8();
        const char* s = ba.constData();
        switch (level) {
            case DbClient::Trace:    logger->trace("{}", s); break;
            case DbClient::Debug:    logger->debug("{}", s); break;
            case DbClient::Info:     logger->info("{}", s); break;
            case DbClient::Warn:     logger->warn("{}", s); break;
            case DbClient::Error:    logger->error("{}", s); break;
            case DbClient::Critical: logger->critical("{}", s); break;
            default:                 logger->info("{}", s); break;
        }
    });
}

// 针对达梦（QODBC）的配置
static DbClientConfig dmConfig() {
    DbClientConfig c;
    c.qtDriver = "QODBC";
    c.driver = "DM8 ODBC DRIVER";
    c.host = "127.0.0.1";
    c.port = 5236;
    c.user = "SYSDBA";
    c.password = "SYSDBA";
    c.charset = "UTF8";
    c.maxConnections = 4;
    c.acquireTimeoutMs = 3000;
    c.connectTimeoutSec = 3;
    c.maxRetries = 2;
    return c;
}

// 针对 SQLite（QSQLITE）的配置，用于在无达梦环境下演示完整成功路径
// 注意：SQLite :memory: 每个连接独立，连接池下数据不共享，因此用文件库演示
static DbClientConfig sqliteConfig() {
    DbClientConfig c;
    c.qtDriver = "QSQLITE";
    c.database = "/tmp/dbclient_demo.sqlite";
    c.maxConnections = 2;
    c.acquireTimeoutMs = 2000;
    c.maxRetries = 1;
    return c;
}

static void runScenario(DbClient& db, const QString& name) {
    auto logger = spdlog::get("dbclient");
    logger->info("========== 场景：{} ==========", name.toStdString());

    if (!db.open()) {
        logger->error("[{}] open failed", name.toStdString());
        return;
    }
    logger->info("[{}] open ok", name.toStdString());

    // 建表
    {
        DbResult r = db.executeUpdate(
            "CREATE TABLE IF NOT EXISTS users ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " name TEXT NOT NULL,"
            " age INTEGER)");
        logger->info("[{}] create table ok={} err={}", name.toStdString(),
                     r.ok, r.errorText.toStdString());
    }

    // 单条插入（参数化）
    {
        DbResult r = db.executeUpdate(
            "INSERT INTO users(name, age) VALUES(?, ?)",
            {"Alice", 28});
        logger->info("[{}] insert1 ok={} affected={} err={}", name.toStdString(),
                     r.ok, r.affectedRows, r.errorText.toStdString());
    }

    // 批量插入
    {
        QList<QVariantList> rows = {
            {"Bob", 32},
            {"Carol", 25},
            {"David", 40},
        };
        DbResult r = db.batchInsert(
            "INSERT INTO users(name, age) VALUES(?, ?)", rows);
        logger->info("[{}] batchInsert ok={} affected={} err={}", name.toStdString(),
                     r.ok, r.affectedRows, r.errorText.toStdString());
    }

    // 查询
    {
        DbResult r = db.execute("SELECT id, name, age FROM users ORDER BY id");
        if (r.ok) {
            while (r.query.next()) {
                logger->info("[{}] row: id={} name={} age={}",
                             name.toStdString(),
                             r.query.value(0).toLongLong(),
                             r.query.value(1).toString().toStdString(),
                             r.query.value(2).toInt());
            }
        } else {
            logger->error("[{}] select failed: {}", name.toStdString(),
                          r.errorText.toStdString());
        }
    }

    // 事务：成功提交
    // 注意：事务内必须使用回调传入的 txDb 执行 SQL，否则会从池中取另一条连接，
    // 导致语句不在该事务内执行。
    {
        DbResult r = db.transaction([&](QSqlDatabase& txDb) -> DbResult {
            QSqlQuery q1(txDb);
            q1.prepare("INSERT INTO users(name, age) VALUES(?, ?)");
            q1.addBindValue("Eve"); q1.addBindValue(29);
            if (!q1.exec()) return DbResult::failure(
                q1.lastError().text(), q1.lastError().nativeErrorCode(), 0);

            QSqlQuery q2(txDb);
            q2.prepare("UPDATE users SET age = age + 1 WHERE name = ?");
            q2.addBindValue("Eve");
            if (!q2.exec()) return DbResult::failure(
                q2.lastError().text(), q2.lastError().nativeErrorCode(), 0);
            return DbResult::success();
        });
        logger->info("[{}] transaction(commit) ok={} err={}", name.toStdString(),
                     r.ok, r.errorText.toStdString());
    }

    // 事务：失败回滚
    {
        DbResult r = db.transaction([&](QSqlDatabase& txDb) -> DbResult {
            QSqlQuery q(txDb);
            q.prepare("INSERT INTO users(name, age) VALUES(?, ?)");
            q.addBindValue("Frank"); q.addBindValue(50);
            q.exec();
            // 故意返回失败触发回滚
            return DbResult::failure("intentional rollback");
        });
        logger->info("[{}] transaction(rollback) ok={} err={}", name.toStdString(),
                     r.ok, r.errorText.toStdString());
    }

    // 验证 Frank 未写入
    {
        DbResult r = db.execute("SELECT COUNT(*) FROM users WHERE name = ?", {"Frank"});
        if (r.ok && r.query.next()) {
            logger->info("[{}] Frank count after rollback = {}", name.toStdString(),
                         r.query.value(0).toInt());
        }
    }

    db.close();
    logger->info("[{}] closed", name.toStdString());
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // 初始化 spdlog：控制台 + 轮转文件
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/dbclient.log", 5 * 1024 * 1024, 3);
    auto logger = std::make_shared<spdlog::logger>("dbclient",
        spdlog::sinks_init_list{console, file});
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [dbclient] %v");
    spdlog::register_logger(logger);

    logger->info("dbclient demo start (built at {} {})", __DATE__, __TIME__);

    // ---- 场景 1：达梦（QODBC）；无达梦实例时会连接失败，演示错误处理与重试 ----
    {
        DbClient db(dmConfig());
        setupDbLogger(db, logger);
        runScenario(db, "DM(QODBC)");
    }

    // ---- 场景 2：SQLite（QSQLITE）内存库，演示完整成功路径 ----
    {
        DbClient db(sqliteConfig());
        setupDbLogger(db, logger);
        runScenario(db, "SQLite(QSQLITE)");
    }

    logger->info("dbclient demo done");
    spdlog::shutdown();
    return 0;
}
