#include "netclient/http_client.h"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QEventLoop>
#include <QTimer>
#include <QThread>

namespace netclient {

HttpClient::HttpClient(QObject* parent)
    : QObject(parent)
    , nam_(new QNetworkAccessManager(this))
{
}

HttpClient::~HttpClient() = default;

void HttpClient::setTimeout(int ms) { timeoutMs_ = ms; }

void HttpClient::doRequest(const QByteArray& method, const QUrl& url,
                           const QByteArray& data, const QByteArray& contentType,
                           const QMap<QString, QString>& headers,
                           HttpCallback callback) {
    QNetworkRequest req(url);
    for (auto it = headers.constBegin(); it != headers.constEnd(); ++it) {
        req.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
    }
    if (!contentType.isEmpty()) {
        req.setHeader(QNetworkRequest::ContentTypeHeader, contentType);
    }

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

    if (!reply) {
        HttpResponse r;
        r.errorString = "unsupported method: " + method;
        if (callback) callback(r);
        return;
    }

    // 超时看门狗
    auto* timer = new QTimer(reply);
    timer->setSingleShot(true);

    QObject::connect(timer, &QTimer::timeout, reply, [reply, callback]() {
        if (!reply->isFinished()) {
            reply->abort();
        }
    });
    timer->start(timeoutMs_);

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
        // 响应头
        const auto rawHeaders = reply->rawHeaderPairs();
        for (const auto& p : rawHeaders) {
            r.headers.insert(QString::fromUtf8(p.first),
                             QString::fromUtf8(p.second));
        }
        reply->deleteLater();
        if (callback) callback(r);
    });
}

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
