#ifndef NETCLIENT_MQTT_CLIENT_H
#define NETCLIENT_MQTT_CLIENT_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <functional>

#include "netclient/netclient_global.h"

class QTcpSocket;
class QThread;
class QTimer;

namespace netclient {

// MQTT 连接配置
struct NETCLIENT_API MqttConfig {
    QString host;
    quint16 port = 1883;
    QString clientId;
    QString username;
    QString password;
    int keepAliveSec = 60;      // 心跳间隔（秒）
    bool cleanSession = true;
    QString willTopic;
    QString willMessage;
    quint8 willQos = 0;
    bool willRetain = false;
};

using MqttMessageCallback = std::function<void(const QString& topic, const QByteArray& payload)>;
using MqttStateCallback = std::function<void(bool connected)>;

// MQTT 3.1.1 客户端：基于 QTcpSocket 实现（QtMqtt 不可用时的纯 Qt 方案）
// 支持 CONNECT / PUBLISH(QoS0) / SUBSCRIBE / PING / DISCONNECT
// 每个实例在独立 QThread 中运行
class NETCLIENT_API MqttClient : public QObject {
    Q_OBJECT
public:
    explicit MqttClient(QObject* parent = nullptr);
    ~MqttClient() override;

    // 连接（异步）
    void connectToHost(const MqttConfig& cfg);

    // 断开
    void disconnectFromHost();

    bool isConnected() const;

    // 发布消息（QoS 0）
    void publish(const QString& topic, const QByteArray& payload, bool retain = false);

    // 订阅主题
    void subscribe(const QString& topicFilter, quint8 qos = 0);

    // 设置回调
    void setMessageCallback(MqttMessageCallback cb);
    void setConnectedCallback(MqttStateCallback cb);

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    void onConnected();
    void onDisconnected();
    void onMessage(const QString& topic, const QByteArray& payload);

private:
    void initWorker();

    QThread* thread_ = nullptr;
    QObject* worker_ = nullptr;
    bool connected_ = false;
};

} // namespace netclient

#endif // NETCLIENT_MQTT_CLIENT_H
