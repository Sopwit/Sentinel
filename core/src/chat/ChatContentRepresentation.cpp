// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/ChatContentRepresentation.h"

#include <QTextBlock>
#include <QTextDocument>
#include <QTextFragment>
#include <QTextFormat>
#include <QUrl>
#include <QVariantMap>

namespace sentinel::core {

QList<ChatContentPart> parseChatMarkdown(const QString& source) {
    QTextDocument document;
    document.setMarkdown(source);
    QList<ChatContentPart> parts;
    for (auto block = document.begin(); block.isValid(); block = block.next()) {
        const bool code = block.blockFormat().hasProperty(QTextFormat::BlockCodeLanguage);
        const auto language = code
            ? block.blockFormat().property(QTextFormat::BlockCodeLanguage).toString()
            : QString{};
        for (auto fragment = block.begin(); !fragment.atEnd(); ++fragment) {
            const auto item = fragment.fragment();
            if (!item.isValid()) continue;
            ChatContentPart part;
            part.kind = code ? ChatContentPartKind::CodeBlock
                            : item.charFormat().isAnchor() ? ChatContentPartKind::Link
                                                           : ChatContentPartKind::Text;
            part.text = item.text();
            part.language = language;
            if (part.kind == ChatContentPartKind::Link) {
                part.url = item.charFormat().anchorHref();
                const QUrl url(part.url);
                if (!url.isValid() || url.host().isEmpty() ||
                    (url.scheme() != QLatin1String("https") &&
                     url.scheme() != QLatin1String("http"))) {
                    part.kind = ChatContentPartKind::Text;
                    part.url.clear();
                }
            }
            if (!parts.isEmpty() && parts.last().kind == part.kind &&
                parts.last().language == part.language && parts.last().url == part.url)
                parts.last().text += part.text;
            else parts.append(part);
        }
        if (block.next().isValid()) {
            if (!parts.isEmpty()) parts.last().text += QLatin1Char('\n');
            else parts.append({ChatContentPartKind::Text, QStringLiteral("\n"), {}, {}});
        }
    }
    return parts;
}

QVariantList chatContentPartsVariant(const QString& source) {
    QVariantList result;
    for (const auto& part : parseChatMarkdown(source)) {
        QVariantMap item;
        item.insert(QStringLiteral("kind"),
                    part.kind == ChatContentPartKind::CodeBlock ? QStringLiteral("code")
                    : part.kind == ChatContentPartKind::Link ? QStringLiteral("link")
                                                             : QStringLiteral("text"));
        item.insert(QStringLiteral("text"), part.text);
        item.insert(QStringLiteral("language"), part.language);
        item.insert(QStringLiteral("url"), part.url);
        result.append(item);
    }
    return result;
}

} // namespace sentinel::core
