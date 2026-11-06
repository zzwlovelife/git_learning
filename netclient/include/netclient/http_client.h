#ifndef NETCLIENT_HTTP_CLIENT_H
#define NETCLIENT_HTTP_CLIENT_H

#include <QObject>
#include <QByteArray>
#include <QString>
#include <QUrl>
#include <QMap>
#include <functional>

#include "netclient/netclient_global.h"

class QNetworkAccessManager;
class QNetworkReply;

namespace netclient {

// =============================================================================
// HttpResponse - HTTP 响应结果封装
// -----------------------------------------------------------------------------
// 所有 HTTP 方法（同步/异步）的返回结构。
// ok=true 表示请求成功且状态码在 2xx 范围；否则查 errorString / statusCode。
// =============================================================================
struct NETCLIENT_API HttpResponse {
    bool ok = false;                   // 请求是否成功（2xx 且无网络错误）
    int statusCode = 0;                // HTTP 状态码（200/404/500...）
    QByteArray body;                   // 响应体（原始字节）
    QString errorString;               // 错误描述（ok=false 时有效）
    QMap<QString, QString> headers;    // 响应头（key->value）
};

// 异步回调签名：请求完成后在调用方线程触发
using HttpCallback = std::function<void(const HttpResponse&)>;

// =============================================================================
// HttpClient - HTTP 客户端
// -----------------------------------------------------------------------------
// 基于 QNetworkAccessManager（QNAM）。QNAM 内部自带线程池处理 DNS/连接，
// 本类直接在调用线程使用 QNAM，异步回调通过信号槽回到调用线程。
//
// 支持方法：GET / POST / PUT / DELETE
// 两种调用模式：
//   - 异步（get/post/put/deleteResource）：立即返回，结果通过 callback 回调
//   - 同步（getSync/postSync）：内部用 QEventLoop 阻塞等待，可在任意线程调用
//
// 并发：QNAM 本身支持并发请求，多个异步请求可同时发起。
// =============================================================================
class NETCLIENT_API HttpClient : public QObject {
    Q_OBJECT
public:
    explicit HttpClient(QObject* parent = nullptr);
    ~HttpClient() override;

    // ---- 异步接口 ----
    // 异步 GET：url=请求地址，callback=完成回调，headers=自定义请求头
    void get(const QUrl& url, HttpCallback callback,
             const QMap<QString, QString>& headers = {});

    // 异步 POST：data=请求体，contentType=Content-Type 头
    void post(const QUrl& url, const QByteArray& data,
              const QByteArray& contentType = "application/json",
              HttpCallback callback = nullptr,
              const QMap<QString, QString>& headers = {});

    // 异步 PUT
    void put(const QUrl& url, const QByteArray& data,
             const QByteArray& contentType = "application/json",
             HttpCallback callback = nullptr,
             const QMap<QString, QString>& headers = {});

    // 异步 DELETE
    void deleteResource(const QUrl& url, HttpCallback callback = nullptr,
                        const QMap<QString, QString>& headers = {});

    // ---- 同步接口（阻塞，内部用 QEventLoop 等待；可在任意线程调用）----
    HttpResponse getSync(const QUrl& url,
                         const QMap<QString, QString>& headers = {},
                         int timeoutMs = 30000);

    HttpResponse postSync(const QUrl& url, const QByteArray& data,
                          const QByteArray& contentType = "application/json",
                          const QMap<QString, QString>& headers = {},
                          int timeoutMs = 30000);

    // 设置默认超时（毫秒），同步接口可单独指定
    void setTimeout(int ms);

private:
    // 内部统一请求分发：根据 method 构造 QNetworkRequest 并发送
    void doRequest(const QByteArray& method, const QUrl& url,
                   const QByteArray& data, const QByteArray& contentType,
                   const QMap<QString, QString>& headers, HttpCallback callback);

    QNetworkAccessManager* nam_ = nullptr;  // Qt 网络访问管理器
    int timeoutMs_ = 30000;                 // 默认超时（毫秒）
};

} // namespace netclient

#endif // NETCLIENT_HTTP_CLIENT_H
