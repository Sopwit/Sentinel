// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/skill/ISkillService.h"
#include <QObject>
#include <QMap>
#include <QNetworkAccessManager>

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
    void downloadSkillFile(const QString& url, const QString& name);

    QMap<QString, Skill> m_skills;
    QNetworkAccessManager m_networkManager;
};

} // namespace sentinel::core
