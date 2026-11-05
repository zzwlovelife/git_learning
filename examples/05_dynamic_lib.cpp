// 本示例只包含动态库的 C 头文件，链接 liblogger.so / logger.dll
#include "logger_api.h"

int main() {
    if (logger_init("logs/dynamic_demo") != 0) {
        return 1;
    }

    logger_log(LOGGER_LEVEL_DEBUG, "debug from dynamic library");
    logger_log(LOGGER_LEVEL_INFO, "hello from dynamic library");
    logger_log(LOGGER_LEVEL_WARN, "this is a warning");
    logger_log(LOGGER_LEVEL_ERROR, "this is an error");

    logger_flush();
    logger_shutdown();
    return 0;
}
