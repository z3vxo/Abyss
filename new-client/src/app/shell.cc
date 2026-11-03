#include "app/app.h"
#include "utils/utils.h"

#include <QDateTime>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QTabBar>
#include <QTabWidget>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QTextCursor>
#include <QTextDocument>
#include <QVBoxLayout>

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
                symbol = "-";
                icon.setForeground(QColor("#ffa657"));
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
