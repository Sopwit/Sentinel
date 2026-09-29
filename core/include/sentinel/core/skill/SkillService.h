// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/skill/ISkillService.h"
#include <QMap>
#include <QNetworkAccessManager>
#include <QObject>

namespace sentinel::core {

class SkillService : public QObject, public ISkillService {
    Q_OBJECT
public:
    explicit SkillService(QObject* parent = nullptr);
    ~SkillService() override;

    // ISkillService interface
    int discoverSkills(const QString& searchDir) override;
    int loadSkillsFromUrl(const QString& indexUrl) override;

    bool addSkill(const Skill& skill) override;
    bool removeSkill(const QString& name) override;
    bool setEnabled(const QString& name, bool enabled);
    bool refreshSkill(const QString& name);
    void setAvailableRequirements(const QStringList& tools, const QStringList& providers,
                                  const QStringList& capabilities);
    void setAvailableTools(const QStringList& tools);
    void setActiveWorkspaceId(const QString& workspaceId);
    void setWorkspacePreferences(const QJsonObject& preferences);
    QList<Skill> skills() const override;
    std::optional<Skill> findSkill(const QString& name) const override;

    QString getSkillContent(const QString& name) const override;
    QString getSkillContentWithFiles(const QString& name, const QString& baseDir) const override;

    void registerEmbeddedSkill(const Skill& skill) override;

signals:
    void skillAdded(const QString& name);
    void skillRemoved(const QString& name);
    void skillUpdated(const QString& name);

private:
    // File-based discovery
    Skill parseMarkdownSkill(const QString& filePath) const;
    QString parseFrontmatter(const QString& content, QString& body) const;

    // URL-based discovery
    void fetchIndexJson(const QString& indexUrl);
    void downloadSkillFile(const QString& url, const QString& name, const QString& indexUrl,
                           const QJsonObject& metadata);

    QMap<QString, Skill> m_skills;
    QMap<QString, bool> m_enabledPreferences;
    QString m_preferencePath;
    QString preferenceKey(const Skill& skill) const;
    void loadPreferences();
    bool savePreferences() const;
    void applyPreference(Skill& skill) const;
    QStringList m_availableTools;
    QStringList m_availableProviders;
    QStringList m_availableCapabilities;
    QString m_activeWorkspaceId;
    QJsonObject m_workspacePreferences;
    void updateRequirements(Skill& skill) const;
    QNetworkAccessManager m_networkManager;
};

} // namespace sentinel::core
