// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include <QCoreApplication>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTimer>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <thread>

namespace {
void send(const QJsonObject& response) {
    const auto bytes = QJsonDocument(response).toJson(QJsonDocument::Compact) + '\n';
    std::fwrite(bytes.constData(), 1, static_cast<size_t>(bytes.size()), stdout);
    std::fflush(stdout);
}
} // namespace

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    const QString logPath = app.arguments().value(1);
    const bool certification = app.arguments().contains(QStringLiteral("--certification"));
    auto handle = [&](const QByteArray& line) {
        const auto request = QJsonDocument::fromJson(line).object();
        const auto method = request.value(QStringLiteral("method")).toString();
        if (method == QStringLiteral("notifications/initialized"))
            return;
        const auto id = request.value(QStringLiteral("id"));
        if (method == QStringLiteral("initialize")) {
            send({{"jsonrpc", "2.0"},
                  {"id", id},
                  {"result", QJsonObject{{"protocolVersion", "2024-11-05"},
                                         {"capabilities", QJsonObject{}},
                                         {"serverInfo", QJsonObject{{"name", "sentinel-test-mcp"},
                                                                    {"version", "1"}}}}}});
            return;
        }
        if (method == QStringLiteral("tools/list")) {
            const QJsonObject schema{
                {"type", "object"},
                {"properties", QJsonObject{{"value", QJsonObject{{"type", "string"}}}}},
                {"required", QJsonArray{QStringLiteral("value")}},
                {"additionalProperties", false}};
            QJsonArray tools{QJsonObject{{"name", "echo_value"},
                                         {"description", "Echo a value"},
                                         {"inputSchema", schema}},
                             QJsonObject{{"name", "delayed_echo"},
                                         {"description", "Delayed echo"},
                                         {"inputSchema", schema}},
                             QJsonObject{{"name", "crash_echo"},
                                         {"description", "Exit during call"},
                                         {"inputSchema", schema}}};
            if (certification) {
                tools.append(QJsonObject{
                    {"name", "add"},
                    {"description", "Add two numbers"},
                    {"inputSchema",
                     QJsonObject{
                         {"type", "object"},
                         {"additionalProperties", false},
                         {"required", QJsonArray{"a", "b"}},
                         {"properties", QJsonObject{{"a", QJsonObject{{"type", "number"}}},
                                                    {"b", QJsonObject{{"type", "number"}}}}}}}});
                for (const auto& name :
                     {"malformed_json", "missing_result", "wrong_id", "server_error"})
                    tools.append(QJsonObject{{"name", name},
                                             {"description", "Certification fault injection"},
                                             {"inputSchema", schema}});
            }
            send({{"jsonrpc", "2.0"}, {"id", id}, {"result", QJsonObject{{"tools", tools}}}});
            return;
        }
        if (method == QStringLiteral("tools/call")) {
            const auto params = request.value(QStringLiteral("params")).toObject();
            const auto name = params.value(QStringLiteral("name")).toString();
            if (certification && name == "add") {
                const auto arguments = params.value("arguments").toObject();
                const double sum =
                    arguments.value("a").toDouble() + arguments.value("b").toDouble();
                send({{"jsonrpc", "2.0"},
                      {"id", id},
                      {"result", QJsonObject{{"content", QJsonArray{QJsonObject{
                                                             {"type", "text"},
                                                             {"text", QString::number(sum)}}}},
                                             {"structuredContent", QJsonObject{{"sum", sum}}}}}});
                return;
            }
            if (certification && name == "malformed_json") {
                std::fputs("{broken-json\n", stdout);
                std::fflush(stdout);
                return;
            }
            if (certification && name == "missing_result") {
                send({{"jsonrpc", "2.0"}, {"id", id}});
                return;
            }
            if (certification && name == "wrong_id") {
                send({{"jsonrpc", "2.0"},
                      {"id", id.toInt() + 10000},
                      {"result", QJsonObject{{"content", QJsonArray{}}}}});
                return;
            }
            if (certification && name == "server_error") {
                send({{"jsonrpc", "2.0"},
                      {"id", id},
                      {"error",
                       QJsonObject{{"code", -32000}, {"message", "Deterministic server error"}}}});
                return;
            }
            QFile log(logPath);
            if (log.open(QIODevice::Append)) {
                log.write(name.toUtf8() + '\n');
                log.close();
            }
            const auto value = params.value(QStringLiteral("arguments"))
                                   .toObject()
                                   .value(QStringLiteral("value"))
                                   .toString();
            const auto answer = [id, value, name, logPath] {
                QJsonObject content{{"type", "text"}, {"text", "ECHO: " + value}};
                QJsonObject result{{"content", QJsonArray{content}}};
                send({{"jsonrpc", "2.0"}, {"id", id}, {"result", result}});
                QFile log(logPath);
                if (log.open(QIODevice::Append))
                    log.write(QByteArray("response:") + name.toUtf8() + '\n');
            };
            if (name == QStringLiteral("delayed_echo"))
                QTimer::singleShot(400, &app, answer);
            else if (name == QStringLiteral("crash_echo"))
                QTimer::singleShot(0, &app, [] { std::_Exit(7); });
            else
                answer();
        }
    };
    std::thread reader([&] {
        std::string line;
        while (std::getline(std::cin, line)) {
            QMetaObject::invokeMethod(
                &app, [&, bytes = QByteArray::fromStdString(line)] { handle(bytes); },
                Qt::QueuedConnection);
        }
        QMetaObject::invokeMethod(&app, &QCoreApplication::quit, Qt::QueuedConnection);
    });
    reader.detach();
    return app.exec();
}
