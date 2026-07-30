// SPDX-FileCopyrightText: 2026 Sopwit <support@sentinel.dev>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include <sentinel/desktop/viewmodels/RagViewModel.h>

namespace sentinel::desktop::viewmodels {

RagViewModel::RagViewModel(QObject* parent)
    : QObject(parent) {
}

void RagViewModel::setIsIndexing(bool indexing) {
    if (m_isIndexing != indexing) {
        m_isIndexing = indexing;
        Q_EMIT isIndexingChanged();
    }
}

void RagViewModel::setIndexedDocumentCount(int count) {
    if (m_indexedDocumentCount != count) {
        m_indexedDocumentCount = count;
        Q_EMIT indexedDocumentCountChanged();
    }
}

void RagViewModel::rebuildIndex() {
    setIsIndexing(true);
    // Asynchronous re-indexing placeholder
    setIsIndexing(false);
}

} // namespace sentinel::desktop::viewmodels
