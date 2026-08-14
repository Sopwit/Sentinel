// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>

namespace sentinel::core {

class IExternalEditorService {
public:
    virtual ~IExternalEditorService() = default;

    virtual QString detectEditor() const = 0;
    virtual QString editContent(const QString& content = {}) const = 0;
    virtual void setEditor(const QString& editor) = 0;
    virtual QString configuredEditor() const = 0;
    virtual bool isAvailable() const = 0;
};

} // namespace sentinel::core
