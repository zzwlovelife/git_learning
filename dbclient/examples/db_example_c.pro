QT += core sql
CONFIG += c++17 console
TARGET = db_example_c
include(../../third_party/spdlog/spdlog.pri)
INCLUDEPATH += $$PWD/../../dbclient/include
SOURCES += db_example_c.cpp
LIBS += -L$$OUT_PWD/../../lib -ldbclient
DESTDIR = $$OUT_PWD/../../bin
