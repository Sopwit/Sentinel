// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QJsonObject>
#include <QMap>
#include <QString>

namespace sentinel::core {

struct ObservabilityConfig {
    bool enabled{false};
    QString serviceName{"sentinel"};
    QString serviceVersion{"1.0.0"};
    QString otlpEndpoint{"http://localhost:4318"};
    QString protocol{"http"};
    double samplingRatio{1.0};
    bool logToConsole{false};
    bool logToFile{true};
    QString logFilePath;
};

struct SpanData {
    QString name;
    QString traceId;
    QString spanId;
    QString parentSpanId;
    qint64 startTimeMs{0};
    qint64 endTimeMs{0};
    QMap<QString, QString> attributes;
    bool isRoot{false};
};

struct MetricData {
    QString name;
    QString description;
    QString unit;
    double value{0.0};
    QMap<QString, QString> attributes;
};

class IObservabilityService {
public:
    virtual ~IObservabilityService() = default;

    // Configuration
    virtual void configure(const ObservabilityConfig& config) = 0;
    virtual ObservabilityConfig config() const = 0;
    virtual bool isEnabled() const = 0;

    // Tracing
    virtual QString startSpan(const QString& name, const QString& parentSpanId = {}) = 0;
    virtual void endSpan(const QString& spanId, bool success = true) = 0;
    virtual void setSpanAttribute(const QString& spanId, const QString& key,
                                  const QString& value) = 0;

    // Metrics
    virtual void recordMetric(const QString& name, double value,
                              const QMap<QString, QString>& attributes = {}) = 0;
    virtual void incrementCounter(const QString& name,
                                  const QMap<QString, QString>& attributes = {}) = 0;

    // Logging
    virtual void logInfo(const QString& message, const QMap<QString, QString>& attributes = {}) = 0;
    virtual void logWarning(const QString& message,
                            const QMap<QString, QString>& attributes = {}) = 0;
    virtual void logError(const QString& message,
                          const QMap<QString, QString>& attributes = {}) = 0;

    // Query
    virtual QList<SpanData> recentSpans(int count = 100) const = 0;
    virtual QList<MetricData> recentMetrics(const QString& name = {}, int count = 100) const = 0;
};

} // namespace sentinel::core
