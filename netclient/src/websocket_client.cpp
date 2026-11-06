#include "netclient/websocket_client.h"

#include <QWebSocket>
#include <QThread>
#include <QTimer>
#include <QMetaObject>

namespace netclient {

// =============================================================================
// WsWorker - 内部 Worker，持有 QWebSocket，运行在独立线程
// -----------------------------------------------------------------------------
// 为什么需要 Worker？QWebSocket 只能在创建它的线程使用。把 QWebSocket 放进
// Worker 并 moveToThread，确保所有 socket 操作都在子线程执行，主线程通过
// QMetaObject::invokeMethod 跨线程调用。
// =============================================================================
class WsWorker : public QObject {
    Q_OBJECT
public:
    explicit WsWorker(QObject* parent = nullptr) : QObject(parent) {
        ws_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
        heartbeatTimer_ = new QTimer(this);
        heartbeatTimer_->setSingleShot(false);  // 周期心跳

        // 连接成功：标记状态并启动心跳
        connect(ws_, &QWebSocket::connected, this, [this]() {
            connected_ = true;
            emit connected();
            if (heartbeatMs_ > 0) heartbeatTimer_->start(heartbeatMs_);
        });
        // 断开：停止心跳
        connect(ws_, &QWebSocket::disconnected, this, [this]() {
            connected_ = false;
            heartbeatTimer_->stop();
            emit disconnected();
        });
        // 消息转发
        connect(ws_, &QWebSocket::textMessageReceived,
                this, &WsWorker::textMessageReceived);
        connect(ws_, &QWebSocket::binaryMessageReceived,
                this, &WsWorker::binaryMessageReceived);

        // 心跳：定时发 Ping。QWebSocket 收到对端 Ping 时由 Qt 协议栈自动回 Pong；
        // 本端主动 ping() 后等待对端 Pong（QWebSocket 内部处理，无需手动计时）
        connect(heartbeatTimer_, &QTimer::timeout, this, [this]() {
            if (connected_) ws_->ping();
        });
    }

    ~WsWorker() override {
        if (ws_) { ws_->close(); ws_->deleteLater(); }
    }

    void setHeartbeat(int ms) { heartbeatMs_ = ms; }

public slots:
    // 以下槽由主线程通过 invokeMethod 跨线程调用，在子线程执行
    void open(const QUrl& url) { ws_->open(url); }
    void close() { ws_->close(); }
    void sendText(const QString& text) { if (connected_) ws_->sendTextMessage(text); }
    void sendBinary(const QByteArray& data) { if (connected_) ws_->sendBinaryMessage(data); }

signals:
    void connected();
    void disconnected();
    void textMessageReceived(const QString& message);
    void binaryMessageReceived(const QByteArray& data);

private:
    QWebSocket* ws_ = nullptr;        // WebSocket 连接
    QTimer* heartbeatTimer_ = nullptr; // 心跳定时器
    int heartbeatMs_ = 30000;          // 心跳间隔（毫秒）
    bool connected_ = false;           // 子线程内的连接状态
};

// =============================================================================
// WebSocketClient 实现（主线程代理）
// =============================================================================

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent) {
    initWorker();  // 创建子线程与 Worker
}

WebSocketClient::~WebSocketClient() {
    if (thread_) {
        // 阻塞调用 close，确保断开完成后再退出线程
        QMetaObject::invokeMethod(worker_, "close", Qt::BlockingQueuedConnection);
        thread_->quit();
        thread_->wait();
        delete thread_;
    }
}

void WebSocketClient::initWorker() {
    thread_ = new QThread(this);
    auto* worker = new WsWorker();  // 无 parent，稍后 moveToThread
    worker_ = worker;
    worker->moveToThread(thread_);

    // 子线程信号 -> 主线程槽（自动 QueuedConnection）
    connect(worker, &WsWorker::connected, this, &WebSocketClient::onConnected);
    connect(worker, &WsWorker::disconnected, this, &WebSocketClient::onDisconnected);
    connect(worker, &WsWorker::textMessageReceived,
            this, &WebSocketClient::textMessageReceived);
    connect(worker, &WsWorker::binaryMessageReceived,
            this, &WebSocketClient::binaryMessageReceived);

    // 线程结束时自动删除 worker
    connect(thread_, &QThread::finished, worker, &QObject::deleteLater);
    thread_->start();
}

// ---- 跨线程调用：用 invokeMethod 投递到子线程 ----
void WebSocketClient::open(const QUrl& url) {
    QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection,
                              Q_ARG(QUrl, url));
}

void WebSocketClient::close() {
    QMetaObject::invokeMethod(worker_, "close", Qt::QueuedConnection);
}

// 状态槽：子线程信号触发，更新主线程缓存的连接状态并二次 emit
void WebSocketClient::onConnected() {
    connected_ = true;
    emit connected();
}

void WebSocketClient::onDisconnected() {
    connected_ = false;
    emit disconnected();
}

bool WebSocketClient::isConnected() const {
    return connected_;
}

void WebSocketClient::sendText(const QString& text) {
    QMetaObject::invokeMethod(worker_, "sendText", Qt::QueuedConnection,
                              Q_ARG(QString, text));
}

void WebSocketClient::sendBinary(const QByteArray& data) {
    QMetaObject::invokeMethod(worker_, "sendBinary", Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));
}

// ---- 回调桥接：把信号转成 std::function 回调 ----
void WebSocketClient::setTextCallback(WsTextCallback cb) {
    connect(this, &WebSocketClient::textMessageReceived, this,
            [cb](const QString& m) { if (cb) cb(m); });
}

void WebSocketClient::setBinaryCallback(WsBinaryCallback cb) {
    connect(this, &WebSocketClient::binaryMessageReceived, this,
            [cb](const QByteArray& d) { if (cb) cb(d); });
}

void WebSocketClient::setConnectedCallback(WsStateCallback cb) {
    connect(this, &WebSocketClient::connected, this, [cb]() { if (cb) cb(true); });
    connect(this, &WebSocketClient::disconnected, this, [cb]() { if (cb) cb(false); });
}

// 设置心跳间隔：跨线程调用 worker 的 setHeartbeat
void WebSocketClient::setHeartbeatInterval(int ms) {
    auto* w = static_cast<WsWorker*>(worker_);
    QMetaObject::invokeMethod(worker_, [w, ms]() {
        w->setHeartbeat(ms);
    }, Qt::QueuedConnection);
}

} // namespace netclient

// WsWorker 的 moc 需要在 cpp 内生成（因为 Q_OBJECT 类定义在 .cpp 中）
#include "websocket_client.moc"
