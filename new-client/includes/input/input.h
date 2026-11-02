#pragma once

#include <QObject>
#include <QString>
#include "../shell/message.h"

class NetworkService;

class Input : public QObject {
    Q_OBJECT

public:
    explicit Input(NetworkService& net, QObject* parent = nullptr);

    void parse(int agentId, const QString& text);

    void Help(int agentId);

signals:
    void outputReady(int agentId, ShellMessageType type, const QString& message);

private:
    NetworkService& m_net;
};
