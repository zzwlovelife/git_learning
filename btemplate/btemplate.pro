# 业务线程模板示例
QT += core
CONFIG += c++17 console
TARGET = btemplate

INCLUDEPATH += \
    $$PWD/include \
    $$PWD/../third_party/spdlog/include

SOURCES += \
    src/main.cpp \
    src/business_thread.cpp \
    src/thread_manager.cpp

HEADERS += \
    include/bthread/message.h \
    include/bthread/message_bus.h \
    include/bthread/business_thread.h \
    include/bthread/thread_manager.h \
    include/bthread/example_threads.h
