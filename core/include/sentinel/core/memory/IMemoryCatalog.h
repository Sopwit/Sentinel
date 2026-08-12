// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/memory/MemoryMetadata.h"

#include <QList>
#include <QtGlobal>

namespace sentinel::core {

class IMemoryCatalog {
public:
    Q_DISABLE_COPY(IMemoryCatalog)
    IMemoryCatalog() = default;
    virtual ~IMemoryCatalog() = default;

    virtual QList<MemoryShardDescriptor> shards() const = 0;
};

} // namespace sentinel::core
