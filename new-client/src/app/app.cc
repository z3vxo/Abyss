#include "../../includes/app/app.h"
#include "../../includes/windows/login/login.h"
// #include <qmessagebox.h>
// #include <qtpreprocessorsupport.h>

#include <QFile>
#include <QUiLoader>
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QVBoxLayout>


Application::Application(int& argc, char** argv) : QApplication(argc, argv) {
	setApplicationName("Abyss");
	setApplicationVersion("1.0.0");
	this->initialise();
	this->showLogin();
}

Application::~Application() {
    delete this->m_login;
    delete this->m_core;
}


void Application::initialise() {
    QFile stylesheet("./resc/qss/core.qss");
    if(stylesheet.open(QFile::ReadOnly | QFile::Text)) {
        setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
        stylesheet.close();
    }
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
    m_net.login(username, password, [this](const ApiResult& r, const LoginResponse&) {
        if (r.ok) {
            this->showCore();
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

    this->m_net.getAgents([this](const ApiResult& r, const ListClientsResponse& list) {
        if(!this->m_usersTable)
            return;

        if(!r.ok) {
            this->logToConsole("failed to load agents: " + r.error);
            return;
        }

        this->m_usersTable->setRowCount(list.agents.size());
        for(int i = 0; i < list.agents.size(); ++i) {
            const AgentInfo& a = list.agents.at(i);
            const QString lastSeen = a.lastSeen > 0
                ? QDateTime::fromSecsSinceEpoch(a.lastSeen).toString("yyyy-MM-dd HH:mm:ss")
                : QString("-");

            auto* nameItem = new QTableWidgetItem(a.agentName);
            nameItem->setData(Qt::UserRole, a.agentId);
            this->m_usersTable->setItem(i, 0, nameItem);
            this->m_usersTable->setItem(i, 1, new QTableWidgetItem(a.username));
            this->m_usersTable->setItem(i, 2, new QTableWidgetItem(a.hostname));
            this->m_usersTable->setItem(i, 3, new QTableWidgetItem(a.domain));
            this->m_usersTable->setItem(i, 4, new QTableWidgetItem(a.procPath));
            this->m_usersTable->setItem(i, 5, new QTableWidgetItem(QString::number(a.pid)));
            this->m_usersTable->setItem(i, 6, new QTableWidgetItem(lastSeen));
        }
    });
}


int Application::findShellTab(int agentId) const {
    if(!m_shellTabs)
        return -1;

    for(int i = 0; i < m_shellTabs->count(); ++i) {
        if(m_shellTabs->tabBar()->tabData(i).toInt() == agentId)
            return i;
    }
    return -1;
}

int Application::addShellTab(int agentId, const QString& title) {
    if(!m_shellTabs)
        return -1;

    auto* view   = new QWidget(m_shellTabs);
    auto* layout = new QVBoxLayout(view);
    layout->setContentsMargins(0, 8, 0, 0);
    layout->setSpacing(8);

    auto* output = new QPlainTextEdit(view);
    output->setReadOnly(true);
    auto* input = new QLineEdit(view);
    input->setPlaceholderText("Type a command and press Enter…");

    layout->addWidget(output, 1);
    layout->addWidget(input);

    connect(input, &QLineEdit::returnPressed, view, [output, input, agentId] {
        const QString cmd = input->text();
        if(cmd.isEmpty())
            return;
        output->appendPlainText("$ " + cmd);
        input->clear();
        Q_UNUSED(agentId)
        // TODO: send cmd to agentId over the WebSocket
    });

    const int idx = m_shellTabs->addTab(view, title);
    m_shellTabs->tabBar()->setTabData(idx, agentId);
    return idx;
}

void Application::openAgentTab(int row, int column) {
    Q_UNUSED(column)

    if(!m_usersTable || !m_shellTabs)
        return;

    QTableWidgetItem* nameItem = m_usersTable->item(row, 0);
    if(!nameItem)
        return;

    const int agentId = nameItem->data(Qt::UserRole).toInt();
    int idx = findShellTab(agentId);
    if(idx < 0)
        idx = addShellTab(agentId, nameItem->text());
    m_shellTabs->setCurrentIndex(idx);
}

void Application::closeShellTab(int index) {
    if(!m_shellTabs || index < 0)
        return;

    QWidget* page = m_shellTabs->widget(index);
    m_shellTabs->removeTab(index);
    if(page)
        page->deleteLater();
}

void Application::logToConsole(const QString& text) {
    if(!m_shellTabs)
        return;

    int idx = findShellTab(-1);
    if(idx < 0)
        idx = addShellTab(-1, "console");

    auto* output = m_shellTabs->widget(idx)->findChild<QPlainTextEdit*>();
    if(output)
        output->appendPlainText(text);
}
