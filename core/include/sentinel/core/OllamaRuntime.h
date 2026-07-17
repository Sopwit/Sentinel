#pragma once

#include <QList>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <cstdint>

class QNetworkAccessManager;

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
QList<OllamaModelSummary> fetchOpenAiCompatibleModels(const QUrl& url, int timeoutMs);

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

// ── OllamaModelPuller ─────────────────────────────────────────────────────────
// QObject that drives a real Ollama /api/pull request and emits granular
// progress/completion signals so QML can drive live progress bars.
// Safe to expose to QML via setContextProperty.
// ─────────────────────────────────────────────────────────────────────────────
class OllamaModelPuller : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool pulling READ pulling NOTIFY pullingChanged)
    Q_PROPERTY(QString activeModel READ activeModel NOTIFY activeModelChanged)
    Q_PROPERTY(qreal progress READ progress NOTIFY progressChanged)
    Q_PROPERTY(QString statusText READ statusText NOTIFY statusTextChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit OllamaModelPuller(QObject* parent = nullptr);

    bool pulling() const;
    QString activeModel() const;
    qreal progress() const;
    QString statusText() const;
    QString errorText() const;

    Q_INVOKABLE void pull(const QString& modelId);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void removeModel(const QString& modelId);

signals:
    void pullingChanged();
    void activeModelChanged();
    void progressChanged();
    void statusTextChanged();
    void errorTextChanged();
    void pullFinished(const QString& modelId, bool success);
    void removeFinished(const QString& modelId, bool success);

private:
    void setState(bool pulling, const QString& model, qreal progress, const QString& status,
                  const QString& error);
    void processChunk(const QByteArray& chunk);

    bool pulling_ = false;
    QString activeModel_;
    qreal progress_ = 0.0;
    QString statusText_;
    QString errorText_;
    class QNetworkAccessManager* nam_ = nullptr;
    class QNetworkReply* reply_ = nullptr;
};

// ── OllamaLibraryFetcher ──────────────────────────────────────────────────────
class OllamaLibraryFetcher : public QObject {
    Q_OBJECT
    friend class OllamaRuntimeTest;
    Q_PROPERTY(bool fetching READ fetching NOTIFY fetchingChanged)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit OllamaLibraryFetcher(QObject* parent = nullptr);

    bool fetching() const;
    QVariantList models() const;
    QString errorText() const;

    Q_INVOKABLE void fetch(const QString& sort);
    Q_INVOKABLE void cancel();

signals:
    void fetchingChanged();
    void modelsChanged();
    void errorTextChanged();
    void fetchFinished(bool success);

private:
    void handleReplyFinished();
    void parseHtml(const QString& html);

    bool fetching_ = false;
    QVariantList models_;
    QString errorText_;
    class QNetworkAccessManager* nam_ = nullptr;
    class QNetworkReply* reply_ = nullptr;
};

// ── OllamaModelDetailFetcher ──────────────────────────────────────────────────
class OllamaModelDetailFetcher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fetching READ fetching NOTIFY fetchingChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(QString readme READ readme NOTIFY readmeChanged)
    Q_PROPERTY(QVariantList tags READ tags NOTIFY tagsChanged)
    Q_PROPERTY(QString installCmd READ installCmd NOTIFY installCmdChanged)

public:
    explicit OllamaModelDetailFetcher(QObject* parent = nullptr);

    bool fetching() const;
    QString errorText() const;
    QString readme() const;
    QVariantList tags() const;
    QString installCmd() const;

    Q_INVOKABLE void fetchDetails(const QString& modelId);
    Q_INVOKABLE void cancel();

signals:
    void fetchingChanged();
    void errorTextChanged();
    void readmeChanged();
    void tagsChanged();
    void installCmdChanged();
    void fetchFinished(bool success);

private:
    void handleReplyFinished();
    void parseHtml(const QString& html);
    QString decodeHtml(QString text);

    bool fetching_ = false;
    QString errorText_;
    QString readme_;
    QVariantList tags_;
    QString installCmd_;
    QString modelId_;
    class QNetworkAccessManager* nam_ = nullptr;
    class QNetworkReply* reply_ = nullptr;
};

// ── LMStudioLibraryFetcher ───────────────────────────────────────────────────
class LMStudioLibraryFetcher : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool fetching READ fetching NOTIFY fetchingChanged)
    Q_PROPERTY(QVariantList models READ models NOTIFY modelsChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)

public:
    explicit LMStudioLibraryFetcher(QObject* parent = nullptr);

    bool fetching() const;
    QVariantList models() const;
    QString errorText() const;

    Q_INVOKABLE void fetch();
    Q_INVOKABLE void cancel();

signals:
    void fetchingChanged();
    void modelsChanged();
    void errorTextChanged();
    void fetchFinished(bool success);

private:
    void handleReplyFinished();
    void parseHtml(const QString& html);

    bool fetching_ = false;
    QVariantList models_;
    QString errorText_;
    class QNetworkAccessManager* nam_ = nullptr;
    class QNetworkReply* reply_ = nullptr;
};
