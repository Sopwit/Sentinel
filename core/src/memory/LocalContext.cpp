// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/memory/LocalContext.h"

namespace sentinel::core {

LocalContext& LocalContext::instance() {
    static LocalContext ctx;
    return ctx;
}

void LocalContext::set(const QString& key, const QVariant& value) {
    m_store[key] = value;
}

QVariant LocalContext::get(const QString& key, const QVariant& defaultValue) const {
    return m_store.value(key, defaultValue);
}

bool LocalContext::has(const QString& key) const {
    return m_store.contains(key);
}

void LocalContext::remove(const QString& key) {
    m_store.remove(key);
}

void LocalContext::clear() {
    m_store.clear();
}

QMap<QString, QVariant> LocalContext::all() const {
    return m_store;
}

} // namespace sentinel::core
