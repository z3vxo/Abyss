#pragma once

#include <QObject>
#include <QUrl>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QVector>
#include <cstdint>
#include <functional>
#include <qtmetamacros.h>
#include <qurl.h>

struct ApiResult {
    bool ok = false;
    int status = 0;
    QString error;
    QJsonObject body;
};

struct LoginResponse {
    QString accessToken;
    QString refreshToken;
};

struct AgentInfo {
    int32_t agentId   = 0;
    QString agentName;
    QString username;
    QString hostname;
    QString domain;
    QString procPath;
    QString winVer;
    int     pid       = 0;
    int     tid       = 0;
    int     ppid      = 0;
    QString arch;
    QString isAdmin;
    int     lastSeen  = 0;
    int     createdAt = 0;
};

struct ListClientsResponse {
    int total = 0;
    QVector<AgentInfo> agents;
};


class NetworkService : public QObject {
    Q_OBJECT
public:
    explicit NetworkService(QObject* parent = nullptr);

    void setEndpoint(const QString& domain, quint16 port);
    void login(const QString& username, const QString& password,
               std::function<void(const ApiResult&, const LoginResponse&)> cb);
    void getAgents(std::function<void(const ApiResult&, const ListClientsResponse&)> cb);

private:
    void post(const QString& path, const QJsonObject& body,
              std::function<void(const ApiResult&)> cb);
    void get(const QString& path, std::function<void(const ApiResult&)>cb);

    QNetworkAccessManager m_nam;
    QUrl m_base;
    QString m_accessToken;
};
