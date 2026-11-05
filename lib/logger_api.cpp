#include "logger_api.h"

#include <spdlog/async.h>
#include <spdlog/details/thread_pool.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <filesystem>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

std::mutex g_mutex;
std::shared_ptr<spdlog::logger> g_logger;
std::shared_ptr<spdlog::details::thread_pool> g_pool;

spdlog::level::level_enum to_spdlog_level(logger_level_t level) {
    switch (level) {
    case LOGGER_LEVEL_DEBUG:
        return spdlog::level::debug;
    case LOGGER_LEVEL_WARN:
        return spdlog::level::warn;
    case LOGGER_LEVEL_ERROR:
        return spdlog::level::err;
    case LOGGER_LEVEL_INFO:
    default:
        return spdlog::level::info;
    }
}

}  // namespace

extern "C" int logger_init(const char *log_dir) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_logger) {
        return 0;
    }

    try {
        std::filesystem::create_directories(log_dir);

        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            std::string(log_dir) + "/lib.log", 10 * 1024 * 1024, 5);

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};

        // 使用库私有的线程池，避免和宿主进程的 spdlog 全局状态冲突
        g_pool = std::make_shared<spdlog::details::thread_pool>(8192, 1);
        g_logger = std::make_shared<spdlog::async_logger>(
            "lib", sinks.begin(), sinks.end(), g_pool,
            spdlog::async_overflow_policy::block);

        g_logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [%t] %v");
        g_logger->set_level(spdlog::level::debug);
        g_logger->flush_on(spdlog::level::err);
        return 0;
    } catch (const std::exception &) {
        g_logger.reset();
        g_pool.reset();
        return -1;
    }
}

extern "C" void logger_log(logger_level_t level, const char *message) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (!g_logger || message == nullptr) {
        return;
    }

    try {
        g_logger->log(to_spdlog_level(level), "{}", message);
    } catch (...) {
        // 日志失败不能抛出异常跨越 C ABI 边界
    }
}

extern "C" void logger_flush(void) {
    std::lock_guard<std::mutex> lock(g_mutex);
    if (g_logger) {
        g_logger->flush();
    }
}

extern "C" void logger_shutdown(void) {
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_logger) {
        g_logger->flush();
        g_logger.reset();
    }
    // thread_pool 析构时会投递终止消息并 join 后台线程
    g_pool.reset();
}
