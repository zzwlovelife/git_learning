#ifndef NETCLIENT_WEBSOCKET_CLIENT_H
#define NETCLIENT_WEBSOCKET_CLIENT_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QUrl>
#include <functional>

#include "netclient/netclient_global.h"

class QWebSocket;
class QThread;
class QTimer;

namespace netclient {

using WsTextCallback = std::function<void(const QString&)>;
using WsBinaryCallback = std::function<void(const QByteArray&)>;
using WsStateCallback = std::function<void(bool connected)>;

// WebSocket 客户端：基于 QWebSocket
// 每个实例在独立 QThread 中运行，支持连接保活（Ping/Pong + 定时器）
class NETCLIENT_API WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(QObject* parent = nullptr);
    ~WebSocketClient() override;

    // 连接到服务器（异步，非阻塞）
    void open(const QUrl& url);

    // 关闭连接
    void close();

    // 是否连接
    bool isConnected() const;

    // 发送文本消息
    void sendText(const QString& text);

    // 发送二进制消息
    void sendBinary(const QByteArray& data);

    // 设置回调
    void setTextCallback(WsTextCallback cb);
    void setBinaryCallback(WsBinaryCallback cb);
    void setConnectedCallback(WsStateCallback cb);

    // 设置心跳间隔（毫秒），<=0 关闭心跳；默认 30s
    void setHeartbeatInterval(int ms);

    // 重写 isConnected 的状态维护
private slots:
    void onConnected();
    void onDisconnected();

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString& message);
    void binaryMessageReceived(const QByteArray& data);

private:
    void initWorker();

    QThread* thread_ = nullptr;
    QObject* worker_ = nullptr;  // 实际持有 QWebSocket 的对象
    bool connected_ = false;     // 主线程维护的连接状态
};

} // namespace netclient

#endif // NETCLIENT_WEBSOCKET_CLIENT_H
