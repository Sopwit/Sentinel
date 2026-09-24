// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include "sentinel/core/runtime/InMemoryToolRegistry.h"
#include "sentinel/core/runtime/ToolArgumentValidator.h"
#include "sentinel/core/runtime/ToolExecutionGateway.h"
#include <QtTest>

using namespace sentinel::core;

class ToolArgumentValidatorTest final : public QObject {
    Q_OBJECT
private slots:
    void builtInContractsAreComplete() {
        for (const auto& tool : BuiltInToolProvider::descriptors()) {
            QVERIFY2(!tool.inputSchema.isEmpty(), qPrintable(tool.id));
            QVERIFY2(ToolArgumentValidator::validateSchema(tool.inputSchema).isEmpty(),
                     qPrintable(tool.id));
        }
    }
    void nestedConstraintsAndDefaults() {
        ToolDescriptor descriptor;
        descriptor.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject{
                 {QStringLiteral("options"),
                  QJsonObject{
                      {QStringLiteral("type"), QStringLiteral("object")},
                      {QStringLiteral("properties"),
                       QJsonObject{{QStringLiteral("enabled"),
                                    QJsonObject{{QStringLiteral("type"), QStringLiteral("boolean")},
                                                {QStringLiteral("default"), false}}},
                                   {QStringLiteral("paths"),
                                    QJsonObject{{QStringLiteral("type"), QStringLiteral("array")},
                                                {QStringLiteral("minItems"), 1},
                                                {QStringLiteral("items"),
                                                 QJsonObject{{QStringLiteral("type"),
                                                              QStringLiteral("string")},
                                                             {QStringLiteral("pattern"),
                                                              QStringLiteral("^/")}}}}}}},
                      {QStringLiteral("required"), QJsonArray{QStringLiteral("paths")}},
                      {QStringLiteral("additionalProperties"), false}}}}},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("options")}},
            {QStringLiteral("additionalProperties"), false}};
        const QList<ToolInvocationArgument> valid{
            {QStringLiteral("options"), QStringLiteral("{\"paths\":[\"/tmp\"]}"),
             QJsonObject{{QStringLiteral("paths"), QJsonArray{QStringLiteral("/tmp")}}}}};
        const auto accepted = ToolArgumentValidator::validate(descriptor, valid);
        QVERIFY(accepted.valid);
        QCOMPARE(accepted.normalizedArguments.value(QStringLiteral("options"))
                     .toObject()
                     .value(QStringLiteral("enabled"))
                     .toBool(),
                 false);
        const QList<ToolInvocationArgument> invalid{
            {QStringLiteral("options"), QString(),
             QJsonObject{{QStringLiteral("paths"),
                          QJsonArray{QStringLiteral("/ok"), QStringLiteral("bad")}},
                         {QStringLiteral("extra"), 1}}}};
        const auto rejected = ToolArgumentValidator::validate(descriptor, invalid);
        QVERIFY(!rejected.valid);
        QVERIFY(std::any_of(rejected.errors.begin(), rejected.errors.end(), [](const auto& issue) {
            return issue.path == QLatin1String("$.options.paths[1]");
        }));
        QVERIFY(std::any_of(rejected.errors.begin(), rejected.errors.end(), [](const auto& issue) {
            return issue.keyword == QLatin1String("additionalProperties");
        }));
    }
    void dynamicSourcesNeverReceiveInvalidArguments() {
        class Handler final : public IToolHandler {
        public:
            int calls = 0;
            IToolExecutor::Cancel execute(const ToolExecutionRequest&, const QString&,
                                          const QString&, IToolExecutor::Output,
                                          IToolExecutor::Completion done) override {
                ++calls;
                done({ToolExecutionStatus::Succeeded, QStringLiteral("called")});
                return {};
            }
        };
        InMemoryToolRegistry registry;
        auto handler = std::make_shared<Handler>();
        for (const auto source : {ToolSource::MCP, ToolSource::Plugin}) {
            ToolDescriptor descriptor;
            descriptor.id = source == ToolSource::MCP ? QStringLiteral("mcp.test.action")
                                                      : QStringLiteral("plugin.test.action");
            descriptor.source = source;
            descriptor.providerId = QStringLiteral("test");
            descriptor.inputSchema = QJsonObject{
                {QStringLiteral("type"), QStringLiteral("object")},
                {QStringLiteral("properties"),
                 QJsonObject{{QStringLiteral("count"),
                              QJsonObject{{QStringLiteral("type"), QStringLiteral("integer")}}}}},
                {QStringLiteral("required"), QJsonArray{QStringLiteral("count")}},
                {QStringLiteral("additionalProperties"), false}};
            QVERIFY(registry.registerTool({descriptor, handler}));
            ToolInvocationPlan plan;
            plan.status = ToolInvocationPlanStatus::Planned;
            plan.invocations.append({descriptor.id,
                                     {},
                                     {},
                                     {},
                                     ToolRiskLevel::Low,
                                     ToolExecutionMode::Local,
                                     {{QStringLiteral("count"), QStringLiteral("2.5"), 2.5}},
                                     {}});
            ToolExecutionGateway gateway(&registry);
            const auto validation = gateway.validatePlan(plan);
            QCOMPARE(validation.status, ToolExecutionStatus::InvalidArguments);
            QCOMPARE(handler->calls, 0);
        }
    }
    void malformedContractIsRejectedAtRegistration() {
        InMemoryToolRegistry registry;
        ToolDescriptor descriptor;
        descriptor.id = QStringLiteral("bad");
        descriptor.inputSchema = QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                             {QStringLiteral("oneOf"), QJsonArray{}}};
        QVERIFY(!registry.registerTool(descriptor));
    }
    void gatewayRejectsBeforeHandler() {
        class Handler final : public IToolHandler {
        public:
            int calls = 0;
            IToolExecutor::Cancel execute(const ToolExecutionRequest&, const QString&,
                                          const QString&, IToolExecutor::Output,
                                          IToolExecutor::Completion done) override {
                ++calls;
                done({ToolExecutionStatus::Succeeded, QStringLiteral("called")});
                return {};
            }
        };
        class Executor final : public IToolExecutor {
        public:
            ToolExecutionResult execute(const ToolExecutionRequest&) const override {
                return {ToolExecutionStatus::Succeeded, {}};
            }
        } executor;
        auto handler = std::make_shared<Handler>();
        InMemoryToolRegistry registry;
        ToolDescriptor descriptor;
        descriptor.id = QStringLiteral("sample");
        descriptor.inputSchema = QJsonObject{
            {QStringLiteral("type"), QStringLiteral("object")},
            {QStringLiteral("properties"),
             QJsonObject{{QStringLiteral("path"),
                          QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}}}},
            {QStringLiteral("required"), QJsonArray{QStringLiteral("path")}},
            {QStringLiteral("additionalProperties"), false}};
        QVERIFY(registry.registerTool({descriptor, handler}));
        ToolExecutionGateway gateway(&registry);
        ToolInvocationPlan plan;
        plan.status = ToolInvocationPlanStatus::Planned;
        plan.invocations.append({descriptor.id,
                                 {},
                                 {},
                                 {},
                                 ToolRiskLevel::Low,
                                 ToolExecutionMode::Local,
                                 {{QStringLiteral("unknown"), QStringLiteral("x")}},
                                 {}});
        const auto result = gateway.execute({plan,
                                             {ApprovalStatus::Approved, {}, {}},
                                             {SandboxStatus::Allowed, {}, {}},
                                             {descriptor.id}},
                                            executor);
        QCOMPARE(result.status, ToolExecutionStatus::InvalidArguments);
        QCOMPARE(handler->calls, 0);
    }
};
QTEST_MAIN(ToolArgumentValidatorTest)
#include "test_tool_argument_validator.moc"
