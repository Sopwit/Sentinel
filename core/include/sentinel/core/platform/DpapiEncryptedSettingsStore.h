// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/interfaces/ISettingsStore.h"

#include <QString>
#include <memory>

namespace sentinel::core {

class DpapiEncryptedSettingsStore final : public ISettingsStore {
public:
    explicit DpapiEncryptedSettingsStore(std::unique_ptr<ISettingsStore> inner);

    QString value(const QString& key, const QString& defaultValue = QString()) const override;
    void setValue(QString key, QString value) override;

private:
    static bool isSecretKey(const QString& key);
    static QByteArray encrypt(const QString& plainText);
    static QString decrypt(const QByteArray& cipherData);

    std::unique_ptr<ISettingsStore> inner_;
};

} // namespace sentinel::core
