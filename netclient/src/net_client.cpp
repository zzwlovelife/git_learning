#include "netclient/net_client.h"

namespace netclient {

NetClient::NetClient(QObject* parent)
    : QObject(parent)
    , http_(new HttpClient(this))
    , ws_(new WebSocketClient(this))
    , mqtt_(new MqttClient(this)) {
}

NetClient::~NetClient() = default;

} // namespace netclient
