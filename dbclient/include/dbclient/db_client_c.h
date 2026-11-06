#ifndef DBCLIENT_DB_CLIENT_C_H
#define DBCLIENT_DB_CLIENT_C_H

#include <stdint.h>
#include "dbclient_global.h"

#ifdef __cplusplus
extern "C" {
#endif

// 不透明句柄
typedef struct dbclient_s dbclient_t;

// 日志级别（与 spdlog 对齐）
enum dbclient_log_level {
    DBCLIENT_LOG_TRACE = 0,
    DBCLIENT_LOG_DEBUG = 1,
    DBCLIENT_LOG_INFO = 2,
    DBCLIENT_LOG_WARN = 3,
    DBCLIENT_LOG_ERROR = 4,
    DBCLIENT_LOG_CRITICAL = 5
};

// 日志回调类型
typedef void (*dbclient_log_cb)(int level, const char* msg, void* user_data);

// 配置结构（C 接口）
typedef struct {
    const char* qt_driver;   // Qt 驱动："QODBC"（默认）或 "QSQLITE"
    const char* driver;      // ODBC 驱动名，如 "DM8 ODBC DRIVER"
    const char* host;
    int port;
    const char* user;
    const char* password;
    const char* database;    // 可为 NULL
    const char* charset;     // 默认 "UTF8"
    int max_connections;
    int acquire_timeout_ms;
    int idle_timeout_sec;
    int connect_timeout_sec;
    int max_retries;
} dbclient_config_t;

// 用默认值填充配置
DBCLIENT_API void dbclient_config_init(dbclient_config_t* cfg);

// 执行结果（C 接口）
typedef struct {
    int ok;
    char error_text[1024];
    char sql_state[16];
    int native_error_code;
    int64_t affected_rows;
} dbclient_result_t;

// 创建客户端
DBCLIENT_API dbclient_t* dbclient_create(const dbclient_config_t* cfg);

// 销毁客户端
DBCLIENT_API void dbclient_destroy(dbclient_t* h);

// 设置日志回调
DBCLIENT_API void dbclient_set_logger(dbclient_t* h, dbclient_log_cb cb, void* user_data);

// 打开连接池
DBCLIENT_API int dbclient_open(dbclient_t* h);

// 关闭连接池
DBCLIENT_API void dbclient_close(dbclient_t* h);

// 是否处于打开状态
DBCLIENT_API int dbclient_is_open(dbclient_t* h);

// 执行查询 SQL（参数用 ? 占位）
DBCLIENT_API dbclient_result_t dbclient_execute(dbclient_t* h, const char* sql,
                                                 const char* const* params, int param_count);

// 执行非查询 SQL（INSERT/UPDATE/DELETE），返回影响行数
DBCLIENT_API dbclient_result_t dbclient_execute_update(dbclient_t* h, const char* sql,
                                                        const char* const* params,
                                                        int param_count);

#ifdef __cplusplus
}
#endif

#endif // DBCLIENT_DB_CLIENT_C_H
