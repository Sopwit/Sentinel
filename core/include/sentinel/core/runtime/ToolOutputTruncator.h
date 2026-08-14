// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QByteArray>

namespace sentinel::core {

struct TruncationConfig {
    bool enabled{true};
    int maxLines{2000};
    qint64 maxBytes{51200};
    QString outputDir;
    int retentionDays{7};
    int previewLines{50};
};

struct TruncationResult {
    bool truncated{false};
    QString preview;
    QString fullOutputPath;
    int totalLines{0};
    qint64 totalBytes{0};
};

class ToolOutputTruncator {
public:
    explicit ToolOutputTruncator(const TruncationConfig& config = {});

    TruncationResult truncate(const QByteArray& output, const QString& toolName = {}) const;
    QByteArray readFullOutput(const QString& path) const;
    void cleanupOldFiles() const;

private:
    TruncationConfig m_config;
};

} // namespace sentinel::core
