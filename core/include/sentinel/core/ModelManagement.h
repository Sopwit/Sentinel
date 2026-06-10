#pragma once

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>

namespace sentinel::core {

enum class ModelManagementAction : std::uint8_t {
    Pull,
    Delete,
    Install,
    Refresh,
    Import,
    Export,
};

QString modelManagementActionName(ModelManagementAction action);

enum class ModelManagementStatus : std::uint8_t {
    MetadataOnly,
    Unavailable,
    NotImplemented,
};

QString modelManagementStatusName(ModelManagementStatus status);

struct ModelManagementRequest {
    ModelManagementAction action = ModelManagementAction::Pull;
    QString modelName;
};

struct ModelManagementResult {
    ModelManagementStatus status = ModelManagementStatus::NotImplemented;
    ModelManagementAction action = ModelManagementAction::Pull;
    QString modelName;
    bool available = false;
    QString summary = QStringLiteral("Model management actions are not implemented.");
};

struct ModelRequirementSummary {
    QString modelName;
    QString approximateRam;
    QString approximateDisk;
    QString summary;
};

struct ModelRecommendation {
    QString modelName;
    QString useCase;
    QString qualityClass;
    QString latencyClass;
    ModelRequirementSummary requirements;
    QString summary;
};

QString modelRequirementSummary(const ModelRequirementSummary& requirement);
QString modelRecommendationSummary(const ModelRecommendation& recommendation);
QStringList modelRequirementSummaries(const QList<ModelRequirementSummary>& requirements);
QStringList modelRecommendationSummaries(const QList<ModelRecommendation>& recommendations);
QString safeModelManagementResultSummary(const ModelManagementResult& result);

class IModelManagementService {
public:
    virtual ~IModelManagementService() = default;

    virtual ModelManagementStatus status() const = 0;
    virtual QString statusSummary(int installedModelCount, const QString& selectedModel) const = 0;
    virtual QList<ModelRecommendation> recommendations() const = 0;
    virtual QList<ModelRequirementSummary> requirementSummaries() const = 0;
    virtual ModelManagementResult evaluate(const ModelManagementRequest& request) const = 0;
};

class StaticModelManagementService final : public IModelManagementService {
public:
    ModelManagementStatus status() const override;
    QString statusSummary(int installedModelCount, const QString& selectedModel) const override;
    QList<ModelRecommendation> recommendations() const override;
    QList<ModelRequirementSummary> requirementSummaries() const override;
    ModelManagementResult evaluate(const ModelManagementRequest& request) const override;
};

} // namespace sentinel::core
