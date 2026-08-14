// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/snapshot/IDiffViewerService.h"
#include <QObject>

namespace sentinel::core {

class DiffViewerService : public QObject, public IDiffViewerService {
    Q_OBJECT
public:
    explicit DiffViewerService(QObject* parent = nullptr);
    ~DiffViewerService() override;

    Diff computeDiff(const QString& oldPath, const QString& newPath) const override;
    Diff computeGitDiff(const QString& directory) const override;
    QString formatDiff(const Diff& diff, DiffViewMode mode = DiffViewMode::Unified) const override;
    void setViewMode(DiffViewMode mode) override;
    DiffViewMode viewMode() const override;

private:
    DiffViewMode m_viewMode{DiffViewMode::Unified};
};

} // namespace sentinel::core
