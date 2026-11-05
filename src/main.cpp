#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/daily_file_sink.h>

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct Point {
    int x;
    int y;
};

// 让自定义类型支持 fmt 的 "{}" 格式化
template <>
struct fmt::formatter<Point> {
    constexpr fmt::format_parse_context::iterator parse(fmt::format_parse_context &ctx) {
        return ctx.begin();
    }

    fmt::format_context::iterator format(const Point &p, fmt::format_context &ctx) const {
        return fmt::format_to(ctx.out(), "({}, {})", p.x, p.y);
    }
};

int main() {
    try {
        std::filesystem::create_directories("logs");

        // 自定义日志写入失败时的处理
        spdlog::set_error_handler([](const std::string &msg) {
            std::fprintf(stderr, "[spdlog] error: %s\n", msg.c_str());
        });

        // 初始化异步线程池：队列容量 8192，后台线程 1 个
        spdlog::init_thread_pool(8192, 1);
        // 每 3 秒自动刷盘一次
        spdlog::flush_every(std::chrono::seconds(3));

        // 控制台 sink：debug 及以上
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_level(spdlog::level::debug);

        // 轮转文件 sink：单文件 10MB，保留 5 个历史文件，info 及以上
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            "logs/app.log", 10 * 1024 * 1024, 5);
        rotating_sink->set_level(spdlog::level::info);

        // 一个 logger 挂多个 sink
        std::vector<spdlog::sink_ptr> sinks{console_sink, rotating_sink};
        auto logger = std::make_shared<spdlog::async_logger>(
            "app", sinks.begin(), sinks.end(), spdlog::thread_pool(),
            spdlog::async_overflow_policy::block);

        // 时间、logger 名、彩色级别、线程 ID、正文
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
        logger->set_level(spdlog::level::debug);
        // err 及以上立即刷盘
        logger->flush_on(spdlog::level::err);
        // 保留最近 16 条日志用于 backtrace
        logger->enable_backtrace(16);

        // 注册到全局表，之后可用 spdlog::get("app") 获取
        spdlog::register_logger(logger);

        logger->trace("trace message (filtered out)");   // 低于 debug，被过滤
        logger->debug("debug message");
        logger->info("int={}, hex={:#x}, float={:.2f}", 42, 42, 3.14159);
        logger->warn("warn message");
        logger->error("error message");
        logger->info("custom type: {}", Point{1, 2});

        // 带源码位置的日志（配合 pattern 的 %@ 可输出文件:行号）
        SPDLOG_LOGGER_INFO(logger, "message with source location");

        // 出错时把缓存的上下文一次性打印出来
        logger->dump_backtrace();

        // 按天文件：每天 2:30 切分，文件名自动带日期
        auto daily = spdlog::daily_logger_mt("daily", "logs/daily.log", 2, 30);
        daily->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
        daily->flush_on(spdlog::level::warn);
        daily->info("daily file demo");

        // 从全局注册表取回 logger
        auto same_logger = spdlog::get("app");
        if (same_logger) {
            same_logger->info("logger retrieved from registry");
        }

        // 异步模式必须：等待队列写完并停止后台线程
        spdlog::shutdown();
        spdlog::drop_all();
    } catch (const spdlog::spdlog_ex &ex) {
        std::fprintf(stderr, "spdlog init failed: %s\n", ex.what());
        return 1;
    }
    return 0;
}
