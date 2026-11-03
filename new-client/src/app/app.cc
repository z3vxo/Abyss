#include "app/app.h"
#include "windows/login/login.h"

#include <QAction>
#include <QDebug>
#include <QFile>
#include <QHeaderView>
#include <QMessageBox>
#include <QStringList>
#include <QTabWidget>
#include <QTableWidget>
#include <QUiLoader>

Application::Application(int& argc, char** argv)
    : QApplication(argc, argv), m_input(m_net, this) {
	setApplicationName("Abyss");
	setApplicationVersion("1.0.0");
    connect(&m_input, &Input::outputReady, this, &Application::shellAppend);
    connect(&m_ws, &WsClient::agentCallbackReceived, this, [this](int agentId, int size) {
        shellAppend(agentId, ShellMessageType::Ok,
                    QString("Agent called back (%1 bytes)").arg(size));
    });
    connect(&m_ws, &WsClient::taskOutputReceived, this,
            [this](int agentId, int taskId, const QString& output) {
        shellAppend(agentId, ShellMessageType::Info,
                    QString("Output from task %1\n\n%2").arg(taskId).arg(output));
    });
    connect(&m_ws, &WsClient::newAgentReceived, this,
            [this](const QJsonObject& body) {
        if (!body.value("agent_id").isDouble() || !body.value("agent_name").isString() ||
            !body.value("username").isString() || !body.value("hostname").isString() ||
            !body.value("domain").isString() || !body.value("proc_path").isString() ||
            !body.value("pid").isDouble() || !body.value("last_seen").isDouble()) {
            logToConsole("WebSocket: Invalid new-agent event");
            return;
        }
        AgentInfo agent;
        agent.agentId = body.value("agent_id").toInt();
        agent.agentName = body.value("agent_name").toString();
        agent.username = body.value("username").toString();
        agent.hostname = body.value("hostname").toString();
        agent.domain = body.value("domain").toString();
        agent.procPath = body.value("proc_path").toString();
        agent.pid = body.value("pid").toInt();
        agent.lastSeen = body.value("last_seen").toInt();
        upsertAgent(agent);
    });
    connect(&m_ws, &WsClient::errorOccurred, this, [this](const QString& error) {
        logToConsole("WebSocket: " + error);
    });
    connect(&m_ws, &WsClient::disconnected, this, [this] {
        shellAppend(-1, ShellMessageType::Warn, "WebSocket disconnected");
    });
	this->initialise();
	this->showLogin();
}

Application::~Application() {
    disconnect(&m_ws, nullptr, this, nullptr);
    m_ws.disconnectFromServer();
    delete this->m_login;
    delete this->m_core;
}


void Application::initialise() {
    // Base rules first, then components and view-specific overrides.
    const QStringList stylesheets = {
        "core.qss", "chrome.qss", "tabs.qss", "tables.qss",
        "controls.qss", "scrollbars.qss", "shell.qss",
        "login.qss", "overlays.qss"
    };
    QString combined;
    for (const QString& name : stylesheets) {
        QFile stylesheet("./resc/qss/" + name);
        if (!stylesheet.open(QFile::ReadOnly | QFile::Text)) {
            qWarning() << "Cannot load stylesheet:" << stylesheet.fileName()
                       << stylesheet.errorString();
            continue;
        }
        combined += QString::fromUtf8(stylesheet.readAll());
        combined += '\n';
    }
    setStyleSheet(combined);
}


void Application::showLogin() {
    m_login = new LoginWindow(this);
    connect(m_login, &LoginWindow::loginRequest, this, &Application::handleLogin);
    m_login->show();
}

void Application::handleLogin(const QString& username, const QString& password,
                              const QString& domain, quint16 port) {
    m_login->setBusy(true);
    m_net.setEndpoint(domain, port);
    m_net.login(username, password, [this](const ApiResult& r, const LoginResponse& login) {
        if (r.ok) {
            this->showCore();
            if (m_core)
                m_ws.connectToServer(m_net.baseUrl(), login.accessToken);
            return;
        }

        m_login->setBusy(false);
        if (r.status == 0) {
            QMessageBox::warning(nullptr, "Connection failed",
                                 "Failed to connect: " + r.error);
        } else if (r.status == 401) {
            QMessageBox::warning(nullptr, "Login failed", "Invalid credentials");
        } else {
            QMessageBox::warning(nullptr, "Login failed", r.error);
        }
    });
}

void Application::showCore() {
    QFile file("./resc/ui/core.ui");
    if(!file.open(QFile::ReadOnly)) {
        QMessageBox::critical(nullptr, "error", "failed opening core.ui");
        return;
    }

    QUiLoader loader;
    m_core = loader.load(&file);
    file.close();

    if(!m_core) {
        QMessageBox::critical(nullptr, "error", loader.errorString());
        return;
    }

    if (auto* create = m_core->findChild<QAction*>("actionCreateListener")) {
        connect(create, &QAction::triggered, this, &Application::showCreateListener);
    }

    this->m_shellTabs = this->m_core->findChild<QTabWidget*>("shellTabs");
    if(this->m_shellTabs) {
        connect(this->m_shellTabs, &QTabWidget::tabCloseRequested,
                this, &Application::closeShellTab);
    }

    this->m_usersTable = this->m_core->findChild<QTableWidget*>("usersTable");
    if(this->m_usersTable) {
        this->m_usersTable->setColumnCount(7);
        this->m_usersTable->setHorizontalHeaderLabels({
            "Name", "Username", "Hostname", "Domain", "Process", "PID", "Last Seen"
        });

        QHeaderView* header = this->m_usersTable->horizontalHeader();
        header->setStretchLastSection(false);
        header->setSectionResizeMode(QHeaderView::Stretch);
        header->setSectionResizeMode(5, QHeaderView::ResizeToContents);
        header->setSectionResizeMode(6, QHeaderView::ResizeToContents);
        this->m_usersTable->verticalHeader()->setVisible(false);

        connect(this->m_usersTable, &QTableWidget::cellDoubleClicked,
                this, &Application::openAgentTab);
    }

    this->m_login->close();
    this->m_core->show();
    refreshAgents();
}

