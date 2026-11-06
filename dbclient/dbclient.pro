# dbclient：基于 Qt QSqlDatabase + QODBC + unixODBC + DM ODBC 的数据库连接动态库
TEMPLATE = lib
CONFIG += shared c++17
QT += core sql
TARGET = dbclient

# 导出宏
DEFINES += DBCLIENT_LIB

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/src

SOURCES += \
    src/db_client.cpp \
    src/db_client_c.cpp \
    src/connection_pool.cpp \
    src/scoped_connection.cpp

HEADERS += \
    include/dbclient/dbclient_global.h \
    include/dbclient/db_config.h \
    include/dbclient/db_result.h \
    include/dbclient/db_client.h \
    include/dbclient/db_client_c.h \
    src/connection_pool.h \
    src/scoped_connection.h \
    src/error_classifier.h \
    src/db_client_p.h

# qmake 自动对含 Q_OBJECT 的头文件（connection_pool.h）运行 moc

# 版本信息
VERSION = 1.0.0

DESTDIR = $$OUT_PWD/../lib
