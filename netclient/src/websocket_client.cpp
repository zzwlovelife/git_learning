#include "netclient/websocket_client.h"

#include <QWebSocket>
#include <QThread>
#include <QTimer>
#include <QMetaObject>

namespace netclient {

// 内部 Worker：持有 QWebSocket，运行在独立线程
class WsWorker : public QObject {
    Q_OBJECT
public:
    explicit WsWorker(QObject* parent = nullptr) : QObject(parent) {
        ws_ = new QWebSocket(QString(), QWebSocketProtocol::VersionLatest, this);
        heartbeatTimer_ = new QTimer(this);
        heartbeatTimer_->setSingleShot(false);

        connect(ws_, &QWebSocket::connected, this, [this]() {
            connected_ = true;
            emit connected();
            if (heartbeatMs_ > 0) heartbeatTimer_->start(heartbeatMs_);
        });
        connect(ws_, &QWebSocket::disconnected, this, [this]() {
            connected_ = false;
            heartbeatTimer_->stop();
            emit disconnected();
        });
        connect(ws_, &QWebSocket::textMessageReceived,
                this, &WsWorker::textMessageReceived);
        connect(ws_, &QWebSocket::binaryMessageReceived,
                this, &WsWorker::binaryMessageReceived);

        // 心跳：定时发 Ping（QWebSocket 内部自动处理 Pong）
        connect(heartbeatTimer_, &QTimer::timeout, this, [this]() {
            if (connected_) ws_->ping();
        });
    }

    ~WsWorker() override {
        if (ws_) { ws_->close(); ws_->deleteLater(); }
    }

    void setHeartbeat(int ms) { heartbeatMs_ = ms; }

public slots:
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
    QWebSocket* ws_ = nullptr;
    QTimer* heartbeatTimer_ = nullptr;
    int heartbeatMs_ = 30000;
    bool connected_ = false;
};

// ---------------- WebSocketClient 实现 ----------------

WebSocketClient::WebSocketClient(QObject* parent)
    : QObject(parent) {
    initWorker();
}

WebSocketClient::~WebSocketClient() {
    if (thread_) {
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

    // 转发信号到主线程
    connect(worker, &WsWorker::connected, this, &WebSocketClient::onConnected);
    connect(worker, &WsWorker::disconnected, this, &WebSocketClient::onDisconnected);
    connect(worker, &WsWorker::textMessageReceived,
            this, &WebSocketClient::textMessageReceived);
    connect(worker, &WsWorker::binaryMessageReceived,
            this, &WebSocketClient::binaryMessageReceived);

    connect(thread_, &QThread::finished, worker, &QObject::deleteLater);
    thread_->start();
}

void WebSocketClient::open(const QUrl& url) {
    QMetaObject::invokeMethod(worker_, "open", Qt::QueuedConnection,
                              Q_ARG(QUrl, url));
}

void WebSocketClient::close() {
    QMetaObject::invokeMethod(worker_, "close", Qt::QueuedConnection);
}

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

void WebSocketClient::setHeartbeatInterval(int ms) {
    auto* w = static_cast<WsWorker*>(worker_);
    QMetaObject::invokeMethod(worker_, [w, ms]() {
        w->setHeartbeat(ms);
    }, Qt::QueuedConnection);
}

} // namespace netclient

// WsWorker 的 moc 需要在 cpp 内生成
#include "websocket_client.moc"
