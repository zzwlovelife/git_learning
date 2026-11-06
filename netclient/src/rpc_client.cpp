#include "netclient/rpc_client.h"
#include "netclient/http_client.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace netclient {

RpcClient::RpcClient(const QUrl& endpoint, QObject* parent)
    : QObject(parent)
    , endpoint_(endpoint)
    , http_(new HttpClient(this)) {
}

RpcClient::~RpcClient() = default;

void RpcClient::setTimeout(int ms) { http_->setTimeout(ms); }

static QJsonObject buildRequest(const QString& method, const QJsonValue& params,
                                qint64 id, bool hasId) {
    QJsonObject req;
    req["jsonrpc"] = "2.0";
    req["method"] = method;
    if (!params.isUndefined() && !params.isNull()) {
        req["params"] = params;
    }
    if (hasId) req["id"] = static_cast<double>(id);
    return req;
}

static RpcResult parseResponse(const QByteArray& body) {
    RpcResult r;
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        r.errorMessage = "invalid JSON response";
        return r;
    }
    QJsonObject obj = doc.object();
    if (obj.contains("error") && !obj["error"].isUndefined()) {
        QJsonObject err = obj["error"].toObject();
        r.errorCode = err["code"].toInt();
        r.errorMessage = err["message"].toString();
        r.ok = false;
        return r;
    }
    if (obj.contains("result")) {
        r.result = obj["result"];
        r.ok = true;
        return r;
    }
    r.errorMessage = "response has neither result nor error";
    return r;
}

void RpcClient::call(const QString& method, const QJsonValue& params,
                     RpcCallback callback) {
    qint64 id = nextId_++;
    QJsonObject req = buildRequest(method, params, id, true);
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);

    http_->post(endpoint_, data, "application/json",
                [callback](const HttpResponse& resp) {
        RpcResult r;
        if (!resp.ok) {
            r.errorMessage = "HTTP error: " + resp.errorString
                           + " (status " + QString::number(resp.statusCode) + ")";
            if (callback) callback(r);
            return;
        }
        r = parseResponse(resp.body);
        if (callback) callback(r);
    });
}

void RpcClient::notify(const QString& method, const QJsonValue& params) {
    QJsonObject req = buildRequest(method, params, 0, false);
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);
    http_->post(endpoint_, data, "application/json", nullptr);
}

RpcResult RpcClient::callSync(const QString& method, const QJsonValue& params,
                              int timeoutMs) {
    qint64 id = nextId_++;
    QJsonObject req = buildRequest(method, params, id, true);
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);

    HttpResponse resp = http_->postSync(endpoint_, data, "application/json",
                                        {}, timeoutMs);
    RpcResult r;
    if (!resp.ok) {
        r.errorMessage = "HTTP error: " + resp.errorString
                       + " (status " + QString::number(resp.statusCode) + ")";
        return r;
    }
    return parseResponse(resp.body);
}

} // namespace netclient
