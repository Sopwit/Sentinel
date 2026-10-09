// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include "sentinel/core/chat/ChatModeService.h"
#include <QStringList>
namespace sentinel::core {
struct ChatAttachmentLoadResult {
    QList<ChatAttachment> attachments;
    QString error;
    bool ok() const { return error.isEmpty(); }
};
// Explicit files only; bounded reads through the filesystem boundary.
ChatAttachmentLoadResult loadChatAttachments(const QStringList& paths);
}
