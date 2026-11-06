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

// JSON-RPC 2.0 响应
struct NETCLIENT_API RpcResult {
    bool ok = false;
    QJsonValue result;       // 成功时的结果
    int errorCode = 0;       // 错误码
    QString errorMessage;    // 错误信息
};

using RpcCallback = std::function<void(const RpcResult&)>;

// JSON-RPC 2.0 客户端（基于 HTTP 传输）
// 支持请求-响应、通知（无响应）、批量请求
class NETCLIENT_API RpcClient : public QObject {
    Q_OBJECT
public:
    explicit RpcClient(const QUrl& endpoint, QObject* parent = nullptr);
    ~RpcClient() override;

    // 异步调用：method + params（对象或数组）
    void call(const QString& method, const QJsonValue& params, RpcCallback callback);

    // 异步通知（无 id，不需要响应）
    void notify(const QString& method, const QJsonValue& params = QJsonValue());

    // 同步调用（阻塞）
    RpcResult callSync(const QString& method, const QJsonValue& params,
                       int timeoutMs = 30000);

    // 设置超时
    void setTimeout(int ms);

private:
    QUrl endpoint_;
    HttpClient* http_ = nullptr;
    qint64 nextId_ = 1;
};

} // namespace netclient

#endif // NETCLIENT_RPC_CLIENT_H
