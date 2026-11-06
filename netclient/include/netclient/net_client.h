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

// =============================================================================
// NetClient - 统一门面（Facade）
// -----------------------------------------------------------------------------
// 聚合 HTTP / WebSocket / MQTT / RPC 四种客户端，对外提供统一入口。
// 每种客户端运行在各自独立线程，相互不阻塞，可并发使用。
//
// 用法：
//   NetClient net;
//   net.http()->get(url, cb);
//   net.mqtt()->connectToHost(cfg);
//   auto* rpc = net.createRpc(QUrl("http://host/rpc"));
// =============================================================================
class NETCLIENT_API NetClient : public QObject {
    Q_OBJECT
public:
    explicit NetClient(QObject* parent = nullptr);
    ~NetClient() override;

    HttpClient* http() { return http_; }                 // HTTP 客户端
    WebSocketClient* websocket() { return ws_; }         // WebSocket 客户端
    MqttClient* mqtt() { return mqtt_; }                 // MQTT 客户端

    // 创建一个 RPC 客户端（每个 endpoint 独立实例，由 NetClient 管理生命周期）
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
