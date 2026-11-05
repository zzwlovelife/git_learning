#include <spdlog/spdlog.h>

int main() {
    spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");

    spdlog::info("Hello, {}!", "spdlog");
    spdlog::set_level(spdlog::level::debug);
    spdlog::debug("answer = {}", 42);

    spdlog::info("int={:08d}, hex={:#x}, float={:.2f}", 42, 42, 3.14159);
    spdlog::warn("disk usage {:.1f}%", 87.5);
    spdlog::error("connect failed: {}", "timeout");

    return 0;
}
