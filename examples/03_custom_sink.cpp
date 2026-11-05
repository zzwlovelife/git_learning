#include <spdlog/spdlog.h>
#include <spdlog/sinks/base_sink.h>

#include <cstdio>
#include <memory>
#include <mutex>
#include <string>

// 自定义 sink：把日志写到标准输出，实际项目中可换成
// 网络、消息队列、数据库、内存环形缓冲区等目标
class my_sink : public spdlog::sinks::base_sink<std::mutex> {
protected:
    void sink_it_(const spdlog::details::log_msg &msg) override {
        spdlog::memory_buf_t formatted;
        formatter_->format(msg, formatted);
        std::string text(formatted.data(), formatted.size());
        std::printf("[custom] %s", text.c_str());
    }

    void flush_() override {
        std::fflush(stdout);
    }
};

int main() {
    auto sink = std::make_shared<my_sink>();
    auto logger = std::make_shared<spdlog::logger>("custom", sink);
    logger->set_pattern("[%Y-%m-%d %H:%M:%S] [%l] %v");
    logger->set_level(spdlog::level::info);

    logger->info("hello from custom sink, value={}", 42);
    logger->warn("this is a warning");
    logger->flush();

    return 0;
}
