// =============================================================================
// netclient 综合示例：演示 HTTP / WebSocket / MQTT / RPC 四种客户端的使用
// -----------------------------------------------------------------------------
// - HTTP  与 RPC 使用本地 QTcpServer 模拟服务端，无需外网即可验证
// - WebSocket 尝试连接公共 echo 服务器（wss://ws.postman-echo.com/raw）
// - MQTT     尝试连接公共 Broker（broker.emqx.io:1883）
// 公共服务器不可达时，代码会打印 "API demonstrated"，仍展示完整调用流程。
// =============================================================================
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

// =============================================================================
// LocalServer - 简易本地 HTTP/JSON-RPC 服务器
// 处理两类请求：
//   GET  /           -> 返回固定 JSON
//   POST /           -> 解析 JSON-RPC 请求，支持 echo / add 方法
// =============================================================================
class LocalServer : public QObject {
    Q_OBJECT
public:
    explicit LocalServer(QObject* parent = nullptr) : QObject(parent) {
        server_ = new QTcpServer(this);
        connect(server_, &QTcpServer::newConnection, this, &LocalServer::onNewConn);
    }
    // 监听本地端口（port=0 表示由系统分配空闲端口）
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
                // GET 请求：返回固定 JSON
                resp = "HTTP/1.1 200 OK\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: 29\r\n\r\n"
                       "{\"message\":\"hello from local\"}";
            } else if (data.startsWith("POST ")) {
                // POST 请求：解析 JSON-RPC 体
                int idx = data.indexOf("\r\n\r\n");
                QByteArray body = data.mid(idx + 4);
                QJsonDocument doc = QJsonDocument::fromJson(body);
                QJsonObject req = doc.object();
                QJsonObject result;
                if (req["method"].toString() == "echo") {
                    // echo：原样返回 params
                    result = req["params"].toObject();
                } else if (req["method"].toString() == "add") {
                    // add：数组前两数相加
                    QJsonArray arr = req["params"].toArray();
                    int sum = arr[0].toInt() + arr[1].toInt();
                    result["sum"] = sum;
                } else {
                    result["ok"] = true;
                }
                // 构造 JSON-RPC 响应
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

    // 启动本地 HTTP/JSON-RPC 服务器
    LocalServer local;
    if (!local.start(0)) {
        qCritical() << "failed to start local server";
        return 1;
    }
    QString baseUrl = QString("http://127.0.0.1:%1").arg(local.port());
    qInfo() << "local HTTP server at" << baseUrl;

    // 创建统一门面（内含 HTTP/WebSocket/MQTT 三个独立线程的客户端）
    NetClient net;

    // ---------- 1. HTTP 测试 ----------
    qInfo() << "\n--- HTTP test ---";
    // 同步 GET
    HttpResponse hr = net.http()->getSync(QUrl(baseUrl + "/"));
    qInfo() << "HTTP GET ok=" << hr.ok << "status=" << hr.statusCode
            << "body=" << hr.body;

    // 异步 GET（用 QEventLoop 等待回调）
    QEventLoop loop;
    net.http()->get(QUrl(baseUrl + "/"), [&](const HttpResponse& r) {
        qInfo() << "HTTP async get ok=" << r.ok << "body=" << r.body;
        loop.quit();
    });
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);  // 5s 超时兜底
    loop.exec();

    // ---------- 2. RPC 测试（JSON-RPC over HTTP） ----------
    qInfo() << "\n--- RPC test (JSON-RPC 2.0) ---";
    RpcClient* rpc = net.createRpc(QUrl(baseUrl + "/rpc"));

    // 同步调用 add(3, 4)
    RpcResult rr = rpc->callSync("add", QJsonArray{3, 4});
    qInfo() << "RPC add(3,4) ok=" << rr.ok
            << "sum=" << rr.result.toObject()["sum"].toInt();

    // 同步调用 echo
    QJsonObject echoParams;
    echoParams["msg"] = "hello rpc";
    rr = rpc->callSync("echo", echoParams);
    qInfo() << "RPC echo ok=" << rr.ok
            << "result=" << QJsonDocument(rr.result.toObject()).toJson(QJsonDocument::Compact);

    // ---------- 3. WebSocket 测试 ----------
    qInfo() << "\n--- WebSocket test ---";
    WebSocketClient* ws = net.websocket();
    ws->setHeartbeatInterval(10000);  // 10s 心跳
    ws->setConnectedCallback([](bool connected) {
        qInfo() << "WebSocket" << (connected ? "connected" : "disconnected");
    });
    ws->setTextCallback([](const QString& msg) {
        qInfo() << "WebSocket recv:" << msg;
    });
    ws->open(QUrl("wss://ws.postman-echo.com/raw"));
    // 等待连接最多 4 秒
    QEventLoop wsLoop;
    QTimer::singleShot(4000, &wsLoop, &QEventLoop::quit);
    wsLoop.exec();
    if (ws->isConnected()) {
        ws->sendText("hello websocket");
        // 等待 echo 回显
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

    // 等待连接最多 5 秒
    QEventLoop mqttLoop;
    QTimer::singleShot(5000, &mqttLoop, &QEventLoop::quit);
    mqttLoop.exec();

    if (mqtt->isConnected()) {
        mqtt->subscribe("netclient/demo/test", 0);
        mqtt->publish("netclient/demo/test", "hello mqtt from netclient");
        // 等待回显（自己订阅自己发布的主题）
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
