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

// =============================================================================
// MqttConfig - MQTT 连接配置
// =============================================================================
struct NETCLIENT_API MqttConfig {
    QString host;                 // Broker 地址
    quint16 port = 1883;          // 端口（1883=明文，8883=TLS）
    QString clientId;             // 客户端 ID（空则由库生成）
    QString username;             // 用户名（可选）
    QString password;             // 密码（可选）
    int keepAliveSec = 60;        // 心跳间隔（秒），Broker 在此时间 1.5 倍内未收到报文则断开
    bool cleanSession = true;     // true=断开后清除会话；false=保留订阅与离线消息
    QString willTopic;            // 遗嘱主题（异常断开时 Broker 自动发布）
    QString willMessage;          // 遗嘱消息内容
    quint8 willQos = 0;           // 遗嘱 QoS 等级
    bool willRetain = false;      // 遗嘱是否保留
};

// 消息回调：收到订阅消息时触发（topic=主题，payload=载荷）
using MqttMessageCallback = std::function<void(const QString& topic, const QByteArray& payload)>;
using MqttStateCallback = std::function<void(bool connected)>;

// =============================================================================
// MqttClient - MQTT 3.1.1 客户端
// -----------------------------------------------------------------------------
// 基于 QTcpSocket 自行编解码 MQTT 3.1.1 报文（未使用 QtMqtt，避免 GPL 许可问题）。
// 支持的报文类型：CONNECT / CONNACK / PUBLISH(QoS0) / SUBSCRIBE / SUBACK /
//                 PINGREQ / PINGRESP / DISCONNECT
//
// 每个实例运行在独立 QThread（MqttWorker 持有 QTcpSocket），主线程通过
// QMetaObject::invokeMethod 跨线程调用。
//
// 心跳：按 keepAliveSec 定时发送 PINGREQ，收到 PINGRESP 重置超时计时。
// =============================================================================
class NETCLIENT_API MqttClient : public QObject {
    Q_OBJECT
public:
    explicit MqttClient(QObject* parent = nullptr);
    ~MqttClient() override;

    // 异步连接到 Broker
    void connectToHost(const MqttConfig& cfg);

    // 断开连接（发送 DISCONNECT 报文后关闭 socket）
    void disconnectFromHost();

    bool isConnected() const;

    // 发布消息（QoS 0，至多一次）；retain=true 时 Broker 保留该消息
    void publish(const QString& topic, const QByteArray& payload, bool retain = false);

    // 订阅主题；topicFilter 支持 +（单层）和 #（多层）通配符
    void subscribe(const QString& topicFilter, quint8 qos = 0);

    // ---- 回调设置 ----
    void setMessageCallback(MqttMessageCallback cb);   // 收到消息
    void setConnectedCallback(MqttStateCallback cb);   // 连接状态变化

signals:
    void connected();
    void disconnected();
    void messageReceived(const QString& topic, const QByteArray& payload);

private slots:
    // 状态维护槽：由子线程信号触发，更新主线程状态与回调
    void onConnected();
    void onDisconnected();
    void onMessage(const QString& topic, const QByteArray& payload);

private:
    void initWorker();  // 创建子线程与 MqttWorker

    QThread* thread_ = nullptr;
    QObject* worker_ = nullptr;
    bool connected_ = false;
};

} // namespace netclient

#endif // NETCLIENT_MQTT_CLIENT_H
