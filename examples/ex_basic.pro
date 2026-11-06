CONFIG += c++17 console
TARGET = ex_basic
include(../third_party/spdlog/spdlog.pri)
SOURCES += 01_basic.cpp
DESTDIR = $$OUT_PWD/../bin
