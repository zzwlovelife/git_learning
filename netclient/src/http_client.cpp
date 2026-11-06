#include "netclient/http_client.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QThread>

namespace netclient {

// =============================================================================
// HttpClient 实现
// =============================================================================

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))  // QNAM 内部线程池处理 DNS/连接
{
}

HttpClient::~HttpClient() = default;

void HttpClient::setTimeout(int ms) { timeoutMs_ = ms; }

// -----------------------------------------------------------------------------
// doRequest - 异步请求核心分发
// 所有异步方法（get/post/put/deleteResource）都汇聚到这里。
// 流程：构造 QNetworkRequest -> 选方法发送 -> 挂超时看门狗 -> finished 时解析响应
// -----------------------------------------------------------------------------
void HttpClient::doRequest(const QByteArray& method, const QUrl& url,
                           const QByteArray& data, const QByteArray& contentType,
                           const QMap<QString, QString>& headers,
                           HttpCallback callback) {
    // 1. 构造请求并设置自定义头
    QNetworkRequest req(url);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    if (!contentType.isEmpty()) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    }

    // 2. 按方法名分发到 QNAM 对应接口
    QNetworkReply* reply = nullptr;
    if (method == "GET") {
        reply = nam_->get(req);
    } else if (method == "POST") {
        reply = nam_->post(req, data);
    } else if (method == "PUT") {
        reply = nam_->put(req, data);
    } else if (method == "DELETE") {
        reply = nam_->deleteResource(req);
    }

    // 不支持的方法：直接回调错误
    if (!reply) {
        HttpResponse r;
        r.errorString = "unsupported method: " + method;
        if (callback) callback(r);
        return;
    }

    // 3. 超时看门狗：超时未完成则 abort（QNAM 自身无超时机制）
    auto* timer = new QTimer(reply);  // 挂到 reply 上，reply 删除时自动释放
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, reply, [reply, callback]() {
        Q_UNUSED(callback);
        if (!reply->isFinished()) {
            reply->abort();  // 触发 finished，错误码为 OperationCanceledError
        }
    });
    timer->start(timeoutMs_);

    // 4. 响应完成：解析状态码/响应体/响应头，回调通知
    QObject::connect(reply, &QNetworkReply::finished, reply, [reply, timer, callback]() {
        timer->stop();
        HttpResponse r;
        r.statusCode = reply->attribute(
            QNetworkRequest::HttpStatusCodeAttribute).toInt();
        r.ok = (reply->error() == QNetworkReply::NoError);
        r.body = reply->readAll();
        if (!r.ok) {
            r.errorString = reply->errorString();
        }
        // 收集响应头
        const auto rawHeaders = reply->rawHeaderPairs();
        for (const auto& p : rawHeaders) {
            r.headers.insert(QString::fromUtf8(p.first),
                             QString::fromUtf8(p.second));
        }
        reply->deleteLater();
        if (callback) callback(r);
    });
}

// ---- 异步方法薄封装 ----
void HttpClient::get(const QUrl& url, HttpCallback callback,
                     const QMap<QString, QString>& headers) {
    doRequest("GET", url, {}, {}, headers, callback);
}

void HttpClient::post(const QUrl& url, const QByteArray& data,
                      const QByteArray& contentType, HttpCallback callback,
                      const QMap<QString, QString>& headers) {
    doRequest("POST", url, data, contentType, headers, callback);
}

void HttpClient::put(const QUrl& url, const QByteArray& data,
                     const QByteArray& contentType, HttpCallback callback,
                     const QMap<QString, QString>& headers) {
    doRequest("PUT", url, data, contentType, headers, callback);
}

void HttpClient::deleteResource(const QUrl& url, HttpCallback callback,
                                const QMap<QString, QString>& headers) {
    doRequest("DELETE", url, {}, {}, headers, callback);
}

// -----------------------------------------------------------------------------
// 同步 GET：用局部 QEventLoop 阻塞等待 finished
// 注意：QEventLoop 必须在有事件循环的线程中使用；调用线程若无事件循环，
//       QNetworkReply 的信号可能无法投递。业务线程（有 exec()）可安全使用。
// -----------------------------------------------------------------------------
HttpResponse HttpClient::getSync(const QUrl& url,
                                 const QMap<QString, QString>& headers,
                                 int timeoutMs) {
    QNetworkRequest req(url);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }

    QNetworkReply* reply = nam_->get(req);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        if (reply && !reply->isFinished()) reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();  // 阻塞直到 finished 或超时

    HttpResponse r;
    r.statusCode = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.ok = (reply->error() == QNetworkReply::NoError);
    r.body = reply->readAll();
    if (!r.ok) r.errorString = reply->errorString();
    reply->deleteLater();
    return r;
}

// 同步 POST（逻辑同 getSync）
HttpResponse HttpClient::postSync(const QUrl& url, const QByteArray& data,
                                  const QByteArray& contentType,
                                  const QMap<QString, QString>& headers,
                                  int timeoutMs) {
    QNetworkRequest req(url);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    if (!contentType.isEmpty()) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    }

    QNetworkReply* reply = nam_->post(req, data);
    QEventLoop loop;
    QTimer timer;
    timer.setSingleShot(true);
    QObject::connect(&timer, &QTimer::timeout, &loop, [&]() {
        if (reply && !reply->isFinished()) reply->abort();
    });
    QObject::connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    timer.start(timeoutMs);
    loop.exec();

    HttpResponse r;
    r.statusCode = reply->attribute(
        QNetworkRequest::HttpStatusCodeAttribute).toInt();
    r.ok = (reply->error() == QNetworkReply::NoError);
    r.body = reply->readAll();
    if (!r.ok) r.errorString = reply->errorString();
    reply->deleteLater();
    return r;
}

} // namespace netclient
