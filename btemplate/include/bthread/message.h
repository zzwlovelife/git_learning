#ifndef BTHREAD_MESSAGE_H
#define BTHREAD_MESSAGE_H

#include <QString>
#include <QVariant>
#include <QMetaType>
#include <QDateTime>

namespace bthread {

// =============================================================================
// Message - 线程间通信的消息体
// -----------------------------------------------------------------------------
// 通过 type 字段区分消息种类，data 用 QVariant 承载任意负载（需注册到元类型系统）。
// timestamp 在构造时自动填充，便于日志与调试。
// 跨线程传递时通过 Q_DECLARE_METATYPE + qRegisterMetaType 注册，
// 使 QueuedConnection 能正确拷贝消息对象。
// =============================================================================
struct Message {
    QString type;       // 消息类型（如 "data_acquired" / "process_result"）
    QString from;       // 发送方线程名
    QVariant data;      // 负载数据
    qint64 timestamp = 0;  // 消息创建时间戳（毫秒）

    Message() { timestamp = QDateTime::currentMSecsSinceEpoch(); }
    Message(const QString& t, const QString& f, const QVariant& d = QVariant())
        : type(t), from(f), data(d) {
        timestamp = QDateTime::currentMSecsSinceEpoch();
    }
};

} // namespace bthread

// 注册到 Qt 元类型系统，支持跨线程信号槽传递
Q_DECLARE_METATYPE(bthread::Message)

#endif // BTHREAD_MESSAGE_H
