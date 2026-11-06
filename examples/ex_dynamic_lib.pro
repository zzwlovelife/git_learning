CONFIG += c++17 console
TARGET = ex_dynamic_lib
include(../third_party/spdlog/spdlog.pri)
INCLUDEPATH += $$PWD/../lib
SOURCES += 05_dynamic_lib.cpp
LIBS += -L$$OUT_PWD/../lib -llogger
DESTDIR = $$OUT_PWD/../bin
