#include "netclient/rpc_client.h"
#include "netclient/http_client.h"

#include <QJsonDocument>
#include <QJsonObject>

namespace netclient {

// =============================================================================
// RpcClient 实现 - JSON-RPC 2.0 over HTTP
// =============================================================================

RpcClient::RpcClient(const QUrl& endpoint, QObject* parent)
    : QObject(parent)
    , endpoint_(endpoint)
    , http_(new HttpClient(this))  // 复用 HttpClient 作为传输层
{
}

RpcClient::~RpcClient() = default;

void RpcClient::setTimeout(int ms) { http_->setTimeout(ms); }

// -----------------------------------------------------------------------------
// buildRequest - 构造 JSON-RPC 2.0 请求对象
// hasId=true  时带 id（请求-响应模式，服务器需回对应 id）
// hasId=false 时不带 id（通知模式，服务器不响应）
// -----------------------------------------------------------------------------
static QJsonObject buildRequest(const QString& method, const QJsonValue& params,
                                qint64 id, bool hasId) {
    QJsonObject req;
    req["jsonrpc"] = "2.0";              // 协议版本字段（必需）
    req["method"] = method;              // 方法名
    if (!params.isUndefined() && !params.isNull()) {
        req["params"] = params;          // 参数（对象或数组，可选）
    }
    if (hasId) req["id"] = static_cast<double>(id);  // 请求 id（通知模式省略）
    return req;
}

// -----------------------------------------------------------------------------
// parseResponse - 解析 JSON-RPC 2.0 响应
// 响应格式：{ "jsonrpc":"2.0", "result":..., "id":N }  成功
//           { "jsonrpc":"2.0", "error":{"code":N,"message":"..."}, "id":N }  失败
// -----------------------------------------------------------------------------
static RpcResult parseResponse(const QByteArray& body) {
    RpcResult r;
    QJsonDocument doc = QJsonDocument::fromJson(body);
    if (!doc.isObject()) {
        r.errorMessage = "invalid JSON response";
        return r;
    }
    QJsonObject obj = doc.object();
    // 优先检查 error 字段
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

// 异步调用（请求-响应）：带 id，服务器回包后通过 callback 返回结果
void RpcClient::call(const QString& method, const QJsonValue& params,
                     RpcCallback callback) {
    qint64 id = nextId_++;  // 自增 id，用于匹配响应
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

// 异步通知：不带 id，服务器无需响应（适用于上报、心跳等）
void RpcClient::notify(const QString& method, const QJsonValue& params) {
    QJsonObject req = buildRequest(method, params, 0, false);
    QByteArray data = QJsonDocument(req).toJson(QJsonDocument::Compact);
    http_->post(endpoint_, data, "application/json", nullptr);
}

// 同步调用：转调 HttpClient::postSync 阻塞等待
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
