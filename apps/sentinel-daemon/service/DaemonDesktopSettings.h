// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/app/ApplicationController.h"
#include "sentinel/core/app/SettingsService.h"
#include <QJsonArray>
#include <QJsonObject>
#include <QVariant>
namespace sentinel::daemon {
class DaemonDesktopSettings final {
public:
    DaemonDesktopSettings(core::AppSettings& settings, core::ApplicationController& controller)
        : settings_(settings), controller_(controller) {}
    QJsonObject dispatch(const QString& name, const QJsonArray& args);
    QVariantList productSettings() const;
    QStringList productSettingsSections() const;
    QVariantMap setProductSetting(const QString& id, const QVariant& value);
    QVariantMap resetProductSetting(const QString& id);
    QVariantList resetProductSettingsSection(int section);
    QVariantMap clearWorkspaceSettingOverride(const QString& workspaceId, const QString& key);
    QVariantMap providerSettingsState(const QString& providerId) const;
    QVariantMap extensionSettingsState(const QString& extensionId) const;
    QVariantMap performExtensionSettingsAction(const QString& extensionId, int action);
    QVariantMap speechSettingsState() const;
    QVariantMap securitySettingsState() const;
    QVariantMap productPrivacyState() const;
    QVariantMap productRecoveryState() const;
    QVariantMap productBackupAvailability() const;
    QVariantMap exportProductBackup(const QStringList& domains) const;
    QVariantMap importProductBackup(const QByteArray& data, const QStringList& domains,
                                    bool replace);
    QVariantMap clearProductData(const QString& domain);
    QVariantMap runProductMaintenance();
    QVariantMap resolveInterruptedModelOperation(const QString& operationId);
    QVariantMap clearProductCredential(const QString& id);
    QVariantMap setProductProviderCredential(const QString& providerId, const QString& value);
    QVariantMap setProductPluginCredential(const QString& pluginId, const QString& credentialId,
                                           const QString& value);
    QVariantMap clearProductPluginCredential(const QString& pluginId, const QString& credentialId);

private:
    core::AppSettings& settings_;
    core::ApplicationController& controller_;
};
} // namespace sentinel::daemon
