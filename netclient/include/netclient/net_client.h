#ifndef NETCLIENT_NET_CLIENT_H
#define NETCLIENT_NET_CLIENT_H

#include <QObject>
#include <QUrl>

#include "netclient/netclient_global.h"
#include "netclient/http_client.h"
#include "netclient/websocket_client.h"
#include "netclient/mqtt_client.h"
#include "netclient/rpc_client.h"

namespace netclient {

// 统一门面：持有 HTTP / WebSocket / MQTT / RPC 四种客户端
// 每种客户端独立线程，可并发使用
class NETCLIENT_API NetClient : public QObject {
    Q_OBJECT
public:
    explicit NetClient(QObject* parent = nullptr);
    ~NetClient() override;

    HttpClient* http() { return http_; }
    WebSocketClient* websocket() { return ws_; }
    MqttClient* mqtt() { return mqtt_; }

    // 创建一个 RPC 客户端（每个 endpoint 一个实例）
    RpcClient* createRpc(const QUrl& endpoint) {
        return new RpcClient(endpoint, this);
    }

private:
    HttpClient* http_ = nullptr;
    WebSocketClient* ws_ = nullptr;
    MqttClient* mqtt_ = nullptr;
};

} // namespace netclient

#endif // NETCLIENT_NET_CLIENT_H
