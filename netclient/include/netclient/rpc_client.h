#ifndef NETCLIENT_RPC_CLIENT_H
#define NETCLIENT_RPC_CLIENT_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>
#include <QUrl>
#include <functional>

#include "netclient/netclient_global.h"

namespace netclient {

class HttpClient;

// =============================================================================
// RpcResult - JSON-RPC 2.0 响应结果
// -----------------------------------------------------------------------------
// ok=true 时 result 有效；ok=false 时 errorCode/errorMessage 有效。
// =============================================================================
struct NETCLIENT_API RpcResult {
    bool ok = false;               // 调用是否成功
    QJsonValue result;             // 成功时的返回值（对象/数组/基本类型）
    int errorCode = 0;             // 错误码（JSON-RPC 标准或自定义）
    QString errorMessage;          // 错误描述
};

using RpcCallback = std::function<void(const RpcResult&)>;

// =============================================================================
// RpcClient - JSON-RPC 2.0 客户端（基于 HTTP 传输）
// -----------------------------------------------------------------------------
// 复用 HttpClient 作为传输层，把 JSON-RPC 请求体 POST 到 endpoint。
// 支持三种调用：
//   - call()     异步请求-响应（带 id，服务器需返回对应 id 的响应）
//   - notify()   异步通知（无 id，服务器不响应）
//   - callSync() 同步请求-响应（阻塞等待）
//
// 设计取舍：未使用 gRPC/protobuf，避免引入额外依赖；JSON-RPC 基于 HTTP，
// 纯 Qt 实现，跨语言互通性好。
// =============================================================================
class NETCLIENT_API RpcClient : public QObject {
    Q_OBJECT
public:
    // endpoint=JSON-RPC 服务端 URL
    explicit RpcClient(const QUrl& endpoint, QObject* parent = nullptr);
    ~RpcClient() override;

    // 异步调用：method=方法名，params=参数（QJsonObject 或 QJsonArray）
    void call(const QString& method, const QJsonValue& params, RpcCallback callback);

    // 异步通知（不带 id，服务器无需响应，适用于上报类场景）
    void notify(const QString& method, const QJsonValue& params = QJsonValue());

    // 同步调用（阻塞，内部转调 HttpClient::postSync）
    RpcResult callSync(const QString& method, const QJsonValue& params,
                       int timeoutMs = 30000);

    // 设置超时（毫秒）
    void setTimeout(int ms);

private:
    QUrl endpoint_;              // JSON-RPC 服务端地址
    HttpClient* http_ = nullptr; // 底层 HTTP 传输（复用 HttpClient）
    qint64 nextId_ = 1;          // 请求 id 自增计数器
};

} // namespace netclient

#endif // NETCLIENT_RPC_CLIENT_H
