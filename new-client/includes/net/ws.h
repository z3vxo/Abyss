#pragma once

#include <QJsonObject>
#include <QObject>
#include <QString>
#include <QUrl>
#include <QWebSocket>

class WsClient : public QObject {
    Q_OBJECT

public:
    explicit WsClient(QObject* parent = nullptr);
    void connectToServer(const QUrl& baseUrl, const QString& accessToken);
    void disconnectFromServer();

signals:
    void connected();
    void disconnected();
    void errorOccurred(const QString& message);
    void agentCallbackReceived(int agentId, int size);
    void taskOutputReceived(int agentId, int taskId, const QString& output);
    void newAgentReceived(const QJsonObject& agent);

private:
    void handleMessage(const QString& message);
    QWebSocket m_socket;
};
