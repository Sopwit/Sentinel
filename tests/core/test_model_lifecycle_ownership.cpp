// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/app/AppSettings.h"
#include "sentinel/core/memory/InMemorySettingsStore.h"
#include "sentinel/core/model/ModelService.h"
#include "sentinel/core/model/ModelStorageManager.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include "sentinel/core/security/ProviderCredentials.h"
#include <QCryptographicHash>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QTemporaryDir>
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
    void activeAgentRunRetainsModelBindingAcrossSelectionChange();
    void modelDeletionDoesNotInvalidateActiveRunBinding();
    void repeatedShutdownAndProcessOwnershipInvariants();
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

void ModelLifecycleOwnershipTest::activeAgentRunRetainsModelBindingAcrossSelectionChange() {
    ModelService service;
    const auto initial = service.resolve("ollama", "qwen-2.5-coder");
    QVERIFY(initial.ok());
    QCOMPARE(initial.binding.providerId, QString("ollama"));
    QCOMPARE(initial.binding.modelId, QString("qwen-2.5-coder"));

    // Global selection changes to different provider, model, and endpoint
    service.setSelectedModel({"llama-cpp-server", "mistral-7b"});
    service.setOllamaEndpoint("http://127.0.0.1:19999");
    service.setSelectedProviderId("openai");

    // Captured binding and provider held by active run remain completely unchanged
    QVERIFY(initial.ok());
    QCOMPARE(initial.binding.providerId, QString("ollama"));
    QCOMPARE(initial.binding.modelId, QString("qwen-2.5-coder"));
    QVERIFY(initial.provider != nullptr);

    // New resolution produces distinct binding
    const auto second = service.resolve("llama-cpp-server", "mistral-7b");
    QVERIFY(second.ok());
    QCOMPARE(second.binding.providerId, QString("llama-cpp-server"));
    QCOMPARE(second.binding.modelId, QString("mistral-7b"));
    QVERIFY(second.provider != initial.provider);
}

void ModelLifecycleOwnershipTest::modelDeletionDoesNotInvalidateActiveRunBinding() {
    QTemporaryDir tempDir;
    QVERIFY(tempDir.isValid());
    const auto root = tempDir.path();
    ModelStorageManager storage;
    QVERIFY(storage.setActiveRoot(root));

    const QString repo = "sentinel/fixture-repo";
    const QString rev = "0123456789abcdef0123456789abcdef01234567";
    const QString filename = "model.gguf";
    const auto identity = QStringLiteral("%1@%2:%3").arg(repo, rev, filename);
    const auto dirName = QString::fromLatin1(
        QCryptographicHash::hash(identity.toUtf8(), QCryptographicHash::Sha256).toHex());
    QDir artifactDir(QDir(root).filePath(dirName));
    QVERIFY(artifactDir.mkpath("."));
    const auto artifactPath = artifactDir.filePath(filename);
    const auto sidecarPath = artifactPath + QStringLiteral(".source.json");

    {
        QFile file(artifactPath);
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("GGUF artifact data");
    }
    {
        QFile sidecar(sidecarPath);
        QVERIFY(sidecar.open(QIODevice::WriteOnly));
        QJsonObject obj{{QStringLiteral("repositoryId"), repo},
                        {QStringLiteral("filename"), filename},
                        {QStringLiteral("revision"), rev}};
        sidecar.write(QJsonDocument(obj).toJson());
    }

    // Active run resolves and holds binding
    ModelService service;
    const auto resolution = service.resolve("llama-cpp-server", "model.gguf");
    QVERIFY(resolution.ok());
    QCOMPARE(resolution.binding.modelId, QString("model.gguf"));

    // Delete artifact via storage manager: succeeds because no in-use lease fence exists (witness
    // D6)
    QVERIFY(storage.removeArtifact(artifactPath));
    QVERIFY(!QFile::exists(artifactPath));

    // Active run's captured binding and provider instance remain intact in memory (Part D
    // guarantee)
    QVERIFY(resolution.ok());
    QCOMPARE(resolution.binding.modelId, QString("model.gguf"));
    QVERIFY(resolution.provider != nullptr);
}

void ModelLifecycleOwnershipTest::repeatedShutdownAndProcessOwnershipInvariants() {
    ProcessExecutor processes;
    QVERIFY(processes.records().isEmpty());

    // Repeated shutdown is safe and idempotent
    processes.shutdown();
    processes.shutdown();
    QVERIFY(processes.records().isEmpty());

    // Refuses termination of unknown / external handles
    QVERIFY(!processes.terminate("external-pid-1234"));
    QVERIFY(!processes.kill("external-pid-1234"));
    QVERIFY(processes.records().isEmpty());
}

QTEST_GUILESS_MAIN(ModelLifecycleOwnershipTest)
#include "test_model_lifecycle_ownership.moc"
