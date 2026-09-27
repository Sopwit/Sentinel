// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QList>
#include <QString>
#include <QVariantList>

namespace sentinel::core {

enum class ChatContentPartKind { Text, CodeBlock, Link };

struct ChatContentPart {
    ChatContentPartKind kind = ChatContentPartKind::Text;
    QString text;
    QString language;
    QString url;
};

QList<ChatContentPart> parseChatMarkdown(const QString& source);
QVariantList chatContentPartsVariant(const QString& source);

} // namespace sentinel::core
