#ifndef BTHREAD_MESSAGE_H
#define BTHREAD_MESSAGE_H

#include <QString>
#include <QVariant>
#include <QMetaType>
#include <QDateTime>

namespace bthread {

// 线程间通信的消息体
struct Message {
    QString type;       // 消息类型，如 "data_acquired" / "process_result"
    QString from;       // 发送方线程名
    QVariant data;      // 负载数据（可承载任意注册到 Qt 元类型系统的类型）
    qint64 timestamp = 0;

    Message() { timestamp = QDateTime::currentMSecsSinceEpoch(); }
    Message(const QString& t, const QString& f, const QVariant& d = QVariant())
        : type(t), from(f), data(d) {
        timestamp = QDateTime::currentMSecsSinceEpoch();
    }
};

} // namespace bthread

Q_DECLARE_METATYPE(bthread::Message)

#endif // BTHREAD_MESSAGE_H
