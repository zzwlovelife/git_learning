#ifndef BTHREAD_MESSAGE_BUS_H
#define BTHREAD_MESSAGE_BUS_H

#include <QObject>
#include "message.h"

namespace bthread {

// 消息总线：所有业务线程订阅，实现线程间广播通信
// 总线对象常驻主线程；各业务线程的 onMessage 槽以 QueuedConnection 连接，
// 消息会投递到目标线程的事件队列，由该线程的事件循环取出执行。
class MessageBus : public QObject {
    Q_OBJECT
public:
    explicit MessageBus(QObject* parent = nullptr) : QObject(parent) {
        qRegisterMetaType<Message>("bthread::Message");
    }

    // 广播一条消息到所有订阅线程
    void publish(const Message& msg) { emit messagePublished(msg); }

signals:
    void messagePublished(const bthread::Message& msg);
};

} // namespace bthread

#endif // BTHREAD_MESSAGE_BUS_H
