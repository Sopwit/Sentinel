// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/notification/IExternalEditorService.h"
#include <QObject>
#include <QProcess>

namespace sentinel::core {

class ExternalEditorService : public QObject, public IExternalEditorService {
    Q_OBJECT
public:
    explicit ExternalEditorService(QObject* parent = nullptr);
    ~ExternalEditorService() override;

    QString detectEditor() const override;
    QString editContent(const QString& content = {}) const override;
    void setEditor(const QString& editor) override;
    QString configuredEditor() const override;
    bool isAvailable() const override;

private:
    QString m_configuredEditor;
};

} // namespace sentinel::core
