// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QJsonObject>

namespace sentinel::core {

class AutoTitleGenerator {
public:
    QString generateTitle(const QString& conversationContent) const;
    QString cleanTitle(const QString& title) const;
    int maxLength() const;

private:
    QString extractTopic(const QString& text) const;
    QString summarizeKeyAction(const QString& text) const;
    int m_maxLength{100};
};

} // namespace sentinel::core
