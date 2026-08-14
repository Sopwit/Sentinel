// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/chat/IQuestionService.h"
#include <QObject>
#include <QMap>

namespace sentinel::core {

class QuestionService : public QObject, public IQuestionService {
    Q_OBJECT
public:
    explicit QuestionService(QObject* parent = nullptr);
    ~QuestionService() override;

    QString ask(const QuestionRequest& request, QuestionCallback callback) override;
    void respond(const QString& questionId, const QuestionResponse& response) override;
    void cancel(const QString& questionId) override;
    bool isPending(const QString& questionId) const override;

signals:
    void questionAsked(const QString& questionId, const QString& question);
    void questionAnswered(const QString& questionId);

private:
    QString generateId() const;

    struct PendingQuestion {
        QuestionRequest request;
        QuestionCallback callback;
    };

    QMap<QString, PendingQuestion> m_pending;
};

} // namespace sentinel::core
