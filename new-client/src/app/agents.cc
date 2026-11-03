#include "app/app.h"

#include <QDateTime>
#include <QTableWidget>
#include <QTableWidgetItem>

void Application::refreshAgents() {
    this->m_net.getAgents([this](const ApiResult& r, const ListClientsResponse& list) {
        if(!this->m_usersTable)
            return;

        if(!r.ok) {
            this->logToConsole("failed to load agents: " + r.error);
            return;
        }

        for (const AgentInfo& agent : list.agents)
            upsertAgent(agent);
    });
}

void Application::upsertAgent(const AgentInfo& agent) {
    if (!m_usersTable)
        return;

    int row = 0;
    for (; row < m_usersTable->rowCount(); ++row) {
        const auto* item = m_usersTable->item(row, 0);
        if (item && item->data(Qt::UserRole).toInt() == agent.agentId)
            break;
    }
    if (row == m_usersTable->rowCount())
        m_usersTable->insertRow(row);

    const QString lastSeen = agent.lastSeen > 0
        ? QDateTime::fromSecsSinceEpoch(agent.lastSeen).toString("yyyy-MM-dd HH:mm:ss")
        : QString("-");
    auto* name = new QTableWidgetItem(agent.agentName);
    name->setData(Qt::UserRole, agent.agentId);
    m_usersTable->setItem(row, 0, name);
    m_usersTable->setItem(row, 1, new QTableWidgetItem(agent.username));
    m_usersTable->setItem(row, 2, new QTableWidgetItem(agent.hostname));
    m_usersTable->setItem(row, 3, new QTableWidgetItem(agent.domain));
    m_usersTable->setItem(row, 4, new QTableWidgetItem(agent.procPath));
    m_usersTable->setItem(row, 5, new QTableWidgetItem(QString::number(agent.pid)));
    m_usersTable->setItem(row, 6, new QTableWidgetItem(lastSeen));
}


