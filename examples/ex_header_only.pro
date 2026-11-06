CONFIG += c++17 console
TARGET = ex_header_only
include(../third_party/spdlog/spdlog.pri)
SOURCES += 04_header_only.cpp
DESTDIR = $$OUT_PWD/../bin
