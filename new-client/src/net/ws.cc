#include "net/ws.h"

#include <QAbstractSocket>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QSslError>
#include <QUrlQuery>

WsClient::WsClient(QObject* parent) : QObject(parent) {
    connect(&m_socket, &QWebSocket::connected, this, &WsClient::connected);
    connect(&m_socket, &QWebSocket::disconnected, this, &WsClient::disconnected);
    connect(&m_socket, &QWebSocket::textMessageReceived, this, &WsClient::handleMessage);
    connect(&m_socket, &QWebSocket::errorOccurred, this, [this](QAbstractSocket::SocketError) {
        emit errorOccurred(m_socket.errorString());
    });
    connect(&m_socket, &QWebSocket::sslErrors, this, [this](const QList<QSslError>& errors) {
        m_socket.ignoreSslErrors(errors);
    });
}

void WsClient::connectToServer(const QUrl& baseUrl, const QString& accessToken) {
    QUrl url(baseUrl);
    if (url.scheme() == "https")
        url.setScheme("wss");
    else if (url.scheme() == "http")
        url.setScheme("ws");
    else {
        emit errorOccurred("Unsupported backend URL scheme");
        return;
    }
    if (accessToken.isEmpty()) {
        emit errorOccurred("Cannot connect WebSocket without an access token");
        return;
    }

    url.setPath("/rest/ws/connect");
    QUrlQuery query;
    query.addQueryItem("token", accessToken);
    url.setQuery(query);
    m_socket.open(url);
}

void WsClient::disconnectFromServer() {
    m_socket.close();
}

void WsClient::handleMessage(const QString& message) {
    QJsonParseError error;
    const QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError || !document.isObject()) {
        emit errorOccurred("Received invalid WebSocket JSON");
        return;
    }
    const QJsonObject event = document.object();
    const QString eventName = event.value("event").toString();
    if (eventName != "task-output" && eventName != "new-agent")
        return;
    if (!event.value("body").isObject()) {
        emit errorOccurred("WebSocket event has an invalid body");
        return;
    }
    const QJsonObject body = event.value("body").toObject();
    if (eventName == "new-agent") {
        emit newAgentReceived(body);
        return;
    }
    if (!body.value("agent_id").isDouble() || !body.value("total_size").isDouble() ||
        !body.value("output").isArray()) {
        emit errorOccurred("Invalid task-output event");
        return;
    }
    const int agentId = body.value("agent_id").toInt();
    emit agentCallbackReceived(agentId, body.value("total_size").toInt());
    for (const QJsonValue& value : body.value("output").toArray()) {
        if (!value.isObject()) {
            emit errorOccurred("Invalid task-output entry");
            continue;
        }
        const QJsonObject entry = value.toObject();
        if (!entry.value("task_id").isDouble() || !entry.value("output").isString()) {
            emit errorOccurred("Invalid task-output entry");
            continue;
        }
        emit taskOutputReceived(agentId, entry.value("task_id").toInt(),
                                entry.value("output").toString());
    }
}
