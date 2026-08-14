// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QVariant>
#include <QMap>

namespace sentinel::core {

class LocalContext {
public:
    static LocalContext& instance();

    void set(const QString& key, const QVariant& value);
    QVariant get(const QString& key, const QVariant& defaultValue = {}) const;
    bool has(const QString& key) const;
    void remove(const QString& key);
    void clear();
    QMap<QString, QVariant> all() const;

private:
    LocalContext() = default;
    QMap<QString, QVariant> m_store;
};

} // namespace sentinel::core
