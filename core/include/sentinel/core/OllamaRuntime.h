#pragma once

#include <QList>
#include <QString>
#include <QUrl>

#include <cstdint>

namespace sentinel::core {

enum class OllamaConnectionStatus : std::uint8_t {
    Unavailable,
    Blocked,
    Connected,
};

inline QString ollamaConnectionStatusName(OllamaConnectionStatus status) {
    switch (status) {
    case OllamaConnectionStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case OllamaConnectionStatus::Blocked:
        return QStringLiteral("Blocked");
    case OllamaConnectionStatus::Connected:
        return QStringLiteral("Connected");
    }

    return QStringLiteral("Unavailable");
}

enum class OllamaHealthStatus : std::uint8_t {
    Unavailable,
    InvalidEndpoint,
    Healthy,
    Error,
};

inline QString ollamaHealthStatusName(OllamaHealthStatus status) {
    switch (status) {
    case OllamaHealthStatus::Unavailable:
        return QStringLiteral("Unavailable");
    case OllamaHealthStatus::InvalidEndpoint:
        return QStringLiteral("Invalid Endpoint");
    case OllamaHealthStatus::Healthy:
        return QStringLiteral("Healthy");
    case OllamaHealthStatus::Error:
        return QStringLiteral("Error");
    }

    return QStringLiteral("Unavailable");
}

struct OllamaEndpoint {
    QUrl url;
    bool valid = true;
    bool normalizedFromInvalid = false;

    static OllamaEndpoint defaultEndpoint();
    static OllamaEndpoint fromUserInput(const QString& endpoint);

    QString toString() const;
    bool isLoopbackHttp() const;
};

struct OllamaConfig {
    OllamaEndpoint endpoint = OllamaEndpoint::defaultEndpoint();
    bool healthCheckEnabled = true;
    bool modelDiscoveryEnabled = true;
    int healthCheckTimeoutMs = 750;
    int modelDiscoveryTimeoutMs = 1500;
    int generateTimeoutMs = 30000;
    int streamTimeoutMs = 30000;

    static OllamaConfig fromEndpoint(const QString& endpoint);
};

struct OllamaModelSummary {
    QString name;
    QString modifiedAt;
    qint64 sizeBytes = 0;
};

struct OllamaHealthCheckResult {
    OllamaConnectionStatus connectionStatus = OllamaConnectionStatus::Unavailable;
    OllamaHealthStatus healthStatus = OllamaHealthStatus::Unavailable;
    QString endpoint;
    QString summary = QStringLiteral("Ollama runtime client is unavailable.");
    int timeoutMs = 0;
};

QString ollamaModelSummary(const OllamaModelSummary& model);
QStringList ollamaModelSummaries(const QList<OllamaModelSummary>& models);
QString safeOllamaHealthSummary(const OllamaHealthCheckResult& result);

class IOllamaRuntimeClient {
public:
    virtual ~IOllamaRuntimeClient() = default;

    virtual OllamaConfig config() const = 0;
    virtual OllamaHealthCheckResult healthCheck() const = 0;
    virtual QList<OllamaModelSummary> installedModels() const = 0;
};

class NullOllamaRuntimeClient final : public IOllamaRuntimeClient {
public:
    explicit NullOllamaRuntimeClient(OllamaConfig config = OllamaConfig{});

    OllamaConfig config() const override;
    OllamaHealthCheckResult healthCheck() const override;
    QList<OllamaModelSummary> installedModels() const override;

private:
    OllamaConfig config_;
};

class OllamaHttpRuntimeClient final : public IOllamaRuntimeClient {
public:
    explicit OllamaHttpRuntimeClient(OllamaConfig config = OllamaConfig{}, int timeoutMs = 750);

    OllamaConfig config() const override;
    OllamaHealthCheckResult healthCheck() const override;
    QList<OllamaModelSummary> installedModels() const override;

private:
    QUrl endpointUrl(const QString& path) const;
    bool endpointAllowed() const;

    OllamaConfig config_;
    int timeoutMs_ = 750;
};

} // namespace sentinel::core
