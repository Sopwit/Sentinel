// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/observability/ObservabilityService.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QDir>
#include <QStandardPaths>
#include <QDateTime>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>

namespace sentinel::core {

ObservabilityService::ObservabilityService(QObject* parent)
    : QObject(parent)
{
    connect(&m_flushTimer, &QTimer::timeout, this, &ObservabilityService::flushSpans);
    connect(&m_flushTimer, &QTimer::timeout, this, &ObservabilityService::flushMetrics);
}

ObservabilityService::~ObservabilityService() {
    flushSpans();
    flushMetrics();
}

void ObservabilityService::configure(const ObservabilityConfig& config) {
    m_config = config;

    if (m_config.enabled) {
        m_flushTimer.start(5000); // Flush every 5 seconds

        if (m_config.logFilePath.isEmpty()) {
            m_config.logFilePath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                                   + "/logs/observability.jsonl";
            QDir().mkpath(QFileInfo(m_config.logFilePath).absolutePath());
        }

        qDebug() << QStringLiteral("ObservabilityService: Enabled with endpoint %1").arg(m_config.otlpEndpoint);
    } else {
        m_flushTimer.stop();
        qDebug() << "ObservabilityService: Disabled";
    }
}

ObservabilityConfig ObservabilityService::config() const {
    return m_config;
}

bool ObservabilityService::isEnabled() const {
    return m_config.enabled;
}

QString ObservabilityService::startSpan(const QString& name, const QString& parentSpanId) {
    if (!m_config.enabled) {
        return {};
    }

    SpanData span;
    span.name = name;
    span.traceId = generateId();
    span.spanId = generateId();
    span.parentSpanId = parentSpanId;
    span.startTimeMs = QDateTime::currentMSecsSinceEpoch();
    span.isRoot = parentSpanId.isEmpty();

    m_activeSpans[span.spanId] = span;
    emit spanStarted(span.spanId, name);

    return span.spanId;
}

void ObservabilityService::endSpan(const QString& spanId, bool success) {
    if (!m_config.enabled) {
        return;
    }

    auto it = m_activeSpans.find(spanId);
    if (it == m_activeSpans.end()) {
        return;
    }

    it->endTimeMs = QDateTime::currentMSecsSinceEpoch();
    m_completedSpans.enqueue(*it);
    m_activeSpans.erase(it);

    emit spanEnded(spanId, success);

    if (m_completedSpans.size() >= m_maxStoredSpans) {
        flushSpans();
    }
}

void ObservabilityService::setSpanAttribute(const QString& spanId, const QString& key, const QString& value) {
    if (!m_config.enabled) {
        return;
    }

    auto it = m_activeSpans.find(spanId);
    if (it != m_activeSpans.end()) {
        it->attributes[key] = value;
    }
}

void ObservabilityService::recordMetric(const QString& name, double value, const QMap<QString, QString>& attributes) {
    if (!m_config.enabled) {
        return;
    }

    MetricData metric;
    metric.name = name;
    metric.value = value;
    metric.attributes = attributes;

    m_metrics.enqueue(metric);
    emit metricRecorded(name, value);

    if (m_metrics.size() >= m_maxStoredMetrics) {
        flushMetrics();
    }
}

void ObservabilityService::incrementCounter(const QString& name, const QMap<QString, QString>& attributes) {
    recordMetric(name, 1.0, attributes);
}

void ObservabilityService::logInfo(const QString& message, const QMap<QString, QString>& attributes) {
    Q_UNUSED(attributes)
    m_logs.enqueue({message, "info"});
    emit logMessage("info", message);
}

void ObservabilityService::logWarning(const QString& message, const QMap<QString, QString>& attributes) {
    Q_UNUSED(attributes)
    m_logs.enqueue({message, "warning"});
    emit logMessage("warning", message);
}

void ObservabilityService::logError(const QString& message, const QMap<QString, QString>& attributes) {
    Q_UNUSED(attributes)
    m_logs.enqueue({message, "error"});
    emit logMessage("error", message);
}

QList<SpanData> ObservabilityService::recentSpans(int count) const {
    QList<SpanData> spans;
    for (const auto& s : m_completedSpans) {
        spans.append(s);
    }
    return spans.mid(qMax(0, spans.size() - count));
}

QList<MetricData> ObservabilityService::recentMetrics(const QString& name, int count) const {
    QList<MetricData> metrics;
    for (const auto& m : m_metrics) {
        if (name.isEmpty() || m.name == name) {
            metrics.append(m);
        }
    }
    return metrics.mid(qMax(0, metrics.size() - count));
}

void ObservabilityService::flushSpans() {
    if (m_completedSpans.isEmpty()) {
        return;
    }

    QList<SpanData> spans;
    while (!m_completedSpans.isEmpty()) {
        spans.append(m_completedSpans.dequeue());
    }

    // Export to OTLP or file
    exportToOtlp(spans, {});

    // Write to file if configured
    if (m_config.logToFile && !m_config.logFilePath.isEmpty()) {
        for (const auto& span : spans) {
            QJsonObject spanObj;
            spanObj["traceId"] = span.traceId;
            spanObj["spanId"] = span.spanId;
            spanObj["parentSpanId"] = span.parentSpanId;
            spanObj["name"] = span.name;
            spanObj["startTimeMs"] = span.startTimeMs;
            spanObj["endTimeMs"] = span.endTimeMs;
            spanObj["durationMs"] = span.endTimeMs - span.startTimeMs;

            QJsonObject attrsObj;
            for (auto it = span.attributes.begin(); it != span.attributes.end(); ++it) {
                attrsObj[it.key()] = it.value();
            }
            spanObj["attributes"] = attrsObj;

            QJsonDocument doc(spanObj);
            writeToFile(doc.toJson(QJsonDocument::Compact));
        }
    }
}

void ObservabilityService::flushMetrics() {
    if (m_metrics.isEmpty()) {
        return;
    }

    QList<MetricData> metrics;
    while (!m_metrics.isEmpty()) {
        metrics.append(m_metrics.dequeue());
    }

    exportToOtlp({}, metrics);
}

QString ObservabilityService::generateId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces).remove('-').left(16);
}

void ObservabilityService::writeToFile(const QString& data) {
    QFile file(m_config.logFilePath);
    if (file.open(QIODevice::Append | QIODevice::Text)) {
        file.write(data.toUtf8());
        file.write("\n");
    }
}

void ObservabilityService::exportToOtlp(const QList<SpanData>& spans, const QList<MetricData>& metrics) {
    if (!m_config.enabled || m_config.otlpEndpoint.isEmpty()) {
        return;
    }

    // Convert to OTLP JSON format.
    QJsonObject resource;
    QJsonObject resourceAttrs;
    resourceAttrs["service.name"] = m_config.serviceName;
    resourceAttrs["service.version"] = m_config.serviceVersion;
    resource["attributes"] = QJsonArray{
        QJsonObject{{"key", "service.name"}, {"value", QJsonObject{{"stringValue", m_config.serviceName}}}},
        QJsonObject{{"key", "service.version"}, {"value", QJsonObject{{"stringValue", m_config.serviceVersion}}}}
    };

    QJsonObject payload;
    QJsonArray spanArray;
    for (const SpanData& span : spans) {
        QJsonArray attributes;
        for (auto it = span.attributes.constBegin(); it != span.attributes.constEnd(); ++it) {
            attributes.append(QJsonObject{{"key", it.key()},
                                          {"value", QJsonObject{{"stringValue", it.value()}}}});
        }
        spanArray.append(QJsonObject{{"traceId", span.traceId}, {"spanId", span.spanId},
                                     {"name", span.name},
                                     {"startTimeUnixNano", QString::number(span.startTimeMs * 1000000)},
                                     {"endTimeUnixNano", QString::number(span.endTimeMs * 1000000)},
                                     {"attributes", attributes}});
    }
    payload["resourceSpans"] = QJsonArray{
        QJsonObject{
            {"resource", resource},
            {"scopeSpans", QJsonArray{
                QJsonObject{
                    {"scope", QJsonObject{{"name", "sentinel"}}},
                    {"spans", spanArray}
                }
            }}
        }
    };

    QJsonArray metricArray;
    for (const MetricData& metric : metrics) {
        metricArray.append(QJsonObject{{"name", metric.name}, {"value", metric.value}});
    }
    if (!metricArray.isEmpty()) payload["resourceMetrics"] = metricArray;

    QJsonDocument doc(payload);
    QNetworkAccessManager* manager = new QNetworkAccessManager(this);
    QNetworkRequest request;
    request.setUrl(QUrl(m_config.otlpEndpoint));
    request.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    QNetworkReply* reply = manager->post(request, doc.toJson(QJsonDocument::Compact));
    QObject::connect(reply, &QNetworkReply::finished, manager, [reply, manager]() {
        if (reply->error() != QNetworkReply::NoError) {
            qWarning() << "Sentinel OTLP export failed:" << reply->errorString();
        }
        reply->deleteLater();
        manager->deleteLater();
    });
}

} // namespace sentinel::core
