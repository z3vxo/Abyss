#include "../../includes/app/app.h"
#include "../../includes/windows/login/login.h"
#include "../../includes/utils/utils.h"
// #include <qmessagebox.h>
// #include <qtpreprocessorsupport.h>

#include <QFile>
#include <QFileDialog>
#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialog>
#include <QDebug>
#include <QStringList>
#include <QUiLoader>
#include <QMessageBox>
#include <QDateTime>
#include <QHeaderView>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>


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
            [this](const QJsonObject&) { refreshAgents(); });
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

void Application::showCreateListener() {
    QFile file("./resc/ui/create_ll.ui");
    if (!file.open(QFile::ReadOnly)) {
        QMessageBox::warning(m_core, "Create listener", "Could not open the listener form.");
        return;
    }

    QUiLoader loader;
    auto* dialog = qobject_cast<QDialog*>(loader.load(&file, m_core));
    if (!dialog) {
        QMessageBox::warning(m_core, "Create listener", "Could not load the listener form.");
        return;
    }

    dialog->setAttribute(Qt::WA_DeleteOnClose);
    if (auto* close = dialog->findChild<QPushButton*>("closeButton"))
        connect(close, &QPushButton::clicked, dialog, &QDialog::reject);
    auto* protocol = dialog->findChild<QComboBox*>("protocolCombo");
    auto* selfSigned = dialog->findChild<QCheckBox*>("selfSignedCheckBox");
    auto* tlsSection = dialog->findChild<QWidget*>("tlsSection");
    if (protocol && selfSigned && tlsSection) {
        const auto updateProtocol = [selfSigned, tlsSection](const QString& value) {
            const bool https = value == "https";
            selfSigned->setVisible(https);
            tlsSection->setEnabled(https);
        };
        connect(protocol, &QComboBox::currentTextChanged, dialog, updateProtocol);
        updateProtocol(protocol->currentText());
    }
    const auto setupFilePicker = [dialog](const char* buttonName, const char* inputName,
                                           const QString& title) {
        auto* button = dialog->findChild<QPushButton*>(buttonName);
        auto* input = dialog->findChild<QLineEdit*>(inputName);
        if (!button || !input)
            return;
        connect(button, &QPushButton::clicked, dialog, [dialog, input, title] {
            const QString path = QFileDialog::getOpenFileName(dialog, title, input->text());
            if (!path.isEmpty())
                input->setText(path);
        });
    };
    setupFilePicker("browseTlsKeyButton", "tlsKeyEdit", "Select TLS key");
    setupFilePicker("browseTlsCertButton", "tlsCertEdit", "Select TLS certificate");
    const auto setupEntries = [dialog](const char* buttonName, const char* removeName, const char* layoutName,
                                      const char* scrollName, const QString& inputName,
                                      const QString& placeholder) {
        auto* button = dialog->findChild<QPushButton*>(buttonName);
        auto* remove = dialog->findChild<QPushButton*>(removeName);
        auto* layout = dialog->findChild<QVBoxLayout*>(layoutName);
        auto* scroll = dialog->findChild<QScrollArea*>(scrollName);
        if (!button || !remove || !layout || !scroll)
            return;
        connect(button, &QPushButton::clicked, dialog, [layout, scroll, remove, inputName, placeholder] {
            auto* input = new QLineEdit(scroll->widget());
            input->setObjectName(inputName);
            input->setPlaceholderText(placeholder);
            input->setMinimumHeight(38);
            layout->insertWidget(layout->count() - 1, input);
            remove->setEnabled(true);
            input->setFocus();
            QPointer<QLineEdit> guardedInput(input);
            QTimer::singleShot(0, scroll, [scroll, guardedInput] {
                if (guardedInput)
                    scroll->ensureWidgetVisible(guardedInput);
            });
        });
        connect(remove, &QPushButton::clicked, dialog, [layout, remove, scroll] {
            if (layout->count() <= 2)
                return;
            QLayoutItem* item = layout->takeAt(layout->count() - 2);
            delete item->widget();
            delete item;
            remove->setEnabled(layout->count() > 2);
            QPointer<QWidget> last = layout->itemAt(layout->count() - 2)->widget();
            QTimer::singleShot(0, scroll, [scroll, last] {
                if (last)
                    scroll->ensureWidgetVisible(last);
            });
        });
    };
    setupEntries("addPathButton", "removePathButton", "pathsLayout", "pathsScrollArea", "pathEdit", "/path");
    setupEntries("addCallbackButton", "removeCallbackButton", "callbacksLayout", "callbacksScrollArea", "callbackEdit", "example.com:443");
    dialog->open();
}

void Application::refreshAgents() {
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

    connect(input, &QLineEdit::returnPressed, view, [this, input, agentId] {
        const QString cmd = input->text();
        if(cmd.isEmpty())
            return;
        shellAppend(agentId, ShellMessageType::Command, cmd);
        input->clear();
        this->m_input.parse(agentId, cmd);
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

void Application::shellAppend(int agentId, ShellMessageType type, const QString& message) {
    if(!m_shellTabs)
        return;

    int idx = findShellTab(agentId);
    if(idx < 0 && agentId == -1)
        idx = addShellTab(-1, "console");

    // Output for a closed agent tab must not land in the active tab.
    if(idx < 0)
        return;

    QWidget* page = m_shellTabs->widget(idx);
    if(!page)
        return;

    auto* output = page->findChild<QPlainTextEdit*>();
    if(output) {
        QTextCursor cursor(output->document());
        cursor.movePosition(QTextCursor::End);
        if(!output->document()->isEmpty())
            cursor.insertBlock();

        QTextCharFormat normal;
        normal.setForeground(output->palette().color(QPalette::Text));

        if(type == ShellMessageType::Command) {
            const QString date = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss");
            cursor.insertText("[" + date + "]", utils::text::GetColoredDate());
            cursor.insertText(" ", normal);
            cursor.insertText("[Abyss]", utils::text::GetColoredAbyss());
            cursor.insertText(" $> " + message, normal);
        } else {
            QTextCharFormat icon = normal;
            icon.setFontWeight(QFont::Bold);
            QString symbol;
            switch(type) {
            case ShellMessageType::Info:
                symbol = "*";
                icon.setForeground(QColor("#62f5a1"));
                break;
            case ShellMessageType::Ok:
                symbol = "+";
                icon.setForeground(QColor("#64cfff"));
                break;
            case ShellMessageType::Warn:
                symbol = "*";
                icon.setForeground(QColor("#62f5a1"));
                break;
            case ShellMessageType::Error:
                symbol = "!";
                icon.setForeground(QColor("#ff7185"));
                break;
            case ShellMessageType::Command:
                break;
            }
            cursor.insertText("[" + symbol + "]", icon);
            cursor.insertText(" " + message, normal);
        }
        output->setTextCursor(cursor);
        output->ensureCursorVisible();
    }
}

void Application::logToConsole(const QString& text) {
    shellAppend(-1, ShellMessageType::Error, text);
}
