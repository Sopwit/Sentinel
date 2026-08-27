// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

class HtmlToMarkdown {
public:
    // Convert HTML to Markdown format
    static QString convert(const QString& html);

    // Convert HTML to plain text
    static QString toText(const QString& html);

    // Extract specific elements
    static QString extractTitle(const QString& html);
    static QString extractLinks(const QString& html);
    static QString extractImages(const QString& html);

private:
    // Internal conversion helpers
    static QString processNode(const QString& html, int& pos);
    static QString escapeMarkdown(const QString& text);
    static QString convertTag(const QString& tag, const QString& attributes,
                              const QString& content);
};

} // namespace sentinel::core
