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

// HTTP 响应结果
struct NETCLIENT_API HttpResponse {
    bool ok = false;
    int statusCode = 0;          // HTTP 状态码
    QByteArray body;             // 响应体
    QString errorString;         // 错误信息
    QMap<QString, QString> headers; // 响应头
};

// 异步回调签名
using HttpCallback = std::function<void(const HttpResponse&)>;

// HTTP 客户端：基于 QNetworkAccessManager
// 内部在专用线程运行 QNAM，支持多线程并发调用
class NETCLIENT_API HttpClient : public QObject {
    Q_OBJECT
public:
    explicit HttpClient(QObject* parent = nullptr);
    ~HttpClient() override;

    // 异步 GET
    void get(const QUrl& url, HttpCallback callback,
             const QMap<QString, QString>& headers = {});

    // 异步 POST
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

    // 同步 GET（阻塞，可在任意线程调用）
    HttpResponse getSync(const QUrl& url,
                         const QMap<QString, QString>& headers = {},
                         int timeoutMs = 30000);

    // 同步 POST
    HttpResponse postSync(const QUrl& url, const QByteArray& data,
                          const QByteArray& contentType = "application/json",
                          const QMap<QString, QString>& headers = {},
                          int timeoutMs = 30000);

    // 设置超时（毫秒）
    void setTimeout(int ms);

private:
    void doRequest(const QByteArray& method, const QUrl& url,
                   const QByteArray& data, const QByteArray& contentType,
                   const QMap<QString, QString>& headers, HttpCallback callback);

    QNetworkAccessManager* nam_ = nullptr;
    int timeoutMs_ = 30000;
};

} // namespace netclient

#endif // NETCLIENT_HTTP_CLIENT_H
