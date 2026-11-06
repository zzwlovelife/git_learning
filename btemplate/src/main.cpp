// 主线程职责：
//   1. 初始化日志（spdlog）
//   2. 初始化数据库（dbclient）
//   3. 初始化通信（netclient）
//   4. 加载并启动各业务线程
//   5. 运行事件循环，处理信号（如优雅退出）
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "bthread/thread_manager.h"
#include "bthread/example_threads.h"

using namespace bthread;

// ---------- 日志初始化 ----------
static std::shared_ptr<spdlog::logger> initLog() {
    auto console = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    auto file = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/btemplate.log", 5 * 1024 * 1024, 3);
    auto logger = std::make_shared<spdlog::logger>(
        "app", spdlog::sinks_init_list{console, file});
    logger->set_level(spdlog::level::trace);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
    logger->flush_on(spdlog::level::info);  // 确保每条日志立即写入文件
    spdlog::register_logger(logger);
    return logger;
}

// ---------- 数据库初始化（占位，实际项目接入 dbclient）----------
static bool initDatabase() {
    // 实际项目：
    //   DbClientConfig cfg; cfg.qtDriver = "QODBC"; ...
    //   db_ = new DbClient(cfg); return db_->open();
    spdlog::get("app")->info("[main] database initialized (dbclient)");
    return true;
}

// ---------- 通信初始化（占位，实际项目接入 netclient）----------
static bool initCommunication() {
    // 实际项目：
    //   net_ = new NetClient();
    //   net_->mqtt()->connectToHost(cfg); ...
    spdlog::get("app")->info("[main] communication initialized (netclient)");
    return true;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // 1. 日志
    auto logger = initLog();
    logger->info("=== application start ===");

    // 2. 数据库
    if (!initDatabase()) {
        logger->error("database init failed");
        return 1;
    }

    // 3. 通信
    if (!initCommunication()) {
        logger->error("communication init failed");
        return 1;
    }

    // 4. 加载业务线程
    ThreadManager manager;
    auto* acquire   = new DataAcquireThread("acquire");
    auto* process   = new DataProcessThread("process");
    auto* distribute= new DataDistributeThread("distribute");

    manager.addThread(acquire);
    manager.addThread(process);
    manager.addThread(distribute);

    // 把业务线程的日志统一接入 spdlog
    QObject::connect(&manager, &ThreadManager::threadLog,
                     &app, [logger](const QString& name, const QString& msg) {
        logger->info("[{}] {}", name.toStdString(), msg.toStdString());
    });

    // 全部启动后通知
    QObject::connect(&manager, &ThreadManager::allStarted, [logger]() {
        logger->info("[main] all business threads started");
    });

    manager.startAll();
    logger->info("[main] threads starting...");

    // 演示：运行 8 秒后优雅退出
    QTimer::singleShot(8000, &app, [&]() {
        logger->info("[main] stopping all threads...");
        manager.stopAll();
        logger->info("=== application exit ===");
        app.quit();
    });

    // 5. 主线程事件循环
    int rc = app.exec();

    delete acquire;
    delete process;
    delete distribute;
    spdlog::shutdown();
    return rc;
}
