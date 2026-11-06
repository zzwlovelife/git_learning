#include <dbclient/db_client_c.h>

#include <cstring>
#include <string>
#include <vector>

#include <dbclient/db_client.h>

using namespace dbclient;

// 不透明句柄的真实结构
struct dbclient_s {
    DbClient* client = nullptr;
    dbclient_log_cb logCb = nullptr;
    void* userData = nullptr;
};

extern "C" {

void dbclient_config_init(dbclient_config_t* cfg) {
    if (!cfg) return;
    cfg->qt_driver = "QODBC";
    cfg->driver = "DM8 ODBC DRIVER";
    cfg->host = "127.0.0.1";
    cfg->port = 5236;
    cfg->user = "SYSDBA";
    cfg->password = "SYSDBA";
    cfg->database = nullptr;
    cfg->charset = "UTF8";
    cfg->max_connections = 10;
    cfg->acquire_timeout_ms = 3000;
    cfg->idle_timeout_sec = 300;
    cfg->connect_timeout_sec = 5;
    cfg->max_retries = 3;
}

dbclient_t* dbclient_create(const dbclient_config_t* cfg) {
    if (!cfg) return nullptr;

    DbClientConfig c;
    c.qtDriver = QString::fromUtf8(cfg->qt_driver ? cfg->qt_driver : "QODBC");
    c.driver = QString::fromUtf8(cfg->driver ? cfg->driver : "DM8 ODBC DRIVER");
    c.host = QString::fromUtf8(cfg->host ? cfg->host : "127.0.0.1");
    c.port = cfg->port > 0 ? cfg->port : 5236;
    c.user = QString::fromUtf8(cfg->user ? cfg->user : "SYSDBA");
    c.password = QString::fromUtf8(cfg->password ? cfg->password : "SYSDBA");
    if (cfg->database) c.database = QString::fromUtf8(cfg->database);
    c.charset = QString::fromUtf8(cfg->charset ? cfg->charset : "UTF8");
    c.maxConnections = cfg->max_connections > 0 ? cfg->max_connections : 10;
    c.acquireTimeoutMs = cfg->acquire_timeout_ms > 0 ? cfg->acquire_timeout_ms : 3000;
    c.idleTimeoutSec = cfg->idle_timeout_sec > 0 ? cfg->idle_timeout_sec : 300;
    c.connectTimeoutSec = cfg->connect_timeout_sec > 0 ? cfg->connect_timeout_sec : 5;
    c.maxRetries = cfg->max_retries >= 0 ? cfg->max_retries : 3;

    dbclient_t* h = new (std::nothrow) dbclient_t;
    if (!h) return nullptr;
    h->client = new DbClient(c);
    return h;
}

void dbclient_destroy(dbclient_t* h) {
    if (!h) return;
    delete h->client;
    delete h;
}

void dbclient_set_logger(dbclient_t* h, dbclient_log_cb cb, void* user_data) {
    if (!h || !h->client) return;
    h->logCb = cb;
    h->userData = user_data;
    if (cb) {
        h->client->setLogger([h](int level, const QString& msg) {
            QByteArray ba = msg.toUtf8();
            h->logCb(level, ba.constData(), h->userData);
        });
    } else {
        h->client->setLogger(LogCallback());
    }
}

int dbclient_open(dbclient_t* h) {
    if (!h || !h->client) return 0;
    return h->client->open() ? 1 : 0;
}

void dbclient_close(dbclient_t* h) {
    if (!h || !h->client) return;
    h->client->close();
}

int dbclient_is_open(dbclient_t* h) {
    if (!h || !h->client) return 0;
    return h->client->isOpen() ? 1 : 0;
}

static QVariantList toVariantList(const char* const* params, int param_count) {
    QVariantList list;
    if (params) {
        for (int i = 0; i < param_count; ++i) {
            list << QString::fromUtf8(params[i] ? params[i] : "");
        }
    }
    return list;
}

static void fillResult(dbclient_result_t* out, const DbResult& r) {
    if (!out) return;
    out->ok = r.ok ? 1 : 0;
    qstrncpy(out->error_text, r.errorText.toUtf8().constData(), sizeof(out->error_text) - 1);
    out->error_text[sizeof(out->error_text) - 1] = '\0';
    qstrncpy(out->sql_state, r.sqlState.toUtf8().constData(), sizeof(out->sql_state) - 1);
    out->sql_state[sizeof(out->sql_state) - 1] = '\0';
    out->native_error_code = r.nativeErrorCode;
    out->affected_rows = static_cast<int64_t>(r.affectedRows);
}

dbclient_result_t dbclient_execute(dbclient_t* h, const char* sql,
                                   const char* const* params, int param_count) {
    dbclient_result_t out;
    std::memset(&out, 0, sizeof(out));
    if (!h || !h->client || !sql) {
        out.ok = 0;
        qstrncpy(out.error_text, "invalid argument", sizeof(out.error_text) - 1);
        return out;
    }
    DbResult r = h->client->execute(QString::fromUtf8(sql), toVariantList(params, param_count));
    fillResult(&out, r);
    return out;
}

dbclient_result_t dbclient_execute_update(dbclient_t* h, const char* sql,
                                          const char* const* params, int param_count) {
    dbclient_result_t out;
    std::memset(&out, 0, sizeof(out));
    if (!h || !h->client || !sql) {
        out.ok = 0;
        qstrncpy(out.error_text, "invalid argument", sizeof(out.error_text) - 1);
        return out;
    }
    DbResult r = h->client->executeUpdate(QString::fromUtf8(sql), toVariantList(params, param_count));
    fillResult(&out, r);
    return out;
}

} // extern "C"
