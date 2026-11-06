CONFIG += c++17 console
TARGET = ex_custom_sink
include(../third_party/spdlog/spdlog.pri)
SOURCES += 03_custom_sink.cpp
DESTDIR = $$OUT_PWD/../bin
