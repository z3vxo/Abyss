#include "input/input.h"
#include "net/net.h"
#include <QPointer>
#include <QProcess>

Input::Input(NetworkService& net, QObject* parent)
    : QObject(parent), m_net(net) {
}


void Input::parse(int agentId, const QString& text) {

    QStringList parts = QProcess::splitCommand(text);
    if (parts.isEmpty())
        return;

    const QString command = parts.takeFirst();

    if (command == "help") {
        Help(agentId);
        return;
    }
    else if (command == "whoami") {
        QPointer<Input> self(this);
        m_net.NewTask(agentId, command, {}, [self, agentId, text](int result, const QString& error) {
            if (!self)
                return;
            if (result == 0)
                emit self->outputReady(agentId, ShellMessageType::Info,
                                       "Tasked agent to run " + text);
            else
                emit self->outputReady(agentId, ShellMessageType::Error, error);
        });
        return;
    }
    if (command == "ls") {
        if (parts.size() > 1) {
            emit outputReady(agentId, ShellMessageType::Error,
                             "Usage: ls [dir]");
            return;
        }

        const QString dir = parts.isEmpty() ? "." : parts.first();
        const QVector<Arg> args{
            {"string", QJsonValue(dir)}
        };

        QPointer<Input> self(this);
        m_net.NewTask(agentId, command, args,
            [self, agentId, text](int result, const QString& error) {
                if (!self)
                    return;

                if (result == 0) {
                    emit self->outputReady(
                        agentId, ShellMessageType::Info,
                        "Tasked agent to run " + text);
                } else {
                    emit self->outputReady(
                        agentId, ShellMessageType::Error, error);
                }
            });
        return;
    }


    emit outputReady(agentId, ShellMessageType::Error, "Unknown command: " + text);
}

void Input::Help(int agentId) {
    QString text = R"(Avaliable commands:

help:   show avaliable commands
whoami: list output of whoami /all without spawning cmd.exe)";
    emit outputReady(agentId, ShellMessageType::Info, text);
}
