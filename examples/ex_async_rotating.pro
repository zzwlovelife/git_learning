CONFIG += c++17 console
TARGET = ex_async_rotating
include(../third_party/spdlog/spdlog.pri)
SOURCES += 02_async_rotating.cpp
DESTDIR = $$OUT_PWD/../bin
