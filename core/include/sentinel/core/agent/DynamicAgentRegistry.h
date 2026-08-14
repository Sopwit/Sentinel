// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/agent/IAgentRegistry.h"
#include <QObject>
#include <QMap>

namespace sentinel::core {

struct AgentDefinition {
    QString id;
    QString displayName;
    QString description;
    QString role; // Coordinator, Planner, Researcher, Builder, Guardian, Companion
    QString mode; // build, plan, explore
    QString model; // preferred model
    QString color;
    QStringList permissions;
    QStringList tags;
    QString prompt; // system prompt
    bool enabled{true};
    bool isCustom{false};
};

class DynamicAgentRegistry : public QObject, public IAgentRegistry {
    Q_OBJECT
public:
    explicit DynamicAgentRegistry(QObject* parent = nullptr);
    ~DynamicAgentRegistry() override;

    // IAgentRegistry interface
    QList<AgentDescriptor> agents() const override;
    AgentDescriptor agentById(const QString& id) const override;

    // Dynamic agent management
    bool addAgent(const AgentDefinition& agent);
    bool removeAgent(const QString& agentId);
    bool updateAgent(const AgentDefinition& agent);
    QList<AgentDefinition> agentDefinitions() const;
    std::optional<AgentDefinition> findAgentDefinition(const QString& agentId) const;

    // Discovery
    int discoverAgents(const QString& searchDir);
    int loadAgentsFromConfig(const QJsonArray& agentsArray);

    // Built-in agents
    void loadBuiltInAgents();

signals:
    void agentAdded(const QString& agentId);
    void agentRemoved(const QString& agentId);
    void agentUpdated(const QString& agentId);

private:
    AgentDescriptor definitionToDescriptor(const AgentDefinition& def) const;
    AgentDefinition parseMarkdownAgent(const QString& filePath) const;
    QString parseFrontmatter(const QString& content, QString& body) const;

    QMap<QString, AgentDefinition> m_agents;
};

} // namespace sentinel::core
