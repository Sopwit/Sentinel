// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QList>
#include <QString>

namespace sentinel::core {

struct DoomLoopIndicator {
    QString type;
    QString description;
    int confidence{0};
};

class DoomLoopDetector {
public:
    struct Config {
        int maxRepetitions{3};
        int windowSize{10};
        double similarityThreshold{0.8};
    };

    explicit DoomLoopDetector(const Config& config);

    DoomLoopIndicator detect(const QJsonObject& context) const;
    bool isStuck(const QString& sessionId) const;
    void recordAction(const QString& sessionId, const QString& action);
    void reset(const QString& sessionId);
    QList<DoomLoopIndicator> analyzeHistory(const QString& sessionId) const;

private:
    bool hasRepeatedPattern(const QStringList& actions) const;
    bool hasSelfModification(const QString& action) const;
    bool hasErrorLoop(const QStringList& errors) const;

    Config m_config;
    QMap<QString, QStringList> m_actionHistory;
    QMap<QString, QStringList> m_errorHistory;
};

} // namespace sentinel::core
