#include "sentinel/core/ModelManagement.h"

namespace sentinel::core {

QString modelManagementActionName(ModelManagementAction action) {
    switch (action) {
    case ModelManagementAction::Pull:
        return QStringLiteral("Pull");
    case ModelManagementAction::Delete:
        return QStringLiteral("Delete");
    case ModelManagementAction::Install:
        return QStringLiteral("Install");
    case ModelManagementAction::Refresh:
        return QStringLiteral("Refresh");
    case ModelManagementAction::Import:
        return QStringLiteral("Import");
    case ModelManagementAction::Export:
        return QStringLiteral("Export");
    }

    return QStringLiteral("Pull");
}

QString modelManagementStatusName(ModelManagementStatus status) {
    switch (status) {
    case ModelManagementStatus::MetadataOnly:
        return QStringLiteral("Metadata Only");
    case ModelManagementStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case ModelManagementStatus::NotImplemented:
        return QStringLiteral("Not Implemented");
    }

    return QStringLiteral("Unavailable");
}

QString modelRequirementSummary(const ModelRequirementSummary& requirement) {
    return QStringLiteral("%1: approx RAM %2, approx disk %3. %4")
        .arg(requirement.modelName, requirement.approximateRam, requirement.approximateDisk,
             requirement.summary);
}

QString modelRecommendationSummary(const ModelRecommendation& recommendation) {
    return QStringLiteral("%1: %2, %3 quality, %4 latency. %5")
        .arg(recommendation.modelName, recommendation.useCase, recommendation.qualityClass,
             recommendation.latencyClass, modelRequirementSummary(recommendation.requirements));
}

QStringList modelRequirementSummaries(const QList<ModelRequirementSummary>& requirements) {
    QStringList summaries;
    for (const auto& requirement : requirements) {
        summaries.append(modelRequirementSummary(requirement));
    }
    return summaries;
}

QStringList modelRecommendationSummaries(const QList<ModelRecommendation>& recommendations) {
    QStringList summaries;
    for (const auto& recommendation : recommendations) {
        summaries.append(modelRecommendationSummary(recommendation));
    }
    return summaries;
}

QString safeModelManagementResultSummary(const ModelManagementResult& result) {
    return result.summary.isEmpty() ? QStringLiteral("%1 action is %2.")
                                          .arg(modelManagementActionName(result.action),
                                               modelManagementStatusName(result.status))
                                    : result.summary;
}

namespace {

QList<ModelRequirementSummary> staticRequirements() {
    return {
        ModelRequirementSummary{
            QStringLiteral("llama3.2:3b"),
            QStringLiteral("about 8 GB"),
            QStringLiteral("about 3 GB"),
            QStringLiteral("Approximate descriptive requirement for a lightweight general local "
                           "chat model."),
        },
        ModelRequirementSummary{
            QStringLiteral("mistral:7b"),
            QStringLiteral("about 16 GB"),
            QStringLiteral("about 5 GB"),
            QStringLiteral("Approximate descriptive requirement for balanced local reasoning and "
                           "writing."),
        },
        ModelRequirementSummary{
            QStringLiteral("qwen2.5-coder:7b"),
            QStringLiteral("about 16 GB"),
            QStringLiteral("about 5 GB"),
            QStringLiteral("Approximate descriptive requirement for local coding assistance."),
        },
    };
}

} // namespace

ModelManagementStatus StaticModelManagementService::status() const {
    return ModelManagementStatus::MetadataOnly;
}

QString StaticModelManagementService::statusSummary(int installedModelCount,
                                                    const QString& selectedModel) const {
    const auto selected =
        selectedModel.trimmed().isEmpty() ? QStringLiteral("none") : selectedModel.trimmed();
    return QStringLiteral("Model management readiness is metadata-only: %1 installed local "
                          "models reported, selected model %2, actions unavailable.")
        .arg(installedModelCount)
        .arg(selected);
}

QList<ModelRecommendation> StaticModelManagementService::recommendations() const {
    const auto requirements = staticRequirements();
    return {
        ModelRecommendation{
            requirements.at(0).modelName,
            QStringLiteral("General local chat"),
            QStringLiteral("Balanced"),
            QStringLiteral("Fast"),
            requirements.at(0),
            QStringLiteral("Deterministic recommendation metadata only; Sentinel will not pull or "
                           "install this model."),
        },
        ModelRecommendation{
            requirements.at(1).modelName,
            QStringLiteral("Reasoning and writing"),
            QStringLiteral("Balanced"),
            QStringLiteral("Moderate"),
            requirements.at(1),
            QStringLiteral("Deterministic recommendation metadata only; Sentinel will not pull or "
                           "install this model."),
        },
        ModelRecommendation{
            requirements.at(2).modelName,
            QStringLiteral("Coding assistance"),
            QStringLiteral("Balanced"),
            QStringLiteral("Moderate"),
            requirements.at(2),
            QStringLiteral("Deterministic recommendation metadata only; Sentinel will not pull or "
                           "install this model."),
        },
    };
}

QList<ModelRequirementSummary> StaticModelManagementService::requirementSummaries() const {
    return staticRequirements();
}

ModelManagementResult
StaticModelManagementService::evaluate(const ModelManagementRequest& request) const {
    const auto modelName = request.modelName.trimmed().isEmpty()
                               ? QStringLiteral("unspecified model")
                               : request.modelName.trimmed();
    return ModelManagementResult{
        ModelManagementStatus::NotImplemented,
        request.action,
        modelName,
        false,
        QStringLiteral("%1 for %2 is unavailable: model management is metadata-only and real "
                       "pull, delete, install, refresh, import, or export work is future scoped.")
            .arg(modelManagementActionName(request.action), modelName),
    };
}

} // namespace sentinel::core
