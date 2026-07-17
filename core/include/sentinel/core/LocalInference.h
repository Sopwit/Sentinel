#pragma once

#include "sentinel/core/OllamaRuntime.h"

#include <QList>
#include <QNetworkReply>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QThread>

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>

namespace sentinel::core {

struct ModelLookupReply {
    bool ok = false;
    bool timedOut = false;
    QList<OllamaModelSummary> models;
    QString error;
    QNetworkReply::NetworkError networkError = QNetworkReply::NoError;
};

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
    OllamaNotRunning,
    EndpointUnreachable,
    BlankPrompt,
    MissingModel,
    EndpointBlocked,
    ModelUnavailable,
    PermissionDenied,
    SafetyBlocked,
    BusyRequest,
    RequestFailed,
    Timeout,
    InvalidResponse,
    StreamInterrupted,
};

QString localInferenceErrorName(LocalInferenceError error);

struct LocalInferenceOptions {
    QString model;
    int timeoutMs = 30000;
    bool streamingRequested = false;
    bool cancellationRequested = false;
    std::shared_ptr<std::atomic_bool> cancellationToken;
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
    qint64 firstTokenLatencyMs = -1;
    int approximateOutputTokens = 0;
    double approximateTokensPerSecond = 0.0;
    QList<LocalInferenceTrace> traces;
    int timeoutMs = 0;
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
    qint64 latencyMs = -1;
    qint64 firstTokenLatencyMs = -1;
    int approximateOutputTokens = 0;
    double approximateTokensPerSecond = 0.0;
    QList<LocalInferenceStreamChunk> chunks;
    QList<LocalInferenceTrace> traces;
    int timeoutMs = 0;
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

using LocalInferenceFinishedCallback =
    std::function<void(const QString&, const LocalInferenceResponse&)>;
using LocalInferenceStreamChunkCallback =
    std::function<void(const QString&, const LocalInferenceStreamChunk&)>;
using LocalInferenceStreamFinishedCallback =
    std::function<void(const QString&, const LocalInferenceStreamResult&)>;

class ILocalInferenceWorker {
public:
    virtual ~ILocalInferenceWorker() = default;

    virtual bool startInference(const LocalInferenceRequest& request,
                                LocalInferenceFinishedCallback onFinished) = 0;
    virtual bool startStream(const LocalInferenceRequest& request,
                             LocalInferenceStreamChunkCallback onChunk,
                             LocalInferenceStreamFinishedCallback onFinished) = 0;
    virtual void cancel(const QString& requestId) = 0;
    virtual QString statusSummary() const = 0;
    virtual QString streamStatusSummary() const = 0;
    virtual bool streamingAvailable() const = 0;
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

class LocalInferenceWorker final : public ILocalInferenceWorker {
public:
    LocalInferenceWorker(std::unique_ptr<ILocalInferenceClient> inferenceClient,
                         std::unique_ptr<ILocalInferenceStreamClient> streamClient,
                         QObject* callbackContext, bool threadedInference, bool threadedStream);
    ~LocalInferenceWorker() override;

    bool startInference(const LocalInferenceRequest& request,
                        LocalInferenceFinishedCallback onFinished) override;
    bool startStream(const LocalInferenceRequest& request,
                     LocalInferenceStreamChunkCallback onChunk,
                     LocalInferenceStreamFinishedCallback onFinished) override;
    void cancel(const QString& requestId) override;
    QString statusSummary() const override;
    QString streamStatusSummary() const override;
    bool streamingAvailable() const override;

private:
    bool hasActiveThread() const;
    bool requestCancelled(const LocalInferenceRequest& request) const;

    std::unique_ptr<ILocalInferenceClient> inferenceClient_;
    std::unique_ptr<ILocalInferenceStreamClient> streamClient_;
    QObject* callbackContext_ = nullptr;
    QThread* activeThread_ = nullptr;
    std::shared_ptr<std::atomic_bool> activeCancellationToken_;
    bool threadedInference_ = false;
    bool threadedStream_ = false;
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
    ModelLookupReply installedModels(int timeoutMs) const;

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

struct LMStudioConfig {
    QUrl endpoint = QUrl(QStringLiteral("http://127.0.0.1:1234"));
    int timeoutMs = 30000;

    bool isLoopbackHttp() const {
        const auto host = endpoint.host().toLower();
        return (host == QLatin1String("127.0.0.1") || host == QLatin1String("localhost")) &&
               endpoint.scheme() == QLatin1String("http");
    }
    QString toString() const {
        return endpoint.toString();
    }

    static LMStudioConfig defaultConfig() {
        return {};
    }
};

class LMStudioLocalInferenceClient final : public ILocalInferenceClient {
public:
    explicit LMStudioLocalInferenceClient(LMStudioConfig config = LMStudioConfig{},
                                          int timeoutMs = 30000);

    LocalInferenceResponse infer(const LocalInferenceRequest& request) override;
    QString statusSummary() const override;

private:
    QUrl endpointUrl(const QString& path) const;
    bool endpointAllowed() const;

    LMStudioConfig config_;
    int timeoutMs_ = 30000;
};

class LMStudioLocalInferenceStreamClient final : public ILocalInferenceStreamClient {
public:
    explicit LMStudioLocalInferenceStreamClient(LMStudioConfig config = LMStudioConfig{},
                                                int timeoutMs = 30000);

    LocalInferenceStreamResult
    startStream(const LocalInferenceRequest& request,
                const std::function<void(const LocalInferenceStreamChunk&)>& onChunk) override;
    QString statusSummary() const override;
    bool isAvailable() const override;

private:
    QUrl endpointUrl(const QString& path) const;
    bool endpointAllowed() const;

    LMStudioConfig config_;
    int timeoutMs_ = 30000;
};

} // namespace sentinel::core
