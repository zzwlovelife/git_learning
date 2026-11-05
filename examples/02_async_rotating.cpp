#include <spdlog/spdlog.h>
#include <spdlog/async.h>
#include <spdlog/sinks/rotating_file_sink.h>

#include <filesystem>
#include <memory>

int main() {
    std::filesystem::create_directories("logs");

    spdlog::init_thread_pool(8192, 1);

    auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
        "logs/async_app.log", 10 * 1024 * 1024, 5);

    auto logger = std::make_shared<spdlog::async_logger>(
        "async_app", rotating_sink, spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%t] %v");
    logger->set_level(spdlog::level::info);
    logger->flush_on(spdlog::level::err);

    for (int i = 0; i < 100; ++i) {
        logger->info("async message #{}", i);
    }

    spdlog::shutdown();
    return 0;
}
