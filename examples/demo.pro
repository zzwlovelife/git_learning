CONFIG += c++17 console
TARGET = demo
include(../third_party/spdlog/spdlog.pri)
SOURCES += ../src/main.cpp
DESTDIR = $$OUT_PWD/../bin
