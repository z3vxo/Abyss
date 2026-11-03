#pragma once

#include <QApplication>
#include <QString>
#include "../net/net.h"
#include "../net/ws.h"
#include "../input/input.h"

struct ListenerForm {
    QString name;
    QString bind;
    QString port;
    QString protocol;
    QStringList paths;
    QStringList callbacks;
    bool selfSigned = false;
    bool startNow = false;
    QString tlsKeyPath;
    QString tlsCertPath;
};


class LoginWindow;
class QWidget;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QTabWidget;
class QDialog;

class Application : public QApplication {
    Q_OBJECT;
public:
    Application(int& argc, char** argv);
    ~Application();
private:
    void initialise();
    void showLogin();
    void handleLogin(const QString& username, const QString& password, const QString& domain, quint16 port);
    void showCore();
    void showCreateListener();
    void CreateListener(ListenerForm form, QDialog* dialog);
    void refreshAgents();
    void upsertAgent(const AgentInfo& agent);

    void openAgentTab(int row, int column);
    void closeShellTab(int index);
    int  addShellTab(int agentId, const QString& title);
    int  findShellTab(int agentId) const;
    void shellAppend(int agentId, ShellMessageType type, const QString& message);
    void logToConsole(const QString& text);


    NetworkService m_net;
    WsClient m_ws;
    Input m_input;

    QTabWidget* m_shellTabs    = nullptr;
    QTableWidget* m_usersTable = nullptr;
    LoginWindow* m_login       = nullptr;
    QWidget* m_core            = nullptr;
};
