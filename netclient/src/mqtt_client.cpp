#include "netclient/mqtt_client.h"

#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QMetaObject>
#include <QDataStream>

namespace netclient {

// MQTT 3.1.1 报文类型
enum MqttPacketType : quint8 {
    CONNECT     = 0x10,
    CONNACK     = 0x20,
    PUBLISH     = 0x30,
    PUBACK      = 0x40,
    SUBSCRIBE   = 0x82,  // type 8 + flags 0010
    SUBACK      = 0x90,
    PINGREQ     = 0xC0,
    PINGRESP    = 0xD0,
    DISCONNECT  = 0xE0,
};

// 编码剩余长度（可变长度编码，1-4 字节）
static QByteArray encodeRemainingLength(quint32 length) {
    QByteArray out;
    do {
        quint8 byte = length % 128;
        length /= 128;
        if (length > 0) byte |= 0x80;
        out.append(static_cast<char>(byte));
    } while (length > 0);
    return out;
}

// 编码 UTF-8 字符串（2 字节长度前缀 + 数据）
static QByteArray encodeString(const QString& s) {
    QByteArray utf8 = s.toUtf8();
    QByteArray out;
    out.append(static_cast<char>((utf8.size() >> 8) & 0xFF));
    out.append(static_cast<char>(utf8.size() & 0xFF));
    out.append(utf8);
    return out;
}

// 内部 Worker：持有 QTcpSocket，运行在独立线程，实现 MQTT 3.1.1
class MqttWorker : public QObject {
    Q_OBJECT
public:
    explicit MqttWorker(QObject* parent = nullptr) : QObject(parent) {
        socket_ = new QTcpSocket(this);
        pingTimer_ = new QTimer(this);
        pingTimer_->setSingleShot(false);

        connect(socket_, &QTcpSocket::readyRead, this, &MqttWorker::onReadyRead);
        connect(socket_, &QTcpSocket::connected, this, &MqttWorker::onSocketConnected);
        connect(socket_, &QTcpSocket::disconnected, this, &MqttWorker::onSocketDisconnected);

        connect(pingTimer_, &QTimer::timeout, this, &MqttWorker::sendPingReq);
    }

    ~MqttWorker() override {
        if (socket_) { socket_->disconnectFromHost(); socket_->deleteLater(); }
    }

public slots:
    void connectToHost(const MqttConfig& cfg) {
        config_ = cfg;
        socket_->connectToHost(cfg.host, cfg.port);
    }

    void disconnectFromHost() {
        sendPacket(QByteArray(1, char(DISCONNECT)) + encodeRemainingLength(0));
        socket_->disconnectFromHost();
    }

    void publish(const QString& topic, const QByteArray& payload, bool retain) {
        if (!connected_) return;
        // PUBLISH 固定头：type(3)<<4 | DUP(0) | QoS(00) | RETAIN
        quint8 fixed = PUBLISH | (retain ? 0x01 : 0x00);
        QByteArray variable = encodeString(topic);  // QoS 0 无 packetId
        QByteArray remaining = variable + payload;
        sendPacket(QByteArray(1, char(fixed)) + encodeRemainingLength(remaining.size()) + remaining);
    }

    void subscribe(const QString& topicFilter, quint8 qos) {
        if (!connected_) return;
        packetId_ = (packetId_ + 1) % 65535 + 1;
        quint8 fixed = SUBSCRIBE;
        QByteArray variable;
        variable.append(char((packetId_ >> 8) & 0xFF));
        variable.append(char(packetId_ & 0xFF));
        QByteArray payload = encodeString(topicFilter);
        payload.append(char(qos));
        QByteArray remaining = variable + payload;
        sendPacket(QByteArray(1, char(fixed)) + encodeRemainingLength(remaining.size()) + remaining);
    }

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    void onSocketConnected() {
        sendConnect();
    }

    void onSocketDisconnected() {
        connected_ = false;
        pingTimer_->stop();
        emit disconnected();
    }

    void onReadyRead() {
        buffer_.append(socket_->readAll());
        parsePackets();
    }

    void sendPingReq() {
        if (connected_) sendPacket(QByteArray(1, char(PINGREQ)) + encodeRemainingLength(0));
    }

private:
    void sendPacket(const QByteArray& packet) {
        if (socket_->state() == QAbstractSocket::ConnectedState) {
            socket_->write(packet);
            socket_->flush();
        }
    }

    void sendConnect() {
        // 可变头
        QByteArray variable;
        variable += encodeString("MQTT");          // Protocol Name
        variable.append(char(4));                   // Protocol Level (3.1.1)
        // Connect Flags
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
        // Keep Alive
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
        sendPacket(QByteArray(1, char(CONNECT)) + encodeRemainingLength(remaining.size()) + remaining);
    }

    void parsePackets() {
        while (buffer_.size() >= 2) {
            quint8 type = static_cast<quint8>(buffer_[0]);
            // 解析剩余长度
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
                if (!(b & 0x80)) break;
                if (lenBytes > 4) return;  // 错误
            }
            int totalLen = 1 + lenBytes + remaining;
            if (buffer_.size() < totalLen) return;  // 数据不足

            QByteArray packet = buffer_.mid(1 + lenBytes, remaining);
            buffer_.remove(0, totalLen);
            handlePacket(type, packet);
        }
    }

    void handlePacket(quint8 type, const QByteArray& data) {
        switch (type & 0xF0) {
        case CONNACK:
            if (data.size() >= 2) {
                quint8 returnCode = static_cast<quint8>(data[1]);
                if (returnCode == 0) {
                    connected_ = true;
                    emit connected();
                    if (config_.keepAliveSec > 0)
                        pingTimer_->start(config_.keepAliveSec * 1000);
                }
            }
            break;
        case PUBLISH: {
            // 解析 topic
            if (data.size() < 2) break;
            int topicLen = (static_cast<quint8>(data[0]) << 8) | static_cast<quint8>(data[1]);
            QString topic = QString::fromUtf8(data.mid(2, topicLen));
            int headerLen = 2 + topicLen;
            // QoS 1/2 有 2 字节 packetId
            quint8 qos = (type >> 1) & 0x03;
            if (qos > 0) headerLen += 2;
            QByteArray payload = data.mid(headerLen);
            emit messageReceived(topic, payload);
            break;
        }
        case PINGRESP:
            // 心跳响应，无需处理
            break;
        case SUBACK:
            // 订阅确认
            break;
        default:
            break;
        }
    }

    QTcpSocket* socket_ = nullptr;
    QTimer* pingTimer_ = nullptr;
    QByteArray buffer_;
    quint16 packetId_ = 0;
    bool connected_ = false;
    MqttConfig config_;
};

// ---------------- MqttClient 实现 ----------------

MqttClient::MqttClient(QObject* parent) : QObject(parent) { initWorker(); }

MqttClient::~MqttClient() {
    if (thread_) {
        QMetaObject::invokeMethod(worker_, "disconnectFromHost", Qt::BlockingQueuedConnection);
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

    connect(worker, &MqttWorker::connected, this, &MqttClient::onConnected);
    connect(worker, &MqttWorker::disconnected, this, &MqttClient::onDisconnected);
    connect(worker, &MqttWorker::messageReceived, this, &MqttClient::onMessage);

    connect(thread_, &QThread::finished, worker, &QObject::deleteLater);
    thread_->start();
}

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

void MqttClient::onConnected() { connected_ = true; emit connected(); }
void MqttClient::onDisconnected() { connected_ = false; emit disconnected(); }
void MqttClient::onMessage(const QString& topic, const QByteArray& payload) {
    emit messageReceived(topic, payload);
}

void MqttClient::setMessageCallback(MqttMessageCallback cb) {
    connect(this, &MqttClient::messageReceived, this,
            [cb](const QString& t, const QByteArray& p) { if (cb) cb(t, p); });
}

void MqttClient::setConnectedCallback(MqttStateCallback cb) {
    connect(this, &MqttClient::connected, this, [cb]() { if (cb) cb(true); });
    connect(this, &MqttClient::disconnected, this, [cb]() { if (cb) cb(false); });
}

} // namespace netclient

#include "mqtt_client.moc"
