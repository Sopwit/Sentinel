#pragma once

#include "sentinel/core/OllamaRuntime.h"

#include <QList>
#include <QString>
#include <QStringList>

#include <cstdint>
#include <functional>

namespace sentinel::core {

enum class LocalInferenceStatus : std::uint8_t {
    NotRequested,
    Busy,
    Refused,
    Blocked,
    InvalidRequest,
    ModelUnavailable,
    Succeeded,
    Error,
};

QString localInferenceStatusName(LocalInferenceStatus status);

enum class LocalInferenceError : std::uint8_t {
    None,
    ClientUnavailable,
    BlankPrompt,
    MissingModel,
    EndpointBlocked,
    ModelUnavailable,
    PermissionDenied,
    SafetyBlocked,
    RequestFailed,
    Timeout,
    InvalidResponse,
};

QString localInferenceErrorName(LocalInferenceError error);

struct LocalInferenceOptions {
    QString model;
    int timeoutMs = 30000;
    bool streamingRequested = false;
    bool cancellationRequested = false;
};

struct LocalInferenceRequest {
    QString id = QStringLiteral("local-inference-request-1");
    QString prompt;
    LocalInferenceOptions options;
};

struct LocalInferenceTrace {
    int sequence = 0;
    QString stage;
    QString status;
    QString summary;
};

struct LocalInferenceResponse {
    LocalInferenceStatus status = LocalInferenceStatus::NotRequested;
    LocalInferenceError error = LocalInferenceError::None;
    QString model;
    QString endpoint;
    QString text;
    QString summary = QStringLiteral("No local inference request yet.");
    qint64 latencyMs = -1;
    QList<LocalInferenceTrace> traces;
};

enum class LocalInferenceStreamStatus : std::uint8_t {
    Disabled,
    NotStarted,
    Refused,
    Streaming,
    Completed,
    Cancelled,
    Error,
};

QString localInferenceStreamStatusName(LocalInferenceStreamStatus status);

struct LocalInferenceStreamChunk {
    int sequence = 0;
    QString text;
    bool finalChunk = false;
    bool malformed = false;
    QString summary;
};

struct LocalInferenceStreamResult {
    LocalInferenceStreamStatus status = LocalInferenceStreamStatus::Disabled;
    LocalInferenceError error = LocalInferenceError::None;
    QString summary = QStringLiteral("Local inference streaming is disabled.");
    QString model;
    QString endpoint;
    QString accumulatedText;
    bool cancelled = false;
    int malformedChunkCount = 0;
    QList<LocalInferenceStreamChunk> chunks;
    QList<LocalInferenceTrace> traces;
};

QString localInferenceTraceSummary(const LocalInferenceTrace& trace);
QStringList localInferenceTraceSummaries(const QList<LocalInferenceTrace>& traces);
QString safeLocalInferenceSummary(const LocalInferenceResponse& response);
QString safeLocalInferenceResponseSummary(const LocalInferenceResponse& response);

class ILocalInferenceClient {
public:
    virtual ~ILocalInferenceClient() = default;

    virtual LocalInferenceResponse infer(const LocalInferenceRequest& request) = 0;
    virtual QString statusSummary() const = 0;
};

class ILocalInferenceStreamClient {
public:
    virtual ~ILocalInferenceStreamClient() = default;

    virtual LocalInferenceStreamResult
    startStream(const LocalInferenceRequest& request,
                const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) = 0;
    virtual QString statusSummary() const = 0;
    virtual bool isAvailable() const = 0;
};

class NullLocalInferenceStreamClient final : public ILocalInferenceStreamClient {
public:
    LocalInferenceStreamResult
    startStream(const LocalInferenceRequest& request,
                const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) override;
    QString statusSummary() const override;
    bool isAvailable() const override;
};

class NullLocalInferenceClient final : public ILocalInferenceClient {
public:
    LocalInferenceResponse infer(const LocalInferenceRequest& request) override;
    QString statusSummary() const override;
};

class OllamaLocalInferenceClient final : public ILocalInferenceClient {
public:
    explicit OllamaLocalInferenceClient(OllamaConfig config = OllamaConfig{},
                                        int timeoutMs = 30000);

    LocalInferenceResponse infer(const LocalInferenceRequest& request) override;
    QString statusSummary() const override;

private:
    QUrl endpointUrl(const QString& path) const;
    bool endpointAllowed() const;
    QList<OllamaModelSummary> installedModels(int timeoutMs) const;
    bool hasInstalledModel(const QString& model, int timeoutMs) const;

    OllamaConfig config_;
    int timeoutMs_ = 30000;
};

class OllamaLocalInferenceStreamClient final : public ILocalInferenceStreamClient {
public:
    explicit OllamaLocalInferenceStreamClient(OllamaConfig config = OllamaConfig{},
                                              int timeoutMs = 30000);

    LocalInferenceStreamResult
    startStream(const LocalInferenceRequest& request,
                const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) override;
    QString statusSummary() const override;
    bool isAvailable() const override;

private:
    QUrl endpointUrl(const QString& path) const;
    bool endpointAllowed() const;

    OllamaConfig config_;
    int timeoutMs_ = 30000;
};

} // namespace sentinel::core
