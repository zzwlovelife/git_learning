// dbclient C API 使用示例
// 演示：通过 C ABI 接口操作数据库，日志回调接入 spdlog
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <cstdio>
#include <cstring>

#include <dbclient/db_client_c.h>

// C 日志回调：把 dbclient 的日志转发到 spdlog
static void log_to_spdlog(int level, const char* msg, void* user_data) {
    auto logger = static_cast<std::shared_ptr<spdlog::logger>*>(user_data)->get();
    switch (level) {
        case DBCLIENT_LOG_TRACE:    logger->trace("{}", msg); break;
        case DBCLIENT_LOG_DEBUG:    logger->debug("{}", msg); break;
        case DBCLIENT_LOG_INFO:     logger->info("{}", msg); break;
        case DBCLIENT_LOG_WARN:     logger->warn("{}", msg); break;
        case DBCLIENT_LOG_ERROR:    logger->error("{}", msg); break;
        case DBCLIENT_LOG_CRITICAL: logger->critical("{}", msg); break;
        default:                    logger->info("{}", msg); break;
    }
}

static void print_result(const char* op, const dbclient_result_t* r) {
    auto logger = spdlog::get("dbc_c");
    logger->info("[C API] {} ok={} affected={} sqlstate={} err={}",
                 op, r->ok, r->affected_rows, r->sql_state, r->error_text);
}

int main() {
    // spdlog
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/dbclient_c.log", 5 * 1024 * 1024, 3);
    auto logger = std::make_shared<spdlog::logger>("dbc_c",
        spdlog::sinks_init_list{console, file});
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [dbc_c] %v");
    spdlog::register_logger(logger);

    logger->info("C API demo start");

    // 配置：用 SQLite 文件库演示成功路径（达梦同理，改 qt_driver/driver 即可）
    // 注意：不能用 :memory:，因为 SQLite 每个连接有独立的内存库，
    // 连接池的不同连接看不到彼此的数据。
    dbclient_config_t cfg;
    dbclient_config_init(&cfg);
    cfg.qt_driver = "QSQLITE";
    cfg.database = "/tmp/dbclient_c_demo.sqlite";
    cfg.max_connections = 2;

    // 清理旧库，保证每次运行从干净状态开始
    std::remove("/tmp/dbclient_c_demo.sqlite");

    dbclient_t* h = dbclient_create(&cfg);
    if (!h) {
        logger->error("dbclient_create failed");
        return 1;
    }

    // 设置日志回调
    dbclient_set_logger(h, log_to_spdlog, &logger);

    if (!dbclient_open(h)) {
        logger->error("dbclient_open failed");
        dbclient_destroy(h);
        return 1;
    }
    logger->info("dbclient_open ok, is_open={}", dbclient_is_open(h));

    // 建表
    {
        dbclient_result_t r = dbclient_execute_update(
            h,
            "CREATE TABLE IF NOT EXISTS items ("
            " id INTEGER PRIMARY KEY AUTOINCREMENT,"
            " name TEXT NOT NULL,"
            " qty INTEGER)",
            nullptr, 0);
        print_result("create table", &r);
    }

    // 插入（参数化）
    {
        const char* params[] = {"apple", "10"};
        dbclient_result_t r = dbclient_execute_update(
            h, "INSERT INTO items(name, qty) VALUES(?, ?)", params, 2);
        print_result("insert apple", &r);
    }

    // 批量插入（逐条调用，C API 暂未暴露 batchInsert，可在 C 层循环）
    {
        const char* row1[] = {"banana", "5"};
        const char* row2[] = {"cherry", "20"};
        dbclient_result_t r1 = dbclient_execute_update(
            h, "INSERT INTO items(name, qty) VALUES(?, ?)", row1, 2);
        dbclient_result_t r2 = dbclient_execute_update(
            h, "INSERT INTO items(name, qty) VALUES(?, ?)", row2, 2);
        print_result("insert banana", &r1);
        print_result("insert cherry", &r2);
    }

    // 查询
    {
        dbclient_result_t r = dbclient_execute(
            h, "SELECT id, name, qty FROM items ORDER BY id", nullptr, 0);
        print_result("select", &r);
        // C API 暂不直接暴露结果集遍历；如需读取结果集可扩展 C API 或使用 C++ API
        logger->info("[C API] (result set iteration exposed via C++ API)");
    }

    // 验证影响行数
    {
        const char* params[] = {"apple"};
        dbclient_result_t r = dbclient_execute_update(
            h, "UPDATE items SET qty = qty + 1 WHERE name = ?", params, 1);
        print_result("update apple", &r);
    }

    dbclient_close(h);
    logger->info("dbclient_close done, is_open={}", dbclient_is_open(h));
    dbclient_destroy(h);

    logger->info("C API demo done");
    spdlog::shutdown();
    return 0;
}
