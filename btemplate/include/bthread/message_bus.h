#ifndef BTHREAD_MESSAGE_BUS_H
#define BTHREAD_MESSAGE_BUS_H

#include <QObject>
#include "message.h"

namespace bthread {

// =============================================================================
// MessageBus - 消息总线（发布/订阅模式）
// -----------------------------------------------------------------------------
// 单一 QObject 实例，常驻主线程。所有业务线程订阅其 messagePublished 信号，
// 任意线程调用 publish() 广播消息时，Qt 自动按 QueuedConnection 把消息
// 投递到各订阅线程的事件队列，由各线程事件循环取出并执行 onMessage 槽。
//
// 优点：线程间完全解耦，发送方无需知道接收方是谁；天然线程安全。
// 注意：广播会发送给所有订阅线程，需在 handleMessage 中按 type 过滤。
// =============================================================================
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
