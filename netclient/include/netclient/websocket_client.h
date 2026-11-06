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

// 回调类型：分别处理文本消息、二进制消息、连接状态变化
using WsTextCallback = std::function<void(const QString&)>;
using WsBinaryCallback = std::function<void(const QByteArray&)>;
using WsStateCallback = std::function<void(bool connected)>;

// =============================================================================
// WebSocketClient - WebSocket 客户端
// -----------------------------------------------------------------------------
// 基于 QWebSocket。每个实例在独立 QThread 中运行（WsWorker 持有 QWebSocket），
// 主线程通过 QMetaObject::invokeMethod 跨线程调用，回调经信号槽回到调用方。
//
// 心跳：内置 Ping 定时器，间隔可配（默认 30s）。QWebSocket 收到 Ping 后由
//       Qt 协议栈自动回 Pong，无需手动处理。
//
// 线程模型：
//   主线程（WebSocketClient）  --invokeMethod-->  子线程（WsWorker + QWebSocket）
//   信号(textMessageReceived...)  <--QueuedConnection--  子线程 emit
// =============================================================================
class NETCLIENT_API WebSocketClient : public QObject {
    Q_OBJECT
public:
    explicit WebSocketClient(QObject* parent = nullptr);
    ~WebSocketClient() override;

    // 异步连接到服务器（非阻塞，连接成功后触发 connected 信号/回调）
    void open(const QUrl& url);

    // 关闭连接
    void close();

    // 是否已连接（主线程缓存的状态）
    bool isConnected() const;

    // 发送文本消息
    void sendText(const QString& text);

    // 发送二进制消息
    void sendBinary(const QByteArray& data);

    // ---- 回调设置（也可直接连接下方 signals）----
    void setTextCallback(WsTextCallback cb);       // 收到文本消息
    void setBinaryCallback(WsBinaryCallback cb);   // 收到二进制消息
    void setConnectedCallback(WsStateCallback cb); // 连接/断开状态变化

    // 设置心跳间隔（毫秒），<=0 关闭心跳；默认 30000
    void setHeartbeatInterval(int ms);

private slots:
    // 状态维护槽：由子线程信号触发，更新主线程 connected_ 标志
    void onConnected();
    void onDisconnected();

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString& message);
    void binaryMessageReceived(const QByteArray& data);

private:
    void initWorker();  // 创建子线程与 WsWorker

    QThread* thread_ = nullptr;     // WebSocket 所在的子线程
    QObject* worker_ = nullptr;     // 实际持有 QWebSocket 的 worker 对象
    bool connected_ = false;        // 主线程维护的连接状态缓存
};

} // namespace netclient

#endif // NETCLIENT_WEBSOCKET_CLIENT_H
