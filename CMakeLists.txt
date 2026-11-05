cmake_minimum_required(VERSION 3.16)
project(spdlog_demo LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# spdlog 以源码方式内置在 third_party/spdlog
# SPDLOG_SHARED: OFF -> 静态库(默认)，ON -> 动态库
# ---------------------------------------------------------------------------
option(SPDLOG_SHARED "Link spdlog as a shared library" OFF)

set(SPDLOG_BUILD_EXAMPLE OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_EXAMPLE_HO OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_BENCH OFF CACHE BOOL "" FORCE)
set(SPDLOG_BUILD_SHARED ${SPDLOG_SHARED} CACHE BOOL "" FORCE)
# 静态库要嵌进动态库，需要位置无关代码
set(SPDLOG_BUILD_PIC ON CACHE BOOL "" FORCE)

add_subdirectory(third_party/spdlog)

# ---------------------------------------------------------------------------
# 使用示例
# ---------------------------------------------------------------------------
add_executable(demo src/main.cpp)
target_link_libraries(demo PRIVATE spdlog::spdlog)

add_executable(ex_basic examples/01_basic.cpp)
target_link_libraries(ex_basic PRIVATE spdlog::spdlog)

add_executable(ex_async_rotating examples/02_async_rotating.cpp)
target_link_libraries(ex_async_rotating PRIVATE spdlog::spdlog)

add_executable(ex_custom_sink examples/03_custom_sink.cpp)
target_link_libraries(ex_custom_sink PRIVATE spdlog::spdlog)

# 纯头文件方式：不需要链接任何 spdlog 库
add_executable(ex_header_only examples/04_header_only.cpp)
target_link_libraries(ex_header_only PRIVATE spdlog::spdlog_header_only)

# ---------------------------------------------------------------------------
# 动态库实现：把 spdlog 封装成 liblogger.so / logger.dll，对外暴露 C API
# ---------------------------------------------------------------------------
add_library(logger_shared SHARED lib/logger_api.cpp)
target_include_directories(logger_shared PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/lib)
target_link_libraries(logger_shared PRIVATE spdlog::spdlog)
target_compile_definitions(logger_shared PRIVATE LOGGER_BUILD_SHARED)
set_target_properties(logger_shared PROPERTIES
    OUTPUT_NAME logger
    VERSION 1.0.0
    SOVERSION 1
)

# 动态库的使用方：只包含 C 头文件，链接 liblogger
add_executable(ex_dynamic_lib examples/05_dynamic_lib.cpp)
target_link_libraries(ex_dynamic_lib PRIVATE logger_shared)
