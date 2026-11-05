#ifndef LOGGER_API_H
#define LOGGER_API_H

#if defined(_WIN32)
#  if defined(LOGGER_BUILD_SHARED)
#    define LOGGER_API __declspec(dllexport)
#  else
#    define LOGGER_API __declspec(dllimport)
#  endif
#else
#  define LOGGER_API __attribute__((visibility("default")))
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    LOGGER_LEVEL_DEBUG = 0,
    LOGGER_LEVEL_INFO = 1,
    LOGGER_LEVEL_WARN = 2,
    LOGGER_LEVEL_ERROR = 3
} logger_level_t;

/* 初始化日志库，log_dir 为日志目录；成功返回 0，失败返回 -1 */
LOGGER_API int logger_init(const char *log_dir);

/* 写一条日志，线程安全 */
LOGGER_API void logger_log(logger_level_t level, const char *message);

/* 立即刷盘 */
LOGGER_API void logger_flush(void);

/* 停止后台线程并释放资源 */
LOGGER_API void logger_shutdown(void);

#ifdef __cplusplus
}
#endif

#endif
