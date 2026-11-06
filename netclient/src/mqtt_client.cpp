#include "netclient/mqtt_client.h"

#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QMetaObject>
#include <QDataStream>

namespace netclient {

// =============================================================================
// MQTT 3.1.1 报文类型（固定头高 4 位）
// 参考：MQTT 3.1.1 规范 2.1.2 节
// =============================================================================
enum MqttPacketType : quint8 {
    CONNECT     = 0x10,  // 客户端 -> Broker：发起连接
    CONNACK     = 0x20,  // Broker -> 客户端：连接确认
    PUBLISH     = 0x30,  // 双向：发布消息
    PUBACK      = 0x40,  // QoS1 确认（本实现仅 QoS0，未使用）
    SUBSCRIBE   = 0x82,  // 客户端 -> Broker：订阅（type=8, flags=0010）
    SUBACK      = 0x90,  // Broker -> 客户端：订阅确认
    PINGREQ     = 0xC0,  // 客户端 -> Broker：心跳请求
    PINGRESP    = 0xD0,  // Broker -> 客户端：心跳响应
    DISCONNECT  = 0xE0,  // 客户端 -> Broker：优雅断开
};

// -----------------------------------------------------------------------------
// encodeRemainingLength - MQTT 可变长度编码（1~4 字节）
// 每个字节低 7 位存值，最高位为 1 表示后续还有字节。
// -----------------------------------------------------------------------------
static QByteArray encodeRemainingLength(quint32 length) {
    QByteArray out;
    do {
        quint8 byte = length % 128;
        length /= 128;
        if (length > 0) byte |= 0x80;  // 置延续位
        out.append(static_cast<char>(byte));
    } while (length > 0);
    return out;
}

// encodeString - MQTT UTF-8 字符串编码：2 字节大端长度前缀 + UTF-8 数据
static QByteArray encodeString(const QString& s) {
    QByteArray utf8 = s.toUtf8();
    QByteArray out;
    out.append(static_cast<char>((utf8.size() >> 8) & 0xFF));
    out.append(static_cast<char>(utf8.size() & 0xFF));
    out.append(utf8);
    return out;
}

// =============================================================================
// MqttWorker - 内部 Worker，持有 QTcpSocket，实现 MQTT 3.1.1 协议编解码
// -----------------------------------------------------------------------------
// 运行在独立线程。主线程通过 invokeMethod 跨线程调用其槽函数。
// 协议实现：CONNECT/CONNACK/PUBLISH(QoS0)/SUBSCRIBE/SUBACK/PINGREQ/PINGRESP/
//           DISCONNECT。不支持 QoS1/2（如需可扩展 PUBACK/PUBREC/PUBREL/PUBCOMP）。
// =============================================================================
class MqttWorker : public QObject {
    Q_OBJECT
public:
    explicit MqttWorker(QObject* parent = nullptr) : QObject(parent) {
        socket_ = new QTcpSocket(this);
        pingTimer_ = new QTimer(this);
        pingTimer_->setSingleShot(false);  // 周期心跳

        // socket 信号连接到本 worker 的槽（均在子线程执行）
        connect(socket_, &QTcpSocket::readyRead, this, &MqttWorker::onReadyRead);
        connect(socket_, &QTcpSocket::connected, this, &MqttWorker::onSocketConnected);
        connect(socket_, &QTcpSocket::disconnected, this, &MqttWorker::onSocketDisconnected);

        // 心跳定时器到期 -> 发 PINGREQ
        connect(pingTimer_, &QTimer::timeout, this, &MqttWorker::sendPingReq);
    }

    ~MqttWorker() override {
        if (socket_) { socket_->disconnectFromHost(); socket_->deleteLater(); }
    }

public slots:
    // 由主线程跨线程调用：发起 TCP 连接（连接成功后在 onSocketConnected 发 CONNECT）
    void connectToHost(const MqttConfig& cfg) {
        config_ = cfg;
        socket_->connectToHost(cfg.host, cfg.port);
    }

    // 优雅断开：先发 DISCONNECT 报文，再关闭 TCP
    void disconnectFromHost() {
        sendPacket(QByteArray(1, char(DISCONNECT)) + encodeRemainingLength(0));
        socket_->disconnectFromHost();
    }

    // 发布消息（QoS 0，至多一次）
    void publish(const QString& topic, const QByteArray& payload, bool retain) {
        if (!connected_) return;
        // PUBLISH 固定头：type(3)<<4 | DUP(0) | QoS(00) | RETAIN(0/1)
        quint8 fixed = PUBLISH | (retain ? 0x01 : 0x00);
        QByteArray variable = encodeString(topic);  // QoS 0 无 packetId
        QByteArray remaining = variable + payload;
        sendPacket(QByteArray(1, char(fixed))
                   + encodeRemainingLength(remaining.size()) + remaining);
    }

    // 订阅主题；topicFilter 支持 +/# 通配符
    void subscribe(const QString& topicFilter, quint8 qos) {
        if (!connected_) return;
        packetId_ = (packetId_ + 1) % 65535 + 1;  // packetId 1~65535 循环
        quint8 fixed = SUBSCRIBE;
        // 可变头：2 字节 packetId
        QByteArray variable;
        variable.append(char((packetId_ >> 8) & 0xFF));
        variable.append(char(packetId_ & 0xFF));
        // 载荷：topicFilter + QoS
        QByteArray payload = encodeString(topicFilter);
        payload.append(char(qos));
        QByteArray remaining = variable + payload;
        sendPacket(QByteArray(1, char(fixed))
                   + encodeRemainingLength(remaining.size()) + remaining);
    }

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    // TCP 连接成功 -> 发送 MQTT CONNECT 报文
    void onSocketConnected() {
        sendConnect();
    }

    // TCP 断开
    void onSocketDisconnected() {
        connected_ = false;
        pingTimer_->stop();
        emit disconnected();
    }

    // 收到数据 -> 追加到缓冲区 -> 尝试解析完整报文
    void onReadyRead() {
        buffer_.append(socket_->readAll());
        parsePackets();
    }

    // 心跳：发送 PINGREQ（无载荷）
    void sendPingReq() {
        if (connected_) sendPacket(QByteArray(1, char(PINGREQ)) + encodeRemainingLength(0));
    }

private:
    // 发送报文到 socket
    void sendPacket(const QByteArray& packet) {
        if (socket_->state() == QAbstractSocket::ConnectedState) {
            socket_->write(packet);
            socket_->flush();
        }
    }

    // -------------------------------------------------------------------------
    // sendConnect - 构造并发送 CONNECT 报文
    // 可变头：Protocol Name("MQTT") + Level(4) + Connect Flags + Keep Alive
    // 载荷：  ClientId + [Will Topic + Will Message] + [Username] + [Password]
    // -------------------------------------------------------------------------
    void sendConnect() {
        QByteArray variable;
        variable += encodeString("MQTT");          // Protocol Name
        variable.append(char(4));                   // Protocol Level = 4 (MQTT 3.1.1)
        // Connect Flags 位图：
        //   bit7 Username | bit6 Password | bit5 Will Retain
        //   bit4-3 Will QoS | bit2 Will Flag | bit1 Clean Session | bit0 reserved(0)
        quint8 flags = 0;
        if (config_.cleanSession) flags |= 0x02;
        bool hasWill = !config_.willTopic.isEmpty();
        if (hasWill) {
            flags |= 0x04;  // Will Flag
            flags |= (config_.willQos & 0x03) << 3;
            if (config_.willRetain) flags |= 0x20;
        }
        if (!config_.password.isEmpty()) flags |= 0x40;
        if (!config_.username.isEmpty()) flags |= 0x80;
        variable.append(char(flags));
        // Keep Alive（2 字节大端，单位秒）
        variable.append(char((config_.keepAliveSec >> 8) & 0xFF));
        variable.append(char(config_.keepAliveSec & 0xFF));

        // Payload
        QByteArray payload = encodeString(config_.clientId);
        if (hasWill) {
            payload += encodeString(config_.willTopic);
            payload += encodeString(config_.willMessage);
        }
        if (!config_.username.isEmpty()) payload += encodeString(config_.username);
        if (!config_.password.isEmpty()) payload += encodeString(config_.password);

        QByteArray remaining = variable + payload;
        sendPacket(QByteArray(1, char(CONNECT))
                   + encodeRemainingLength(remaining.size()) + remaining);
    }

    // -------------------------------------------------------------------------
    // parsePackets - 从缓冲区解析一个或多个完整 MQTT 报文
    // 报文结构：固定头(1B) + 可变长度编码(1~4B) + 载荷(remaining B)
    // -------------------------------------------------------------------------
    void parsePackets() {
        while (buffer_.size() >= 2) {
            quint8 type = static_cast<quint8>(buffer_[0]);
            // 解析可变长度编码
            quint32 remaining = 0;
            int multiplier = 1;
            int lenBytes = 0;
            int idx = 1;
            while (idx < buffer_.size()) {
                quint8 b = static_cast<quint8>(buffer_[idx]);
                remaining += (b & 0x7F) * multiplier;
                multiplier *= 128;
                lenBytes++;
                idx++;
                if (!(b & 0x80)) break;       // 最高位为 0，结束
                if (lenBytes > 4) return;      // 超过 4 字节，非法报文
            }
            int totalLen = 1 + lenBytes + remaining;
            if (buffer_.size() < totalLen) return;  // 数据不全，等待后续

            QByteArray packet = buffer_.mid(1 + lenBytes, remaining);
            buffer_.remove(0, totalLen);  // 从缓冲区移除已解析报文
            handlePacket(type, packet);
        }
    }

    // -------------------------------------------------------------------------
    // handlePacket - 按报文类型分发处理
    // -------------------------------------------------------------------------
    void handlePacket(quint8 type, const QByteArray& data) {
        switch (type & 0xF0) {  // 取高 4 位报文类型
        case CONNACK:
            // CONNACK 载荷：连接确认标志(1B) + 返回码(1B)
            // 返回码 0 = 连接已接受
            if (data.size() >= 2) {
                quint8 returnCode = static_cast<quint8>(data[1]);
                if (returnCode == 0) {
                    connected_ = true;
                    emit connected();
                    // 启动心跳定时器（按 keepAlive 秒）
                    if (config_.keepAliveSec > 0)
                        pingTimer_->start(config_.keepAliveSec * 1000);
                }
            }
            break;
        case PUBLISH: {
            // PUBLISH 载荷：topic(2B len + data) + [packetId 2B, QoS>0] + payload
            if (data.size() < 2) break;
            int topicLen = (static_cast<quint8>(data[0]) << 8)
                         | static_cast<quint8>(data[1]);
            QString topic = QString::fromUtf8(data.mid(2, topicLen));
            int headerLen = 2 + topicLen;
            quint8 qos = (type >> 1) & 0x03;  // 从固定头低 4 位取 QoS
            if (qos > 0) headerLen += 2;       // QoS1/2 有 2 字节 packetId
            QByteArray payload = data.mid(headerLen);
            emit messageReceived(topic, payload);
            break;
        }
        case PINGRESP:
            // 心跳响应，无需处理（重置超时计时器的逻辑可在此扩展）
            break;
        case SUBACK:
            // 订阅确认（可在此校验订阅的 QoS 是否被 Broker 接受）
            break;
        default:
            break;
        }
    }

    QTcpSocket* socket_ = nullptr;   // TCP 连接
    QTimer* pingTimer_ = nullptr;    // 心跳定时器
    QByteArray buffer_;              // 接收缓冲区（处理粘包/半包）
    quint16 packetId_ = 0;           // SUBSCRIBE 的 packetId 计数器
    bool connected_ = false;         // MQTT 连接状态（CONNACK 通过后为 true）
    MqttConfig config_;              // 连接配置
};

// =============================================================================
// MqttClient 实现（主线程代理）
// =============================================================================

MqttClient::MqttClient(QObject* parent) : QObject(parent) { initWorker(); }

MqttClient::~MqttClient() {
    if (thread_) {
        // 阻塞断开，确保 DISCONNECT 报文发出
        QMetaObject::invokeMethod(worker_, "disconnectFromHost",
                                  Qt::BlockingQueuedConnection);
        thread_->quit();
        thread_->wait();
        delete thread_;
    }
}

void MqttClient::initWorker() {
    thread_ = new QThread(this);
    auto* worker = new MqttWorker();
    worker_ = worker;
    worker->moveToThread(thread_);

    // 子线程信号 -> 主线程槽
    connect(worker, &MqttWorker::connected, this, &MqttClient::onConnected);
    connect(worker, &MqttWorker::disconnected, this, &MqttClient::onDisconnected);
    connect(worker, &MqttWorker::messageReceived, this, &MqttClient::onMessage);

    connect(thread_, &QThread::finished, worker, &QObject::deleteLater);
    thread_->start();
}

// ---- 跨线程调用 ----
void MqttClient::connectToHost(const MqttConfig& cfg) {
    auto* w = static_cast<MqttWorker*>(worker_);
    QMetaObject::invokeMethod(worker_, [w, cfg]() {
        w->connectToHost(cfg);
    }, Qt::QueuedConnection);
}

void MqttClient::disconnectFromHost() {
    QMetaObject::invokeMethod(worker_, "disconnectFromHost", Qt::QueuedConnection);
}

bool MqttClient::isConnected() const { return connected_; }

void MqttClient::publish(const QString& topic, const QByteArray& payload, bool retain) {
    QMetaObject::invokeMethod(worker_, "publish", Qt::QueuedConnection,
                              Q_ARG(QString, topic), Q_ARG(QByteArray, payload),
                              Q_ARG(bool, retain));
}

void MqttClient::subscribe(const QString& topicFilter, quint8 qos) {
    QMetaObject::invokeMethod(worker_, "subscribe", Qt::QueuedConnection,
                              Q_ARG(QString, topicFilter), Q_ARG(quint8, qos));
}

// ---- 状态/消息转发槽 ----
void MqttClient::onConnected() { connected_ = true; emit connected(); }
void MqttClient::onDisconnected() { connected_ = false; emit disconnected(); }
void MqttClient::onMessage(const QString& topic, const QByteArray& payload) {
    emit messageReceived(topic, payload);
}

// ---- 回调桥接 ----
void MqttClient::setMessageCallback(MqttMessageCallback cb) {
    connect(this, &MqttClient::messageReceived, this,
            [cb](const QString& t, const QByteArray& p) { if (cb) cb(t, p); });
}

void MqttClient::setConnectedCallback(MqttStateCallback cb) {
    connect(this, &MqttClient::connected, this, [cb]() { if (cb) cb(true); });
    connect(this, &MqttClient::disconnected, this, [cb]() { if (cb) cb(false); });
}

} // namespace netclient

// MqttWorker 的 moc 在 cpp 内生成
#include "mqtt_client.moc"
