// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include <QString>
#include <QStringList>

namespace sentinel::core {
class AppSettings;
class ModelService;

enum class OnboardingStep { Welcome, ProcessingMode, Provider, Model, Speech, Privacy, Ready };

struct OnboardingSnapshot {
    int version = 1;
    OnboardingStep step = OnboardingStep::Welcome;
    QString processingMode = QStringLiteral("local");
    QStringList completedSteps;
    bool complete = false;
    bool resumed = false;
    QString providerId;
    QString modelId;
};

struct OnboardingActionResult {
    bool accepted = false;
    QString errorCode;
    OnboardingSnapshot state;
};

class OnboardingService final {
public:
    explicit OnboardingService(AppSettings& settings, ModelService* models = nullptr);
    OnboardingSnapshot snapshot() const;
    OnboardingActionResult chooseProcessingMode(const QString& mode);
    OnboardingActionResult chooseModel(const QString& providerId, const QString& modelId);
    OnboardingActionResult advance(bool skip = false);
    OnboardingActionResult back();
    OnboardingActionResult reopen();
    OnboardingActionResult finish();
    static QString stepId(OnboardingStep step);
    static QStringList stepIds();

private:
    void save(const OnboardingSnapshot& state);
    AppSettings& settings_;
    ModelService* models_;
};
} // namespace sentinel::core
