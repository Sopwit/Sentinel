// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/DynamicAgentRegistry.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QTextStream>

namespace sentinel::core {

DynamicAgentRegistry::DynamicAgentRegistry(QObject* parent) : QObject(parent) {
    loadBuiltInAgents();
}

DynamicAgentRegistry::~DynamicAgentRegistry() = default;

QList<AgentDescriptor> DynamicAgentRegistry::agents() const {
    QList<AgentDescriptor> result;
    for (const auto& def : m_agents) {
        if (def.enabled) {
            result.append(definitionToDescriptor(def));
        }
    }
    return result;
}

AgentDescriptor DynamicAgentRegistry::agentById(const QString& id) const {
    auto it = m_agents.find(id);
    if (it == m_agents.end()) {
        return {};
    }
    return definitionToDescriptor(it.value());
}

bool DynamicAgentRegistry::addAgent(const AgentDefinition& agent) {
    if (agent.id.isEmpty() || m_agents.contains(agent.id)) {
        return false;
    }

    m_agents[agent.id] = agent;
    emit agentAdded(agent.id);
    return true;
}

bool DynamicAgentRegistry::removeAgent(const QString& agentId) {
    auto it = m_agents.find(agentId);
    if (it == m_agents.end()) {
        return false;
    }

    // Don't remove built-in agents
    if (!it->isCustom) {
        return false;
    }

    m_agents.erase(it);
    emit agentRemoved(agentId);
    return true;
}

bool DynamicAgentRegistry::updateAgent(const AgentDefinition& agent) {
    if (agent.id.isEmpty() || !m_agents.contains(agent.id)) {
        return false;
    }

    m_agents[agent.id] = agent;
    emit agentUpdated(agent.id);
    return true;
}

QList<AgentDefinition> DynamicAgentRegistry::agentDefinitions() const {
    return m_agents.values();
}

std::optional<AgentDefinition>
DynamicAgentRegistry::findAgentDefinition(const QString& agentId) const {
    auto it = m_agents.find(agentId);
    if (it == m_agents.end()) {
        return std::nullopt;
    }
    return it.value();
}

int DynamicAgentRegistry::discoverAgents(const QString& searchDir) {
    QDir dir(searchDir);
    if (!dir.exists()) {
        return 0;
    }

    int discoveredCount = 0;

    // Search for .md files
    QDirIterator it(searchDir, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString filePath = it.filePath();

        AgentDefinition agent = parseMarkdownAgent(filePath);
        if (!agent.id.isEmpty()) {
            agent.isCustom = true;
            m_agents[agent.id] = agent;
            discoveredCount++;
            qDebug() << QStringLiteral("DynamicAgentRegistry: Discovered agent '%1' from %2")
                            .arg(agent.id, filePath);
        }
    }

    return discoveredCount;
}

int DynamicAgentRegistry::loadAgentsFromConfig(const QJsonArray& agentsArray) {
    int loadedCount = 0;

    for (const auto& agentValue : agentsArray) {
        QJsonObject agentObj = agentValue.toObject();
        AgentDefinition agent;
        agent.id = agentObj["id"].toString();
        agent.displayName = agentObj["displayName"].toString();
        agent.description = agentObj["description"].toString();
        agent.role = agentObj["role"].toString();
        agent.mode = agentObj["mode"].toString();
        agent.model = agentObj["model"].toString();
        agent.color = agentObj["color"].toString();
        agent.prompt = agentObj["prompt"].toString();
        agent.isCustom = true;

        QJsonArray permissionsArray = agentObj["permissions"].toArray();
        for (const auto& perm : permissionsArray) {
            agent.permissions.append(perm.toString());
        }

        QJsonArray tagsArray = agentObj["tags"].toArray();
        for (const auto& tag : tagsArray) {
            agent.tags.append(tag.toString());
        }

        if (!agent.id.isEmpty()) {
            m_agents[agent.id] = agent;
            loadedCount++;
        }
    }

    return loadedCount;
}

void DynamicAgentRegistry::loadBuiltInAgents() {
    // Atlas - Coordinator
    AgentDefinition atlas;
    atlas.id = "atlas";
    atlas.displayName = "Atlas";
    atlas.description = "Coordination and routing agent";
    atlas.role = "Coordinator";
    atlas.mode = "build";
    atlas.color = "#4A90D9";
    atlas.tags = {"coordination", "routing"};
    m_agents[atlas.id] = atlas;

    // Orin - Planner
    AgentDefinition orin;
    orin.id = "orin";
    orin.displayName = "Orin";
    orin.description = "Multi-step plan structuring agent";
    orin.role = "Planner";
    orin.mode = "plan";
    orin.color = "#7B68EE";
    orin.tags = {"planning", "analysis"};
    m_agents[orin.id] = orin;

    // Vela - Researcher
    AgentDefinition vela;
    vela.id = "vela";
    vela.displayName = "Vela";
    vela.description = "Long-context summarization agent";
    vela.role = "Researcher";
    vela.mode = "explore";
    vela.color = "#20B2AA";
    vela.tags = {"research", "summarization"};
    m_agents[vela.id] = vela;

    // Kaze - Builder
    AgentDefinition kaze;
    kaze.id = "kaze";
    kaze.displayName = "Kaze";
    kaze.description = "Coding and tool planning agent";
    kaze.role = "Builder";
    kaze.mode = "build";
    kaze.color = "#FF6347";
    kaze.tags = {"coding", "development"};
    m_agents[kaze.id] = kaze;

    // Nyx - Guardian
    AgentDefinition nyx;
    nyx.id = "nyx";
    nyx.displayName = "Nyx";
    nyx.description = "Privacy and safety boundaries agent";
    nyx.role = "Guardian";
    nyx.mode = "plan";
    nyx.color = "#9370DB";
    nyx.tags = {"security", "privacy"};
    m_agents[nyx.id] = nyx;

    // Sol - Companion
    AgentDefinition sol;
    sol.id = "sol";
    sol.displayName = "Sol";
    sol.description = "Conversational flows agent";
    sol.role = "Companion";
    sol.mode = "build";
    sol.color = "#FFD700";
    sol.tags = {"conversation", "companion"};
    m_agents[sol.id] = sol;
}

AgentDescriptor DynamicAgentRegistry::definitionToDescriptor(const AgentDefinition& def) const {
    AgentDescriptor desc;
    desc.id = def.id;
    desc.displayName = def.displayName;
    desc.role = AgentRole::Coordinator; // Default, would need mapping
    desc.state = AgentState::Available;
    desc.priority = AgentPriority::Normal;

    // Map role string to enum
    if (def.role == "Planner")
        desc.role = AgentRole::Planner;
    else if (def.role == "Researcher")
        desc.role = AgentRole::Researcher;
    else if (def.role == "Builder")
        desc.role = AgentRole::Builder;
    else if (def.role == "Guardian")
        desc.role = AgentRole::Guardian;
    else if (def.role == "Companion")
        desc.role = AgentRole::Companion;

    return desc;
}

AgentDefinition DynamicAgentRegistry::parseMarkdownAgent(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    AgentDefinition agent;
    agent.isCustom = true;

    // Parse frontmatter
    QString body;
    QString frontmatter = parseFrontmatter(content, body);

    if (!frontmatter.isEmpty()) {
        QRegularExpression idRegex("id:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression nameRegex("name:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression descRegex("description:\\s*(.+)",
                                     QRegularExpression::CaseInsensitiveOption);
        QRegularExpression roleRegex("role:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression modeRegex("mode:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression modelRegex("model:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression colorRegex("color:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);

        QRegularExpressionMatch idMatch = idRegex.match(frontmatter);
        if (idMatch.hasMatch())
            agent.id = idMatch.captured(1).trimmed();

        QRegularExpressionMatch nameMatch = nameRegex.match(frontmatter);
        if (nameMatch.hasMatch())
            agent.displayName = nameMatch.captured(1).trimmed();

        QRegularExpressionMatch descMatch = descRegex.match(frontmatter);
        if (descMatch.hasMatch())
            agent.description = descMatch.captured(1).trimmed();

        QRegularExpressionMatch roleMatch = roleRegex.match(frontmatter);
        if (roleMatch.hasMatch())
            agent.role = roleMatch.captured(1).trimmed();

        QRegularExpressionMatch modeMatch = modeRegex.match(frontmatter);
        if (modeMatch.hasMatch())
            agent.mode = modeMatch.captured(1).trimmed();

        QRegularExpressionMatch modelMatch = modelRegex.match(frontmatter);
        if (modelMatch.hasMatch())
            agent.model = modelMatch.captured(1).trimmed();

        QRegularExpressionMatch colorMatch = colorRegex.match(frontmatter);
        if (colorMatch.hasMatch())
            agent.color = colorMatch.captured(1).trimmed();
    }

    // If no id from frontmatter, use filename
    if (agent.id.isEmpty()) {
        QFileInfo info(filePath);
        agent.id = info.completeBaseName();
    }

    agent.prompt = body.trimmed();
    return agent;
}

QString DynamicAgentRegistry::parseFrontmatter(const QString& content, QString& body) const {
    if (!content.startsWith("---")) {
        body = content;
        return {};
    }

    int endIndex = content.indexOf("---", 3);
    if (endIndex == -1) {
        body = content;
        return {};
    }

    QString frontmatter = content.mid(3, endIndex - 3).trimmed();
    body = content.mid(endIndex + 3).trimmed();

    return frontmatter;
}

} // namespace sentinel::core
