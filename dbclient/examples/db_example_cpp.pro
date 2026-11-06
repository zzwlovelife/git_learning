QT += core sql
CONFIG += c++17 console
TARGET = db_example_cpp
include(../../third_party/spdlog/spdlog.pri)
INCLUDEPATH += $$PWD/../../dbclient/include
SOURCES += db_example_cpp.cpp
LIBS += -L$$OUT_PWD/../../lib -ldbclient
DESTDIR = $$OUT_PWD/../../bin
