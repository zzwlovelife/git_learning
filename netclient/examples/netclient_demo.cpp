// netclient 综合示例：HTTP / WebSocket / MQTT / RPC 客户端演示
// HTTP 与 RPC 使用本地 QTcpServer 验证（无需外网）
// WebSocket 与 MQTT 尝试公共服务器，失败则演示 API 调用流程
#include <QCoreApplication>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include <QEventLoop>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

#include <netclient/net_client.h>

using namespace netclient;

// 简易本地 HTTP 服务器：处理 GET / 与 JSON-RPC POST
class LocalServer : public QObject {
    Q_OBJECT
public:
    explicit LocalServer(QObject* parent = nullptr) : QObject(parent) {
        server_ = new QTcpServer(this);
        connect(server_, &QTcpServer::newConnection, this, &LocalServer::onNewConn);
    }
    bool start(quint16 port) {
        return server_->listen(QHostAddress::LocalHost, port);
    }
    quint16 port() const { return server_->serverPort(); }
private slots:
    void onNewConn() {
        auto* sock = server_->nextPendingConnection();
        connect(sock, &QTcpSocket::readyRead, this, [this, sock]() {
            QByteArray data = sock->readAll();
            QByteArray resp;
            if (data.startsWith("GET ")) {
                resp = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: 29\r\n\r\n"
                       "{\"message\":\"hello from local\"}";
            } else if (data.startsWith("POST ")) {
                // 解析 JSON-RPC
                int idx = data.indexOf("\r\n\r\n");
                QByteArray body = data.mid(idx + 4);
                QJsonDocument doc = QJsonDocument::fromJson(body);
                QJsonObject req = doc.object();
                QJsonObject result;
                if (req["method"].toString() == "echo") {
                    result = req["params"].toObject();
                } else if (req["method"].toString() == "add") {
                    QJsonArray arr = req["params"].toArray();
                    int sum = arr[0].toInt() + arr[1].toInt();
                    result["sum"] = sum;
                } else {
                    result["ok"] = true;
                }
                QJsonObject respObj;
                respObj["jsonrpc"] = "2.0";
                respObj["result"] = result;
                if (req.contains("id")) respObj["id"] = req["id"];
                QByteArray respBody = QJsonDocument(respObj).toJson(QJsonDocument::Compact);
                resp = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: " + QByteArray::number(respBody.size()) +
                       "\r\n\r\n" + respBody;
            }
            sock->write(resp);
            sock->disconnectFromHost();
        });
    }
private:
    QTcpServer* server_ = nullptr;
};

int main(int argc, char* argv[]) {
    QCoreApplication app(argc, argv);
    qInfo() << "=== netclient demo start ===";

    // 启动本地服务器
    LocalServer local;
    if (!local.start(0)) {
        qCritical() << "failed to start local server";
        return 1;
    }
    QString baseUrl = QString("http://127.0.0.1:%1").arg(local.port());
    qInfo() << "local HTTP server at" << baseUrl;

    NetClient net;

    // ---------- 1. HTTP 测试 ----------
    qInfo() << "\n--- HTTP test ---";
    HttpResponse hr = net.http()->getSync(QUrl(baseUrl + "/"));
    qInfo() << "HTTP GET ok=" << hr.ok << "status=" << hr.statusCode
            << "body=" << hr.body;

    // 异步 HTTP
    QEventLoop loop;
    net.http()->get(QUrl(baseUrl + "/"), [&](const HttpResponse& r) {
        qInfo() << "HTTP async get ok=" << r.ok << "body=" << r.body;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    // ---------- 2. RPC 测试（JSON-RPC over HTTP） ----------
    qInfo() << "\n--- RPC test (JSON-RPC 2.0) ---";
    RpcClient* rpc = net.createRpc(QUrl(baseUrl + "/rpc"));

    // add 方法
    RpcResult rr = rpc->callSync("add", QJsonArray{3, 4});
    qInfo() << "RPC add(3,4) ok=" << rr.ok
            << "sum=" << rr.result.toObject()["sum"].toInt();

    // echo 方法
    QJsonObject echoParams;
    echoParams["msg"] = "hello rpc";
    rr = rpc->callSync("echo", echoParams);
    qInfo() << "RPC echo ok=" << rr.ok
            << "result=" << QJsonDocument(rr.result.toObject()).toJson(QJsonDocument::Compact);

    // ---------- 3. WebSocket 测试 ----------
    qInfo() << "\n--- WebSocket test ---";
    WebSocketClient* ws = net.websocket();
    ws->setHeartbeatInterval(10000);
    ws->setConnectedCallback([](bool connected) {
        qInfo() << "WebSocket" << (connected ? "connected" : "disconnected");
    });
    ws->setTextCallback([](const QString& msg) {
        qInfo() << "WebSocket recv:" << msg;
    });
    ws->open(QUrl("wss://ws.postman-echo.com/raw"));
    // 尝试连接 4 秒
    QEventLoop wsLoop;
    QTimer::singleShot(4000, &wsLoop, &QEventLoop::quit);
    wsLoop.exec();
    if (ws->isConnected()) {
        ws->sendText("hello websocket");
        QEventLoop recvLoop;
        QTimer::singleShot(2000, &recvLoop, &QEventLoop::quit);
        recvLoop.exec();
    } else {
        qInfo() << "WebSocket: public echo server unreachable (API demonstrated)";
    }
    ws->close();

    // ---------- 4. MQTT 测试 ----------
    qInfo() << "\n--- MQTT test ---";
    MqttClient* mqtt = net.mqtt();
    mqtt->setConnectedCallback([](bool connected) {
        qInfo() << "MQTT" << (connected ? "connected" : "disconnected");
    });
    mqtt->setMessageCallback([](const QString& topic, const QByteArray& payload) {
        qInfo() << "MQTT recv topic=" << topic << "payload=" << payload;
    });
    MqttConfig cfg;
    cfg.host = "broker.emqx.io";
    cfg.port = 1883;
    cfg.clientId = "netclient_demo_" + QString::number(QCoreApplication::applicationPid());
    cfg.keepAliveSec = 30;
    mqtt->connectToHost(cfg);

    QEventLoop mqttLoop;
    QTimer::singleShot(5000, &mqttLoop, &QEventLoop::quit);
    mqttLoop.exec();

    if (mqtt->isConnected()) {
        mqtt->subscribe("netclient/demo/test", 0);
        mqtt->publish("netclient/demo/test", "hello mqtt from netclient");
        QEventLoop recvLoop;
        QTimer::singleShot(3000, &recvLoop, &QEventLoop::quit);
        recvLoop.exec();
    } else {
        qInfo() << "MQTT: public broker unreachable (API demonstrated)";
    }
    mqtt->disconnectFromHost();

    qInfo() << "\n=== netclient demo done ===";
    return 0;
}

#include "netclient_demo.moc"
