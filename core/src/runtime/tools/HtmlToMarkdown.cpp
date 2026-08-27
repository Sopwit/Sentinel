// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/tools/HtmlToMarkdown.h"
#include <QRegularExpression>
#include <QTextStream>

namespace sentinel::core {

QString HtmlToMarkdown::convert(const QString& html) {
    if (html.isEmpty()) {
        return {};
    }

    QString result = html;

    // Remove script and style elements
    result.remove(QRegularExpression("<script[^>]*>[\\s\\S]*?</script>",
                                     QRegularExpression::CaseInsensitiveOption));
    result.remove(QRegularExpression("<style[^>]*>[\\s\\S]*?</style>",
                                     QRegularExpression::CaseInsensitiveOption));

    // Convert headers
    for (int i = 6; i >= 1; --i) {
        QString prefix(i, '#');
        result.replace(QRegularExpression(QStringLiteral("<h%1[^>]*>(.*?)</h%1>").arg(i),
                                          QRegularExpression::CaseInsensitiveOption |
                                              QRegularExpression::DotMatchesEverythingOption),
                       QStringLiteral("%1 %2\n\n").arg(prefix));
    }

    // Convert paragraphs
    result.replace(
        QRegularExpression("<p[^>]*>(.*?)</p>", QRegularExpression::CaseInsensitiveOption |
                                                    QRegularExpression::DotMatchesEverythingOption),
        QStringLiteral("%1\n\n"));

    // Convert line breaks
    result.replace(QRegularExpression("<br[^>]*>", QRegularExpression::CaseInsensitiveOption),
                   "\n");
    result.replace(QRegularExpression("<br[^>]*/?>", QRegularExpression::CaseInsensitiveOption),
                   "\n");

    // Convert bold
    result.replace(
        QRegularExpression("<b[^>]*>(.*?)</b>", QRegularExpression::CaseInsensitiveOption |
                                                    QRegularExpression::DotMatchesEverythingOption),
        QStringLiteral("**%1**"));
    result.replace(QRegularExpression("<strong[^>]*>(.*?)</strong>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("**%1**"));

    // Convert italic
    result.replace(
        QRegularExpression("<i[^>]*>(.*?)</i>", QRegularExpression::CaseInsensitiveOption |
                                                    QRegularExpression::DotMatchesEverythingOption),
        QStringLiteral("*%1*"));
    result.replace(QRegularExpression("<em[^>]*>(.*?)</em>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("*%1*"));

    // Convert links
    result.replace(QRegularExpression("<a[^>]*href=\"([^\"]+)\"[^>]*>(.*?)</a>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("[%2](%1)"));

    // Convert images
    result.replace(QRegularExpression("<img[^>]*src=\"([^\"]+)\"[^>]*/?>",
                                      QRegularExpression::CaseInsensitiveOption),
                   QStringLiteral("![Image](%1)"));

    // Convert lists
    result.replace(QRegularExpression("<li[^>]*>(.*?)</li>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("- %1\n"));

    // Convert code blocks
    result.replace(QRegularExpression("<pre[^>]*><code[^>]*>(.*?)</code></pre>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("```\n%1\n```\n\n"));

    // Convert inline code
    result.replace(QRegularExpression("<code[^>]*>(.*?)</code>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("`%1`"));

    // Convert blockquotes
    result.replace(QRegularExpression("<blockquote[^>]*>(.*?)</blockquote>",
                                      QRegularExpression::CaseInsensitiveOption |
                                          QRegularExpression::DotMatchesEverythingOption),
                   QStringLiteral("> %1\n\n"));

    // Convert tables (simplified)
    result.replace(QRegularExpression("<table[^>]*>", QRegularExpression::CaseInsensitiveOption),
                   "\n");
    result.replace(QRegularExpression("</table>", QRegularExpression::CaseInsensitiveOption), "\n");
    result.replace(QRegularExpression("<tr[^>]*>", QRegularExpression::CaseInsensitiveOption), "");
    result.replace(QRegularExpression("</tr>", QRegularExpression::CaseInsensitiveOption), "\n");
    result.replace(QRegularExpression("<td[^>]*>", QRegularExpression::CaseInsensitiveOption),
                   "| ");
    result.replace(QRegularExpression("</td>", QRegularExpression::CaseInsensitiveOption), " ");
    result.replace(QRegularExpression("<th[^>]*>", QRegularExpression::CaseInsensitiveOption),
                   "| ");
    result.replace(QRegularExpression("</th>", QRegularExpression::CaseInsensitiveOption), " ");

    // Remove remaining HTML tags
    result.remove(QRegularExpression("<[^>]*>"));

    // Decode HTML entities
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&quot;", "\"");
    result.replace("&#39;", "'");
    result.replace("&nbsp;", " ");

    // Clean up whitespace
    result.replace(QRegularExpression("\n{3,}"), "\n\n");
    result = result.trimmed();

    return result + "\n";
}

QString HtmlToMarkdown::toText(const QString& html) {
    if (html.isEmpty()) {
        return {};
    }

    QString result = html;

    // Remove script and style elements
    result.remove(QRegularExpression("<script[^>]*>[\\s\\S]*?</script>",
                                     QRegularExpression::CaseInsensitiveOption));
    result.remove(QRegularExpression("<style[^>]*>[\\s\\S]*?</style>",
                                     QRegularExpression::CaseInsensitiveOption));

    // Remove all HTML tags
    result.remove(QRegularExpression("<[^>]*>"));

    // Decode HTML entities
    result.replace("&amp;", "&");
    result.replace("&lt;", "<");
    result.replace("&gt;", ">");
    result.replace("&quot;", "\"");
    result.replace("&#39;", "'");
    result.replace("&nbsp;", " ");

    // Clean up whitespace
    result.replace(QRegularExpression("\\s+"), " ");
    result = result.trimmed();

    return result;
}

QString HtmlToMarkdown::extractTitle(const QString& html) {
    QRegularExpression titleRegex("<title[^>]*>(.*?)</title>",
                                  QRegularExpression::CaseInsensitiveOption |
                                      QRegularExpression::DotMatchesEverythingOption);
    QRegularExpressionMatch match = titleRegex.match(html);
    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }
    return {};
}

QString HtmlToMarkdown::extractLinks(const QString& html) {
    QString links;
    QRegularExpression linkRegex("<a[^>]*href=\"([^\"]+)\"[^>]*>(.*?)</a>",
                                 QRegularExpression::CaseInsensitiveOption |
                                     QRegularExpression::DotMatchesEverythingOption);

    QRegularExpressionMatchIterator it = linkRegex.globalMatch(html);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString url = match.captured(1);
        QString text = match.captured(2).trimmed();
        if (!text.isEmpty()) {
            links += QStringLiteral("[%1](%2)\n").arg(text, url);
        }
    }

    return links;
}

QString HtmlToMarkdown::extractImages(const QString& html) {
    QString images;
    QRegularExpression imgRegex("<img[^>]*src=\"([^\"]+)\"[^>]*/?>",
                                QRegularExpression::CaseInsensitiveOption);

    QRegularExpressionMatchIterator it = imgRegex.globalMatch(html);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString src = match.captured(1);
        images += QStringLiteral("![Image](%1)\n").arg(src);
    }

    return images;
}

} // namespace sentinel::core
