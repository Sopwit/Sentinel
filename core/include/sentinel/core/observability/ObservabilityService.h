// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "sentinel/core/observability/IObservabilityService.h"
#include <QObject>
#include <QQueue>
#include <QTimer>
#include <QUuid>

namespace sentinel::core {

class ObservabilityService : public QObject, public IObservabilityService {
    Q_OBJECT
public:
    explicit ObservabilityService(QObject* parent = nullptr);
    ~ObservabilityService() override;

    // IObservabilityService interface
    void configure(const ObservabilityConfig& config) override;
    ObservabilityConfig config() const override;
    bool isEnabled() const override;

    QString startSpan(const QString& name, const QString& parentSpanId = {}) override;
    void endSpan(const QString& spanId, bool success = true) override;
    void setSpanAttribute(const QString& spanId, const QString& key, const QString& value) override;

    void recordMetric(const QString& name, double value, const QMap<QString, QString>& attributes = {}) override;
    void incrementCounter(const QString& name, const QMap<QString, QString>& attributes = {}) override;

    void logInfo(const QString& message, const QMap<QString, QString>& attributes = {}) override;
    void logWarning(const QString& message, const QMap<QString, QString>& attributes = {}) override;
    void logError(const QString& message, const QMap<QString, QString>& attributes = {}) override;

    QList<SpanData> recentSpans(int count = 100) const override;
    QList<MetricData> recentMetrics(const QString& name = {}, int count = 100) const override;

signals:
    void spanStarted(const QString& spanId, const QString& name);
    void spanEnded(const QString& spanId, bool success);
    void metricRecorded(const QString& name, double value);
    void logMessage(const QString& level, const QString& message);

private slots:
    void flushSpans();
    void flushMetrics();

private:
    QString generateId() const;
    void writeToFile(const QString& data);
    void exportToOtlp(const QList<SpanData>& spans, const QList<MetricData>& metrics);

    ObservabilityConfig m_config;
    QMap<QString, SpanData> m_activeSpans;
    QQueue<SpanData> m_completedSpans;
    QQueue<MetricData> m_metrics;
    QQueue<QPair<QString, QString>> m_logs;
    QTimer m_flushTimer;
    int m_maxStoredSpans{1000};
    int m_maxStoredMetrics{5000};
};

} // namespace sentinel::core
