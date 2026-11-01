#pragma once

#include <QApplication>
#include <QString>
#include "../net/net.h"




class LoginWindow;
class QWidget;
class QLineEdit;
class QPlainTextEdit;
class QTableWidget;
class QTabWidget;

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

    void openAgentTab(int row, int column);
    void closeShellTab(int index);
    int  addShellTab(int agentId, const QString& title);
    int  findShellTab(int agentId) const;
    void logToConsole(const QString& text);

    NetworkService m_net;

    QTabWidget* m_shellTabs    = nullptr;
    QTableWidget* m_usersTable = nullptr;
    LoginWindow* m_login       = nullptr;
    QWidget* m_core            = nullptr;
};
