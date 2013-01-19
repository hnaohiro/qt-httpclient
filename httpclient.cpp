#include "cookieshandler.h"

#include <QApplication>
#include <QEventLoop>
#include <QRegExp>
#include <QStringList>
#include <QtNetwork/QNetworkCookieJar>
#include <QtNetwork/QNetworkRequest>

HttpClient::HttpClient(QObject *parent) : QObject(parent)
{
    m_manager = new QNetworkAccessManager(this);
    m_manager->setCookieJar(new QNetworkCookieJar(this));
    m_textCodec = NULL;
}

HttpClient::~HttpClient()
{
    delete m_manager;
}

QString HttpClient::post(const QString &url, const QByteArray &postData)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QNetworkReply *reply = m_manager->post(request, postData);
    waitForFinish(reply);

    // check redirect
    QByteArray location = reply->rawHeader(QString("Location").toAscii());
    if (location.size() > 0) {
        return get(QString(location));
    } else {
        QByteArray replyData = reply->readAll();
        if (m_textCodec != NULL) {
            return m_textCodec->toUnicode(replyData);
        } else {
            return QString(replyData);
        }
    }
}

QString HttpClient::post(const QString &url, QMap<QString, QString> &postData)
{
    QStringList data;
    foreach (QString key, postData.keys()) {
        data.append(key + "=" + postData[key]);
    }
    return post(url, data.join("&").toAscii());
}

QString HttpClient::get(const QString &url)
{
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    QNetworkReply* reply = m_manager->get(request);
    waitForFinish(reply);

    // check redirect
    QByteArray location = reply->rawHeader(QString("Location").toAscii());
    if (location.size() > 0) {
        return get(QString(location));
    } else {
        QByteArray replyData = reply->readAll();
        if (m_textCodec != NULL) {
            return m_textCodec->toUnicode(replyData);
        } else {
            return QString(replyData);
        }
    }
}

void HttpClient::setTextCodec(const QString &encoding)
{
    m_textCodec = QTextCodec::codecForName(encoding.toAscii());
}

QMap<QString, QString> HttpClient::allCookies(const QString &url)
{
    QMap<QString, QString> map;
    QList<QNetworkCookie> list = m_manager->cookieJar()->cookiesForUrl(QUrl(url));
    foreach (QNetworkCookie cookie, list) {
        map.insert(QString(cookie.name()), QString(cookie.value()));
    }
    return map;
}

QMap<QString, QString> HttpClient::defaultValuesFromName(const QString &html, const QString &name)
{
    QMap<QString, QString> map;

    // get form
    QRegExp rx("<form.*name=\"" + name + "\".*>.*</form>");
    rx.setMinimal(true);
    if (rx.indexIn(html, 0) == -1) {
        return map;
    }
    QString form = rx.cap(0);

    // get inputs
    rx.setPattern("<(input|select).*name=\"([^\"]+)\".*(value=\"([^\"]+)\")?.*/?>");
    int pos = 0;
    while ((pos = rx.indexIn(form, pos)) != -1) {
        map.insert(rx.cap(2), rx.cap(4));
        pos += rx.matchedLength();
     }

    return map;
}

void HttpClient::waitForFinish(QNetworkReply *reply)
{
    QEventLoop loop;
    connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
    loop.exec();
}
