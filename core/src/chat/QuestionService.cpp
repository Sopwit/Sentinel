// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/chat/QuestionService.h"
#include <QUuid>

namespace sentinel::core {

QuestionService::QuestionService(QObject* parent) : QObject(parent) {}
QuestionService::~QuestionService() = default;

QString QuestionService::ask(const QuestionRequest& request, QuestionCallback callback) {
    QuestionRequest req = request;
    if (req.questionId.isEmpty()) {
        req.questionId = generateId();
    }

    m_pending[req.questionId] = {req, callback};
    emit questionAsked(req.questionId, req.question);
    return req.questionId;
}

void QuestionService::respond(const QString& questionId, const QuestionResponse& response) {
    auto it = m_pending.find(questionId);
    if (it == m_pending.end())
        return;

    if (it->callback) {
        it->callback(response);
    }

    m_pending.erase(it);
    emit questionAnswered(questionId);
}

void QuestionService::cancel(const QString& questionId) {
    auto it = m_pending.find(questionId);
    if (it == m_pending.end())
        return;

    QuestionResponse resp;
    resp.questionId = questionId;
    resp.rejected = true;

    if (it->callback) {
        it->callback(resp);
    }

    m_pending.erase(it);
}

bool QuestionService::isPending(const QString& questionId) const {
    return m_pending.contains(questionId);
}

QString QuestionService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).left(8);
}

} // namespace sentinel::core
