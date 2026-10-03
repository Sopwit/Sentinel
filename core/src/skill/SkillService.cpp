// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/skill/SkillService.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include <QDebug>
#include <QCryptographicHash>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QSet>
#include <QTextStream>

namespace sentinel::core {

SkillService::SkillService(QObject* parent) : QObject(parent) {
    m_preferencePath = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) +
        QStringLiteral("/skill_preferences.json");
    loadPreferences();
}

QString SkillService::preferenceKey(const Skill& skill) const {
    const auto identity = QString::number(static_cast<int>(skill.sourceType)) + QLatin1Char(':') +
        (skill.sourceType == SkillSourceType::Url ? skill.indexSource : skill.sourceLocation) +
        QLatin1Char(':') + skill.name;
    return QString::fromLatin1(QCryptographicHash::hash(identity.toUtf8(),
        QCryptographicHash::Sha256).toHex());
}

void SkillService::loadPreferences() {
    QFile file(m_preferencePath);
    if (!file.open(QIODevice::ReadOnly)) return;
    const auto document = QJsonDocument::fromJson(file.readAll());
    const auto entries = document.object().value(QStringLiteral("enabled")).toObject();
    for (auto it = entries.begin(); it != entries.end(); ++it)
        if (it.value().isBool()) m_enabledPreferences.insert(it.key(), it.value().toBool());
}

bool SkillService::savePreferences() const {
    if (!QDir().mkpath(QFileInfo(m_preferencePath).absolutePath())) return false;
    QJsonObject entries;
    for (auto it = m_enabledPreferences.cbegin(); it != m_enabledPreferences.cend(); ++it)
        entries.insert(it.key(), it.value());
    QSaveFile file(m_preferencePath);
    if (!file.open(QIODevice::WriteOnly)) return false;
    const auto bytes = QJsonDocument(QJsonObject{{QStringLiteral("enabled"), entries}})
                           .toJson(QJsonDocument::Compact);
    return file.write(bytes) == bytes.size() && file.commit();
}

void SkillService::applyPreference(Skill& skill) const {
    skill.enabledPreference = m_enabledPreferences.value(preferenceKey(skill),
                                                          skill.enabledPreference);
}

SkillService::~SkillService() = default;

int SkillService::discoverSkills(const QString& searchDir) {
    QDir dir(searchDir);
    if (!dir.exists()) {
        const auto root = QFileInfo(searchDir).absoluteFilePath() + QDir::separator();
        for (auto it = m_skills.begin(); it != m_skills.end();) {
            if (it->sourceType == SkillSourceType::Directory &&
                it->sourceLocation.startsWith(root)) {
                const auto name = it.key();
                it = m_skills.erase(it);
                emit skillRemoved(name);
            } else ++it;
        }
        return 0;
    }

    int discoveredCount = 0;
    QSet<QString> discoveredPaths;
    const QString root = QFileInfo(searchDir).canonicalFilePath();

    // Search for .md files in the directory
    QDirIterator it(searchDir, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString filePath = it.filePath();

        Skill skill = parseMarkdownSkill(filePath);
        if (skill.isValid()) {
            discoveredPaths.insert(QFileInfo(filePath).canonicalFilePath());
            const auto previous = m_skills.value(skill.name);
            if (!previous.name.isEmpty() &&
                (previous.sourceType != SkillSourceType::Directory ||
                 previous.sourceLocation != skill.sourceLocation))
                continue;
            applyPreference(skill);
            updateRequirements(skill);
            m_skills[skill.name] = skill;
            if (previous.name.isEmpty())
                emit skillAdded(skill.name);
            else if (previous.content != skill.content || previous.description != skill.description)
                emit skillUpdated(skill.name);
            discoveredCount++;
            qDebug() << QStringLiteral("SkillService: Discovered skill '%1' from %2")
                            .arg(skill.name, filePath);
        }
    }

    for (auto skillIt = m_skills.begin(); skillIt != m_skills.end();) {
        const auto& skill = skillIt.value();
        const QString path = QFileInfo(skill.sourceLocation).canonicalFilePath();
        if (skill.sourceType == SkillSourceType::Directory &&
            (skill.sourceLocation.startsWith(root + QDir::separator()) || path.startsWith(root + QDir::separator())) &&
            !discoveredPaths.contains(path)) {
            const QString name = skillIt.key();
            skillIt = m_skills.erase(skillIt);
            emit skillRemoved(name);
        } else {
            ++skillIt;
        }
    }

    return discoveredCount;
}

int SkillService::loadSkillsFromUrl(const QString& indexUrl) {
    if (QUrl(indexUrl).scheme() != QLatin1String("https"))
        return 0;
    fetchIndexJson(indexUrl);
    return 0; // Async, return count later
}

bool SkillService::addSkill(const Skill& skill) {
    if (!skill.isValid() || m_skills.contains(skill.name)) {
        return false;
    }

    Skill next = skill;
    applyPreference(next);
    updateRequirements(next);
    m_skills[skill.name] = next;
    emit skillAdded(skill.name);
    return true;
}

bool SkillService::setEnabled(const QString& name, bool enabled) {
    auto it = m_skills.find(name);
    if (it == m_skills.end())
        return false;
    const bool previousEffectivePreference = it->enabledPreference;
    it->enabledPreference = enabled;
    const auto key = preferenceKey(it.value());
    const bool hadPrevious = m_enabledPreferences.contains(key);
    const bool previous = m_enabledPreferences.value(key, true);
    m_enabledPreferences.insert(key, enabled);
    if (!savePreferences()) {
        if (hadPrevious) m_enabledPreferences.insert(key, previous);
        else m_enabledPreferences.remove(key);
        it->enabledPreference = previousEffectivePreference;
        return false;
    }
    updateRequirements(it.value());
    emit skillUpdated(name);
    return true;
}

bool SkillService::refreshSkill(const QString& name) {
    const auto skill = findSkill(name);
    if (!skill)
        return false;
    if (skill->sourceType == SkillSourceType::Directory) {
        discoverSkills(QFileInfo(skill->sourceLocation).absolutePath());
        return m_skills.contains(name);
    }
    if (skill->sourceType == SkillSourceType::Url && !skill->indexSource.isEmpty()) {
        fetchIndexJson(skill->indexSource);
        return true;
    }
    return false;
}

void SkillService::setAvailableRequirements(const QStringList& tools,
                                            const QStringList& providers,
                                            const QStringList& capabilities) {
    m_availableTools = tools;
    m_availableProviders = providers;
    m_availableCapabilities = capabilities;
    for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
        const auto previous = it->state;
        const auto missing = it->missingRequirements;
        updateRequirements(it.value());
        if (previous != it->state || missing != it->missingRequirements)
            emit skillUpdated(it.key());
    }
}

void SkillService::setAvailableTools(const QStringList& tools) {
    setAvailableRequirements(tools, m_availableProviders, m_availableCapabilities);
}

void SkillService::setActiveWorkspaceId(const QString& workspaceId) {
    m_activeWorkspaceId = workspaceId;
    for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
        const auto previous = it->state;
        const auto missing = it->missingRequirements;
        updateRequirements(it.value());
        if (previous != it->state || missing != it->missingRequirements)
            emit skillUpdated(it.key());
    }
}

void SkillService::setWorkspacePreferences(const QJsonObject& preferences) {
    if (m_workspacePreferences == preferences) return;
    m_workspacePreferences = preferences;
    for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
        const auto previous = it->state;
        updateRequirements(it.value());
        if (previous != it->state) emit skillUpdated(it.key());
    }
}

void SkillService::updateRequirements(Skill& skill) const {
    skill.missingRequirements.clear();
    if (!skill.enabledPreference ||
        (m_workspacePreferences.value(QStringLiteral("skill:") + skill.name).isBool() &&
         !m_workspacePreferences.value(QStringLiteral("skill:") + skill.name).toBool())) {
        skill.state = SkillState::Disabled;
        return;
    }
    if (skill.lastError == QLatin1String("Unsupported skill scope") ||
        (skill.scope == SkillScope::Workspace && skill.workspaceId.isEmpty())) {
        skill.state = SkillState::Incompatible;
        return;
    }
    if (skill.scope == SkillScope::Workspace && skill.workspaceId != m_activeWorkspaceId)
        skill.missingRequirements.append(QStringLiteral("workspace:%1").arg(skill.workspaceId));
    if (!skill.lastError.isEmpty()) {
        skill.state = SkillState::Failed;
        return;
    }
    for (const auto& tool : skill.requiredTools)
        if (!m_availableTools.contains(tool))
            skill.missingRequirements.append(QStringLiteral("tool:%1").arg(tool));
    for (const auto& provider : skill.requiredProviders)
        if (!m_availableProviders.contains(provider))
            skill.missingRequirements.append(QStringLiteral("provider:%1").arg(provider));
    for (const auto& capability : skill.requiredCapabilities)
        if (!m_availableCapabilities.contains(capability))
            skill.missingRequirements.append(QStringLiteral("capability:%1").arg(capability));
    skill.state = skill.missingRequirements.isEmpty() ? SkillState::Enabled
                                                     : SkillState::MissingRequirements;
}

bool SkillService::removeSkill(const QString& name) {
    if (!m_skills.contains(name)) {
        return false;
    }

    m_skills.remove(name);
    emit skillRemoved(name);
    return true;
}

QList<Skill> SkillService::skills() const {
    return m_skills.values();
}

std::optional<Skill> SkillService::findSkill(const QString& name) const {
    auto it = m_skills.find(name);
    if (it == m_skills.end()) {
        return std::nullopt;
    }
    return it.value();
}

QString SkillService::getSkillContent(const QString& name) const {
    auto skill = findSkill(name);
    if (!skill || skill->state != SkillState::Enabled) {
        return {};
    }
    return skill->content;
}

QString SkillService::getSkillContentWithFiles(const QString& name, const QString& baseDir) const {
    auto skill = findSkill(name);
    if (!skill || skill->state != SkillState::Enabled) {
        return {};
    }

    QString content = skill->content;

    // Find related files in the base directory
    if (!baseDir.isEmpty()) {
        QDir dir(baseDir);
        if (dir.exists()) {
            QStringList relatedFiles;
            QDirIterator it(baseDir, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                it.next();
                QString filePath = it.filePath();
                // Skip hidden files and common non-relevant files
                QFileInfo info(filePath);
                if (info.fileName().startsWith('.')) {
                    continue;
                }
                relatedFiles.append(filePath);
            }

            if (!relatedFiles.isEmpty()) {
                content += "\n\n## Related Files\n";
                for (const QString& file : relatedFiles) {
                    content += QStringLiteral("- %1\n").arg(file);
                }
            }
        }
    }

    return content;
}

void SkillService::registerEmbeddedSkill(const Skill& skill) {
    if (skill.isValid()) {
        const auto previous = m_skills.value(skill.name);
        if (!previous.name.isEmpty() && previous.sourceType != SkillSourceType::Embedded)
            return;
        Skill next = skill;
        applyPreference(next);
        updateRequirements(next);
        m_skills[skill.name] = next;
        if (previous.name.isEmpty()) emit skillAdded(skill.name);
        else emit skillUpdated(skill.name);
    }
}

Skill SkillService::parseMarkdownSkill(const QString& filePath) const {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return {};
    }

    QTextStream stream(&file);
    QString content = stream.readAll();
    file.close();

    Skill skill;
    skill.sourceType = SkillSourceType::Directory;
    skill.sourceLocation = filePath;

    // Parse frontmatter
    QString body;
    QString frontmatter = parseFrontmatter(content, body);

    if (!frontmatter.isEmpty()) {
        // Parse YAML-like frontmatter (simplified)
        QRegularExpression nameRegex("name:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression descRegex("description:\\s*(.+)",
                                     QRegularExpression::CaseInsensitiveOption);
        QRegularExpression versionRegex("(?:^|\\n)version:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression authorRegex("(?:^|\\n)author:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression toolsRegex("(?:^|\\n)required_tools:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression providersRegex("(?:^|\\n)required_providers:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression capabilitiesRegex("(?:^|\\n)required_capabilities:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression scopeRegex("(?:^|\\n)scope:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);
        QRegularExpression workspaceRegex("(?:^|\\n)workspace_id:\\s*(.+)", QRegularExpression::CaseInsensitiveOption);

        QRegularExpressionMatch nameMatch = nameRegex.match(frontmatter);
        if (nameMatch.hasMatch()) {
            skill.name = nameMatch.captured(1).trimmed();
        }

        QRegularExpressionMatch descMatch = descRegex.match(frontmatter);
        if (descMatch.hasMatch()) {
            skill.description = descMatch.captured(1).trimmed();
        }
        skill.version = versionRegex.match(frontmatter).captured(1).trimmed();
        skill.author = authorRegex.match(frontmatter).captured(1).trimmed();
        auto parseList = [&frontmatter](const QRegularExpression& regex) {
            const auto value = regex.match(frontmatter).captured(1).trimmed();
            QStringList items = value.split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (auto& item : items)
                item = item.trimmed();
            return items;
        };
        skill.requiredTools = parseList(toolsRegex);
        skill.requiredProviders = parseList(providersRegex);
        skill.requiredCapabilities = parseList(capabilitiesRegex);
        const auto scope = scopeRegex.match(frontmatter).captured(1).trimmed();
        if (scope == QLatin1String("workspace"))
            skill.scope = SkillScope::Workspace;
        else if (scope == QLatin1String("global-with-workspace-override"))
            skill.scope = SkillScope::GlobalWithWorkspaceOverride;
        else if (!scope.isEmpty() && scope != QLatin1String("global")) {
            skill.state = SkillState::Incompatible;
            skill.lastError = QStringLiteral("Unsupported skill scope");
        }
        skill.workspaceId = workspaceRegex.match(frontmatter).captured(1).trimmed();
        if (skill.scope == SkillScope::Workspace && skill.workspaceId.isEmpty()) {
            skill.state = SkillState::Incompatible;
            skill.lastError = QStringLiteral("Workspace skill requires workspace_id");
        }
    }

    // If no name from frontmatter, use filename
    if (skill.name.isEmpty()) {
        QFileInfo info(filePath);
        skill.name = info.completeBaseName();
    }

    skill.content = body.trimmed();

    return skill;
}

QString SkillService::parseFrontmatter(const QString& content, QString& body) const {
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

void SkillService::fetchIndexJson(const QString& indexUrl) {
    if (NetworkPolicyService::instance().check(QUrl(indexUrl)) != NetworkDecision::Allowed) {
        for (auto it = m_skills.begin(); it != m_skills.end(); ++it)
            if (it->indexSource == indexUrl && it->state != SkillState::Disabled) {
                it->state = SkillState::Failed;
                it->lastError = QStringLiteral("Offline");
                emit skillUpdated(it.key());
            }
        return;
    }
    QNetworkRequest request;
    request.setUrl(QUrl(indexUrl));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QIODevice::readyRead, reply, [reply] {
        if (reply->bytesAvailable() > 1024 * 1024) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, indexUrl]() {
        if (reply->error() != QNetworkReply::NoError) {
            for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
                if (it->indexSource == indexUrl) {
                    if (it->state != SkillState::Disabled)
                        it->state = SkillState::Failed;
                    it->lastError = QStringLiteral("Skill index unavailable");
                    emit skillUpdated(it.key());
                }
            }
            qWarning() << "SkillService: Skill index fetch failed";
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject() ||
            !doc.object().value(QStringLiteral("skills")).isArray()) {
            for (auto it = m_skills.begin(); it != m_skills.end(); ++it) {
                if (it->indexSource == indexUrl) {
                    if (it->state != SkillState::Disabled)
                        it->state = SkillState::Failed;
                    it->lastError = QStringLiteral("Invalid skill index");
                    emit skillUpdated(it.key());
                }
            }
            qWarning() << QStringLiteral("SkillService: Failed to parse index JSON: %1")
                              .arg(parseError.errorString());
            return;
        }

        QJsonObject index = doc.object();
        QJsonArray skills = index["skills"].toArray();
        QSet<QString> listed;

        for (const auto& skillValue : skills) {
            QJsonObject skillObj = skillValue.toObject();
            QString name = skillObj["name"].toString();
            QString url = skillObj["url"].toString();

            if (!name.isEmpty() && !url.isEmpty()) {
                listed.insert(name);
                auto previous = m_skills.find(name);
                if (previous != m_skills.end() && previous->indexSource == indexUrl &&
                    previous->sourceLocation != url) {
                    if (previous->state != SkillState::Disabled)
                        previous->state = SkillState::Failed;
                    previous->lastError = QStringLiteral("Skill source changed; refreshing");
                    emit skillUpdated(name);
                }
                downloadSkillFile(url, name, indexUrl, skillObj);
            }
        }
        for (auto it = m_skills.begin(); it != m_skills.end();) {
            if (it->indexSource == indexUrl && !listed.contains(it.key())) {
                const auto name = it.key();
                it = m_skills.erase(it);
                emit skillRemoved(name);
            } else ++it;
        }
    });
}

void SkillService::downloadSkillFile(const QString& url, const QString& name,
                                     const QString& indexUrl, const QJsonObject& metadata) {
    if (NetworkPolicyService::instance().check(QUrl(url)) != NetworkDecision::Allowed) {
        auto it = m_skills.find(name);
        if (it != m_skills.end()) {
            it->state = SkillState::Failed;
            it->lastError = QStringLiteral("Offline");
            emit skillUpdated(name);
        }
        return;
    }
    if (QUrl(url).scheme() != QLatin1String("https")) {
        auto it = m_skills.find(name);
        if (it != m_skills.end() && it->indexSource == indexUrl) {
            if (it->state != SkillState::Disabled)
                it->state = SkillState::Failed;
            it->lastError = QStringLiteral("Skill source URL must use HTTPS");
            emit skillUpdated(name);
        }
        return;
    }
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                         QNetworkRequest::ManualRedirectPolicy);
    request.setTransferTimeout(30000);
    QNetworkReply* reply = m_networkManager.get(request);
    connect(reply, &QIODevice::readyRead, reply, [reply] {
        if (reply->bytesAvailable() > 512 * 1024) reply->abort();
    });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, name, url, indexUrl, metadata]() {
        if (reply->error() != QNetworkReply::NoError) {
            auto it = m_skills.find(name);
            if (it != m_skills.end() && it->indexSource == indexUrl) {
                if (it->state != SkillState::Disabled)
                    it->state = SkillState::Failed;
                it->lastError = QStringLiteral("Skill download unavailable");
                emit skillUpdated(name);
            }
            qWarning() << "SkillService: Skill download failed" << name;
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        Skill skill;
        skill.name = name;
        skill.content = QString::fromUtf8(data);
        skill.sourceType = SkillSourceType::Url;
        skill.sourceLocation = url;
        skill.indexSource = indexUrl;
        skill.description = metadata.value(QStringLiteral("description")).toString().left(500);
        skill.version = metadata.value(QStringLiteral("version")).toString().left(80);
        skill.author = metadata.value(QStringLiteral("author")).toString().left(120);
        auto readList = [&metadata](const QString& key) {
            QStringList values;
            for (const auto& value : metadata.value(key).toArray())
                if (value.isString() && !value.toString().trimmed().isEmpty())
                    values.append(value.toString().trimmed().left(120));
            return values;
        };
        skill.requiredTools = readList(QStringLiteral("requiredTools"));
        skill.requiredProviders = readList(QStringLiteral("requiredProviders"));
        skill.requiredCapabilities = readList(QStringLiteral("requiredCapabilities"));
        const auto scope = metadata.value(QStringLiteral("scope")).toString();
        if (scope == QLatin1String("workspace"))
            skill.scope = SkillScope::Workspace;
        else if (scope == QLatin1String("global-with-workspace-override"))
            skill.scope = SkillScope::GlobalWithWorkspaceOverride;
        else if (!scope.isEmpty() && scope != QLatin1String("global")) {
            skill.state = SkillState::Incompatible;
            skill.lastError = QStringLiteral("Unsupported skill scope");
        }
        skill.workspaceId = metadata.value(QStringLiteral("workspaceId")).toString().left(120);

        if (skill.isValid()) {
            const auto previous = m_skills.value(name);
            if (!previous.name.isEmpty() &&
                (previous.sourceType != SkillSourceType::Url || previous.indexSource != indexUrl))
                return;
            applyPreference(skill);
            updateRequirements(skill);
            m_skills[name] = skill;
            if (previous.name.isEmpty()) emit skillAdded(name);
            else emit skillUpdated(name);
            qDebug() << QStringLiteral("SkillService: Downloaded skill '%1'").arg(name);
        }
    });
}

} // namespace sentinel::core
