# netclient 综合示例
QT += core network
CONFIG += c++17 console
TARGET = netclient_demo
INCLUDEPATH += $$PWD/../include
SOURCES += netclient_demo.cpp
LIBS += -L$$OUT_PWD/../lib -lnetclient
DESTDIR = $$OUT_PWD/../bin
