// =============================================================================
// 主线程入口：按职责顺序初始化基础设施，加载业务线程，运行事件循环
// -----------------------------------------------------------------------------
// 主线程职责：
//   1. 初始化日志（spdlog：控制台 + 轮转文件）
//   2. 初始化数据库（dbclient —— 此处占位，实际项目接入）
//   3. 初始化通信（netclient —— 此处占位，实际项目接入）
//   4. 创建 ThreadManager，注册并启动各业务线程
//   5. 运行 QCoreApplication 事件循环，处理信号与定时器
// =============================================================================
#include <QCoreApplication>
#include <QTimer>
#include <QDebug>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include "bthread/thread_manager.h"
#include "bthread/example_threads.h"

using namespace bthread;

// -----------------------------------------------------------------------------
// initLog - 初始化 spdlog 日志器（名称 "app"，供业务线程通过 spdlog::get("app") 获取）
// 双 sink：彩色控制台 + 轮转文件（5MB x 3 份）
// flush_on(info) 确保每条日志立即落盘，避免程序异常退出时丢失。
// -----------------------------------------------------------------------------
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

// -----------------------------------------------------------------------------
// initDatabase - 数据库初始化（占位）
// 实际项目中接入 dbclient 库（见 /workspace/dbclient）：
//   DbClientConfig cfg; cfg.qtDriver = "QODBC"; ...
//   db_ = new DbClient(cfg); return db_->open();
// -----------------------------------------------------------------------------
static bool initDatabase() {
    spdlog::get("app")->info("[main] database initialized (dbclient)");
    return true;
}

// -----------------------------------------------------------------------------
// initCommunication - 通信初始化（占位）
// 实际项目中接入 netclient 库（见 /workspace/netclient）：
//   net_ = new NetClient();
//   net_->mqtt()->connectToHost(cfg); ...
// -----------------------------------------------------------------------------
static bool initCommunication() {
    spdlog::get("app")->info("[main] communication initialized (netclient)");
    return true;
}

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);

    // 1. 初始化日志
    auto logger = initLog();
    logger->info("=== application start ===");

    // 2. 初始化数据库
    if (!initDatabase()) {
        logger->error("database init failed");
        return 1;
    }

    // 3. 初始化通信
    if (!initCommunication()) {
        logger->error("communication init failed");
        return 1;
    }

    // 4. 加载业务线程（创建 -> 注册到管理器 -> 管理器注入消息总线）
    ThreadManager manager;
    auto* acquire    = new DataAcquireThread("acquire");     // 数据采集
    auto* process    = new DataProcessThread("process");     // 数据处理
    auto* distribute = new DataDistributeThread("distribute"); // 数据分发

    manager.addThread(acquire);
    manager.addThread(process);
    manager.addThread(distribute);

    // 业务线程日志已由子线程直接写入 spdlog（文件+控制台）。
    // threadLogMessage 仅用于 UI 显示：GUI 程序中接到 QPlainTextEdit 等控件。
    // 本示例为控制台程序，此处留空演示 UI 通道接入点。
    static const char* kLevelNames[] = {"DEBUG", "INFO", "WARN", "ERROR"};
    QObject::connect(&manager, &ThreadManager::threadLogMessage,
                     &app, [](int level, const QString& name, const QString& msg) {
        // 实际 GUI 项目中：
        //   ui->logView->appendPlainText(QString("[%1] [%2] %3")
        //       .arg(kLevelNames[level]).arg(name).arg(msg));
        Q_UNUSED(level); Q_UNUSED(name); Q_UNUSED(msg);
    });

    // 所有线程启动完成后，主线程记录日志
    QObject::connect(&manager, &ThreadManager::allStarted, [logger]() {
        logger->info("[main] all business threads started");
    });

    manager.startAll();
    logger->info("[main] threads starting...");

    // 演示：运行 8 秒后触发优雅退出（stopAll 逆序停止所有线程）
    QTimer::singleShot(8000, &app, [&]() {
        logger->info("[main] stopping all threads...");
        manager.stopAll();
        logger->info("=== application exit ===");
        app.quit();
    });

    // 5. 主线程事件循环（处理信号、定时器、QueuedConnection 消息）
    int rc = app.exec();

    // 清理：线程对象 + spdlog
    delete acquire;
    delete process;
    delete distribute;
    spdlog::shutdown();
    return rc;
}
