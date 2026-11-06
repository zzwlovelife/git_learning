#include "netclient/net_client.h"

namespace netclient {

// =============================================================================
// NetClient - 统一门面实现
// 构造时一次性创建 HTTP/WebSocket/MQTT 三个客户端，各自独立线程。
// 生命周期由 NetClient 管理（以 this 为 parent），析构时自动清理。
// =============================================================================

NetClient::NetClient(QObject* parent)
    : QObject(parent)
    , http_(new HttpClient(this))
    , ws_(new WebSocketClient(this))
    , mqtt_(new MqttClient(this)) {
}

NetClient::~NetClient() = default;

} // namespace netclient
