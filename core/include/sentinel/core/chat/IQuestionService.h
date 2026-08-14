// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <functional>

namespace sentinel::core {

struct QuestionOption {
    QString label;
    QString description;
};

struct QuestionRequest {
    QString questionId;
    QString question;
    QStringList options;
    bool multiple{false};
    QString defaultOption;
};

struct QuestionResponse {
    QString questionId;
    QStringList selectedOptions;
    bool rejected{false};
};

using QuestionCallback = std::function<void(const QuestionResponse& response)>;

class IQuestionService {
public:
    virtual ~IQuestionService() = default;

    virtual QString ask(const QuestionRequest& request, QuestionCallback callback) = 0;
    virtual void respond(const QString& questionId, const QuestionResponse& response) = 0;
    virtual void cancel(const QString& questionId) = 0;
    virtual bool isPending(const QString& questionId) const = 0;
};

} // namespace sentinel::core
