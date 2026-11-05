// 本示例链接的是 spdlog::spdlog_header_only，
// 不需要任何 .a/.so 库文件，实现随源码一起编译。
#include <spdlog/spdlog.h>

int main() {
    spdlog::info("header-only mode works, {}", "no library file needed");
    spdlog::warn("this binary contains the whole implementation");
    return 0;
}
