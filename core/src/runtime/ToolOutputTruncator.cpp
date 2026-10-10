// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/ToolOutputTruncator.h"
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QTextStream>

namespace sentinel::core {

ToolOutputTruncator::ToolOutputTruncator(const TruncationConfig& config) : m_config(config) {
    if (m_config.outputDir.isEmpty()) {
        m_config.outputDir =
            QStandardPaths::writableLocation(QStandardPaths::TempLocation) + "/sentinel/truncation";
    }
}

TruncationResult ToolOutputTruncator::truncate(const QByteArray& output,
                                               const QString& toolName) const {
    TruncationResult result;
    result.totalBytes = output.size();
    result.totalLines = output.count('\n');

    bool needsTruncation =
        (result.totalLines > m_config.maxLines) || (result.totalBytes > m_config.maxBytes);

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

    // Keep both ends within the byte budget, cutting only at UTF-8 boundaries.
    const QByteArray bytes = result.preview.toUtf8();
    const qsizetype limit = qMax<qsizetype>(0, m_config.maxBytes);
    if (bytes.size() > limit) {
        const QByteArray marker("\n... [middle omitted; byte limit] ...\n");
        if (limit <= marker.size()) {
            result.preview = QString::fromLatin1(marker.left(limit));
        } else {
            const qsizetype available = limit - marker.size();
            qsizetype headEnd = available / 2;
            qsizetype tailStart = bytes.size() - (available - headEnd);
            const auto continuation = [](char byte) {
                return (static_cast<unsigned char>(byte) & 0xc0) == 0x80;
            };
            while (headEnd > 0 && continuation(bytes.at(headEnd))) {
                --headEnd;
            }
            while (tailStart < bytes.size() && continuation(bytes.at(tailStart))) {
                ++tailStart;
            }
            result.preview = QString::fromUtf8(bytes.left(headEnd) + marker + bytes.mid(tailStart));
        }
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
    if (!dir.exists())
        return;

    QDateTime cutoff = QDateTime::currentDateTime().addDays(-m_config.retentionDays);
    for (const auto& entry : dir.entryInfoList(QDir::Files)) {
        if (entry.lastModified() < cutoff) {
            QFile::remove(entry.absoluteFilePath());
        }
    }
}

} // namespace sentinel::core
