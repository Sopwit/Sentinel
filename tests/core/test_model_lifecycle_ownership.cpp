// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include "sentinel/core/security/ProviderCredentials.h"
#include <QtTest>

using namespace sentinel::core;

class ModelLifecycleOwnershipTest final : public QObject {
    Q_OBJECT
private slots:
    void concurrentBindingsSurviveSelectionAndRegistryDestruction();
    void disconnectionPreservesSelection_data();
    void disconnectionPreservesSelection();
    void duplicateSelectionDoesNotRewriteSettings();
    void unknownProcessCannotBeTerminated();
    void unobservedEndpointIsNotReady_data();
    void unobservedEndpointIsNotReady();
};

void ModelLifecycleOwnershipTest::concurrentBindingsSurviveSelectionAndRegistryDestruction() {
    // These are provider-handle leases, not model-memory/process leases.
    ModelBindingResolution first, second;
    std::weak_ptr<IChatProvider> lifetime;
    {
        ModelService service;
        first = service.resolve("lm-studio", "fixture-a");
        second = service.resolve("lm-studio", "fixture-a");
        QVERIFY(first.ok());
        QVERIFY(second.ok());
        lifetime = first.provider;
        service.setSelectedModel({"llama-cpp-server", "fixture-b"});
        QCOMPARE(first.binding.modelId, QString("fixture-a"));
        QCOMPARE(second.binding.providerId, QString("lm-studio"));
    }
    QVERIFY(!lifetime.expired());
    first.provider.reset();
    QVERIFY(lifetime.expired());
    QVERIFY(second.provider); // Independently resolved provider remains alive.
    QCOMPARE(second.binding.modelId, QString("fixture-a"));
}

void ModelLifecycleOwnershipTest::disconnectionPreservesSelection_data() {
    QTest::addColumn<QString>("provider");
    for (const auto& id : {"ollama", "lm-studio", "llama-cpp-server", "openai-compatible-local"}) {
        QTest::newRow(id) << QString::fromLatin1(id);
    }
}

void ModelLifecycleOwnershipTest::disconnectionPreservesSelection() {
    QFETCH(QString, provider);
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    settings.setSelectedRuntimeProvider(provider);
    settings.setSelectedModelForProvider(provider, "user-choice");
    ModelService service(&settings);
    QCOMPARE(service.selectedModel().modelId, QString("user-choice"));
    const auto successSequence = service.beginProviderHealthObservation();
    if (provider == "ollama") {
        OllamaModelDiscoveryResult result;
        result.lifecycle = ChatRequestLifecycle::Completed;
        result.errorCategory = ChatProviderErrorCategory::None;
        result.models.append({"user-choice", {}, 1024});
        service.acceptOllamaDiscovery(result, successSequence);
    } else {
        service.acceptProviderDiscovery(provider, {{"user-choice", {}, 1024}},
                                        {true, ChatProviderErrorCategory::None, 200, "fixture"},
                                        successSequence);
    }
    // Compatible discovery stores catalog and reports health through separate calls.
    service.reportProviderDiscovery(provider, successSequence, true,
                                    ChatProviderErrorCategory::None);
    QCOMPARE(service.providerHealth(provider), ProviderHealth::Available);
    const auto failureSequence = service.beginProviderHealthObservation();
    service.reportProviderRequest(provider, failureSequence, false,
                                  ChatProviderErrorCategory::ConnectionFailed, "fixture");
    QCOMPARE(service.providerHealth(provider), ProviderHealth::Degraded);
    QCOMPARE(service.selectedModel().providerId, provider);
    QCOMPARE(service.selectedModel().modelId, QString("user-choice"));
    QCOMPARE(settings.selectedModelForProvider(provider), QString("user-choice"));
    // A late successful observation cannot erase the newer disconnection.
    service.reportProviderRequest(provider, successSequence, true, ChatProviderErrorCategory::None,
                                  "late fixture");
    QCOMPARE(service.providerHealth(provider), ProviderHealth::Degraded);
}

void ModelLifecycleOwnershipTest::duplicateSelectionDoesNotRewriteSettings() {
    AppSettings settings(std::make_unique<InMemorySettingsStore>(), inMemoryTestCredentialStore());
    settings.setSelectedRuntimeProvider("lm-studio");
    settings.setSelectedModelForProvider("lm-studio", "chosen-model");
    settings.setSelectedModelForProvider("ollama", "other-user-choice");
    ModelService service(&settings);
    QSignalSpy changed(&service, &ModelService::selectedModelChanged);
    service.setSelectedModel({"lm-studio", "chosen-model"});
    service.setSelectedModel({"lm-studio", "chosen-model"});
    QCOMPARE(changed.count(), 0);
    QCOMPARE(settings.selectedModelForProvider("ollama"), QString("other-user-choice"));
    QCOMPARE(settings.selectedRuntimeProvider(), QString("lm-studio"));
}

void ModelLifecycleOwnershipTest::unobservedEndpointIsNotReady_data() {
    QTest::addColumn<QString>("provider");
    for (const auto& id : {"lm-studio", "llama-cpp-server", "openai-compatible-local", "openai"}) {
        QTest::newRow(id) << QString::fromLatin1(id);
    }
}

void ModelLifecycleOwnershipTest::unobservedEndpointIsNotReady() {
    QFETCH(QString, provider);
    ModelService service;
    const auto status = service.providerStatusSnapshot(provider, "selected-but-unobserved");
    QCOMPARE(status.health, ProviderHealth::Unknown);
    QCOMPARE(status.catalog, ProviderCatalogState::Unverified);
    QVERIFY(status.modelIds.isEmpty());
}

void ModelLifecycleOwnershipTest::unknownProcessCannotBeTerminated() {
    ProcessExecutor processes;
    // No PID/name/endpoint can be adopted as an owned process handle.
    QVERIFY(!processes.terminate("external-ollama"));
    QVERIFY(!processes.kill("external-llama-server"));
    QVERIFY(processes.records().isEmpty());
    processes.shutdown();
    QVERIFY(processes.records().isEmpty());
}

QTEST_GUILESS_MAIN(ModelLifecycleOwnershipTest)
#include "test_model_lifecycle_ownership.moc"
