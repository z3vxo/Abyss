#include "input/input.h"
#include <QPointer>
#include <QProcess>
#include <qcontainerfwd.h>

Input::Input(NetworkService& net, QObject* parent)
    : QObject(parent), m_net(net) {
}


void Input::NewTask(int agentId, QString cmd, QVector<Arg> Args, QString text) {
    QPointer<Input> self(this);
    m_net.NewTask(agentId, cmd, Args,
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
}

void Input::parse(int agentId, const QString& text) {

    QStringList parts = QProcess::splitCommand(text);
    if (parts.isEmpty())
        return;

    const QString command = parts.takeFirst().toLower();

    if (command == "help") {
        Help(agentId);
        return;
    }
    else if (command == "whoami" || command == "pwd") {
        this->NewTask(agentId, command, {}, text);
        return;
    }
    if (command == "ls" || command == "cd" || command == "cat") {
        if (command == "cd" || command == "cat" && (parts.isEmpty() || parts.first().isEmpty())) {
            emit outputReady(agentId, ShellMessageType::Warn,
                             "must provide and argument");
            return;
        }
        if (parts.size() > 1) {
            emit outputReady(agentId, ShellMessageType::Warn,
                             command == "cd" ? "Usage: cd <dir>" : "Usage: ls [dir]");
            return;
        }

        const QString dir = parts.isEmpty() ? "." : parts.first();
        const QVector<Arg> args{
            {"string", QJsonValue(dir)}
        };
        this->NewTask(agentId, command, args, text);

        return;
    }


    emit outputReady(agentId, ShellMessageType::Error, "Unknown command: " + text);
}

void Input::Help(int agentId) {
    QString text = R"(Avaliable commands:

help:   show avaliable commands
whoami: list output of whoami /all without spawning cmd.exe
ls:     list files in a directory
cd:     change directory)";
    emit outputReady(agentId, ShellMessageType::Info, text);
}
