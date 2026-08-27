// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/session/MdnsServiceImpl.h"

#include <QRegularExpression>

namespace sentinel::core {

MdnsServiceImpl::MdnsServiceImpl(QObject* parent) : QObject(parent) {}
MdnsServiceImpl::~MdnsServiceImpl() {
    for (QProcess* process : m_publishProcesses) {
        process->terminate();
        process->deleteLater();
    }
}

bool MdnsServiceImpl::publish(const MdnsService& service) {
    if (!m_enabled)
        return false;
    if (service.name.isEmpty() || service.type.isEmpty() || service.port <= 0)
        return false;
    unpublish(service.name);
#if defined(Q_OS_MACOS)
    auto* process = new QProcess(this);
    process->start(QStringLiteral("dns-sd"),
                   {QStringLiteral("-R"), service.name, service.type,
                    service.domain.isEmpty() ? QStringLiteral("local") : service.domain,
                    QString::number(service.port)});
    if (!process->waitForStarted(2000)) {
        process->deleteLater();
        return false;
    }
    m_publishProcesses.insert(service.name, process);
    m_published.append(service);
    return true;
#elif defined(Q_OS_LINUX)
    auto* process = new QProcess(this);
    process->start(QStringLiteral("avahi-publish-service"),
                   {service.name, service.type, QString::number(service.port)});
    if (!process->waitForStarted(2000)) {
        process->deleteLater();
        return false;
    }
    m_publishProcesses.insert(service.name, process);
    m_published.append(service);
    return true;
#else
    Q_UNUSED(service);
    return false;
#endif
}

bool MdnsServiceImpl::unpublish(const QString& name) {
    for (int i = 0; i < m_published.size(); ++i) {
        if (m_published[i].name == name) {
            if (auto* process = m_publishProcesses.take(name)) {
                process->terminate();
                process->deleteLater();
            }
            m_published.removeAt(i);
            return true;
        }
    }
    return false;
}

QList<MdnsService> MdnsServiceImpl::discoveredServices() const {
    if (!m_enabled)
        return {};
    m_discovered.clear();
#if defined(Q_OS_LINUX)
    QProcess process;
    process.start(QStringLiteral("avahi-browse"),
                  {QStringLiteral("-rt"), QStringLiteral("_sentinel._tcp")});
    if (process.waitForFinished(3000)) {
        const QString output = QString::fromUtf8(process.readAllStandardOutput());
        for (const QString& line : output.split('\n')) {
            const QStringList fields = line.split(';');
            if (fields.size() < 9 || fields[0] != QStringLiteral("=") ||
                fields[1] != QStringLiteral("*"))
                continue;
            bool ok = false;
            const int port = fields[8].toInt(&ok);
            if (ok)
                m_discovered.append({fields[3], fields[4], fields[6], port});
        }
    }
#endif
    return m_discovered;
}

void MdnsServiceImpl::setEnabled(bool enabled) {
    m_enabled = enabled;
}
bool MdnsServiceImpl::isEnabled() const {
    return m_enabled;
}

} // namespace sentinel::core
