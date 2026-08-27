// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/skill/SkillService.h"
#include <QDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkReply>
#include <QRegularExpression>
#include <QTextStream>

namespace sentinel::core {

SkillService::SkillService(QObject* parent) : QObject(parent) {}

SkillService::~SkillService() = default;

int SkillService::discoverSkills(const QString& searchDir) {
    QDir dir(searchDir);
    if (!dir.exists()) {
        return 0;
    }

    int discoveredCount = 0;

    // Search for .md files in the directory
    QDirIterator it(searchDir, QStringList() << "*.md", QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        it.next();
        QString filePath = it.filePath();

        Skill skill = parseMarkdownSkill(filePath);
        if (skill.isValid()) {
            m_skills[skill.name] = skill;
            discoveredCount++;
            qDebug() << QStringLiteral("SkillService: Discovered skill '%1' from %2")
                            .arg(skill.name, filePath);
        }
    }

    return discoveredCount;
}

int SkillService::loadSkillsFromUrl(const QString& indexUrl) {
    fetchIndexJson(indexUrl);
    return 0; // Async, return count later
}

bool SkillService::addSkill(const Skill& skill) {
    if (!skill.isValid()) {
        return false;
    }

    m_skills[skill.name] = skill;
    emit skillAdded(skill.name);
    return true;
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
    if (!skill) {
        return {};
    }
    return skill->content;
}

QString SkillService::getSkillContentWithFiles(const QString& name, const QString& baseDir) const {
    auto skill = findSkill(name);
    if (!skill) {
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
        m_skills[skill.name] = skill;
        emit skillAdded(skill.name);
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

        QRegularExpressionMatch nameMatch = nameRegex.match(frontmatter);
        if (nameMatch.hasMatch()) {
            skill.name = nameMatch.captured(1).trimmed();
        }

        QRegularExpressionMatch descMatch = descRegex.match(frontmatter);
        if (descMatch.hasMatch()) {
            skill.description = descMatch.captured(1).trimmed();
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
    QNetworkRequest request;
    request.setUrl(QUrl(indexUrl));
    QNetworkReply* reply = m_networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, indexUrl]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << QStringLiteral("SkillService: Failed to fetch index from %1: %2")
                              .arg(indexUrl, reply->errorString());
            reply->deleteLater();
            return;
        }

        QByteArray data = reply->readAll();
        reply->deleteLater();

        QJsonParseError parseError;
        QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError) {
            qWarning() << QStringLiteral("SkillService: Failed to parse index JSON: %1")
                              .arg(parseError.errorString());
            return;
        }

        QJsonObject index = doc.object();
        QJsonArray skills = index["skills"].toArray();

        for (const auto& skillValue : skills) {
            QJsonObject skillObj = skillValue.toObject();
            QString name = skillObj["name"].toString();
            QString url = skillObj["url"].toString();

            if (!name.isEmpty() && !url.isEmpty()) {
                downloadSkillFile(url, name);
            }
        }
    });
}

void SkillService::downloadSkillFile(const QString& url, const QString& name) {
    QNetworkRequest request;
    request.setUrl(QUrl(url));
    QNetworkReply* reply = m_networkManager.get(request);

    connect(reply, &QNetworkReply::finished, this, [this, reply, name, url]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << QStringLiteral("SkillService: Failed to download skill '%1': %2")
                              .arg(name, reply->errorString());
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

        if (skill.isValid()) {
            m_skills[name] = skill;
            emit skillAdded(name);
            qDebug() << QStringLiteral("SkillService: Downloaded skill '%1'").arg(name);
        }
    });
}

} // namespace sentinel::core
