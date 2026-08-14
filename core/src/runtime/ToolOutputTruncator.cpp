// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ToolOutputTruncator.h"
#include <QFile>
#include <QDir>
#include <QTextStream>
#include <QDateTime>
#include <QStandardPaths>

namespace sentinel::core {

ToolOutputTruncator::ToolOutputTruncator(const TruncationConfig& config) : m_config(config) {
    if (m_config.outputDir.isEmpty()) {
        m_config.outputDir = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/sentinel/truncation";
    }
}

TruncationResult ToolOutputTruncator::truncate(const QByteArray& output, const QString& toolName) const {
    TruncationResult result;
    result.totalBytes = output.size();
    result.totalLines = output.count('\n');

    bool needsTruncation = (result.totalLines > m_config.maxLines) ||
                           (result.totalBytes > m_config.maxBytes);

    if (!needsTruncation) {
        result.preview = QString::fromUtf8(output);
        return result;
    }

    result.truncated = true;

    const QStringList lines = QString::fromUtf8(output).split('\n');
    const int requestedLines = qMax(2, m_config.previewLines);
    const int lineCount = qMin(lines.size(), requestedLines);
    const int headCount = qMax(1, lineCount / 2);
    const int tailCount = qMax(1, lineCount - headCount);
    const QStringList head = lines.mid(0, headCount);
    const QStringList tail = lines.mid(qMax(0, lines.size() - tailCount), tailCount);
    result.preview = head.join('\n');
    result.preview += QStringLiteral("\n\n... [%1 lines omitted, %2 bytes total]...\n\n")
                          .arg(qMax(0, lines.size() - head.size() - tail.size()))
                          .arg(result.totalBytes);
    result.preview += tail.join('\n');

    // Enforce the byte bound without splitting UTF-8 characters.
    if (result.preview.toUtf8().size() > m_config.maxBytes) {
        QByteArray previewBytes = result.preview.toUtf8().left(static_cast<int>(m_config.maxBytes));
        while (!previewBytes.isEmpty() && (static_cast<unsigned char>(previewBytes.back()) & 0xc0) == 0x80) {
            previewBytes.chop(1);
        }
        result.preview = QString::fromUtf8(previewBytes);
    }

    QDir().mkpath(m_config.outputDir);
    QString filename = QStringLiteral("%1_%2.txt")
                           .arg(toolName.isEmpty() ? "output" : toolName)
                           .arg(QDateTime::currentMSecsSinceEpoch());
    result.fullOutputPath = m_config.outputDir + "/" + filename;

    QFile file(result.fullOutputPath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(output);
    }

    return result;
}

QByteArray ToolOutputTruncator::readFullOutput(const QString& path) const {
    QFile file(path);
    if (file.open(QIODevice::ReadOnly)) {
        return file.readAll();
    }
    return {};
}

void ToolOutputTruncator::cleanupOldFiles() const {
    QDir dir(m_config.outputDir);
    if (!dir.exists()) return;

    QDateTime cutoff = QDateTime::currentDateTime().addDays(-m_config.retentionDays);
    for (const auto& entry : dir.entryInfoList(QDir::Files)) {
        if (entry.lastModified() < cutoff) {
            QFile::remove(entry.absoluteFilePath());
        }
    }
}

} // namespace sentinel::core
