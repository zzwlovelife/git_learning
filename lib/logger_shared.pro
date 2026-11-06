# logger_shared：把 spdlog 封装成动态库，对外暴露 C API
TEMPLATE = lib
CONFIG += shared c++17
TARGET = logger

# 导出宏（Windows dllexport / Linux visibility）
DEFINES += LOGGER_BUILD_SHARED

# 集成 spdlog（header-only）
include(../third_party/spdlog/spdlog.pri)

INCLUDEPATH += $$PWD

SOURCES += logger_api.cpp
HEADERS += logger_api.h

# 注意：本 .pro 位于 lib/ 目录，qmake 的构建目录即为 build-qmake/lib/，
# 因此产物（liblogger.so）直接落在统一的 lib 输出目录中，无需再设 DESTDIR
# （若设 DESTDIR=$$OUT_PWD/../lib 会指向自身目录，触发 rm+mv 自删除 bug）。
