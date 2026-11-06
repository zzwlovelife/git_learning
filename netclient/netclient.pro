# netclient 动态库：HTTP / WebSocket / MQTT / RPC 客户端
TEMPLATE = lib
CONFIG += shared c++17
QT += core network websockets
TARGET = netclient
DEFINES += NETCLIENT_LIB

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/src

SOURCES += \
    src/http_client.cpp \
    src/websocket_client.cpp \
    src/mqtt_client.cpp \
    src/rpc_client.cpp \
    src/net_client.cpp

HEADERS += \
    include/netclient/netclient_global.h \
    include/netclient/http_client.h \
    include/netclient/websocket_client.h \
    include/netclient/mqtt_client.h \
    include/netclient/rpc_client.h \
    include/netclient/net_client.h

VERSION = 1.0.0
DESTDIR = $$OUT_PWD/../lib
