#include "sentinel/core/InMemoryMemoryCandidateStore.h"

#include <QtTest>

using sentinel::core::InMemoryMemoryCandidateStore;
using sentinel::core::MemoryCandidate;
using sentinel::core::MemoryCandidateCategory;
using sentinel::core::MemoryCandidateConfidence;
using sentinel::core::MemoryCandidateSource;
using sentinel::core::memoryCandidateSummary;
using sentinel::core::MemoryReviewState;

class InMemoryMemoryCandidateStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void createsCandidateWithDeterministicIdAndPendingReview();
    void recordsApprovalAndRejectionMetadata();
    void returnsDeterministicSummaries();
};

void InMemoryMemoryCandidateStoreTest::createsCandidateWithDeterministicIdAndPendingReview() {
    InMemoryMemoryCandidateStore store;
    MemoryCandidate candidate;
    candidate.source = MemoryCandidateSource::ConversationText;
    candidate.category = MemoryCandidateCategory::Semantic;
    candidate.confidence = MemoryCandidateConfidence::Medium;
    candidate.title = QStringLiteral("Project fact");
    candidate.excerpt = QStringLiteral("Sentinel uses Qt/QML.");

    const auto created = store.createCandidate(candidate);

    QCOMPARE(created.id, QStringLiteral("memory-candidate-1"));
    QCOMPARE(created.reviewState, MemoryReviewState::PendingReview);
    QCOMPARE(store.candidates().size(), 1);
}

void InMemoryMemoryCandidateStoreTest::recordsApprovalAndRejectionMetadata() {
    InMemoryMemoryCandidateStore store;
    MemoryCandidate first;
    first.title = QStringLiteral("First");
    MemoryCandidate second;
    second.title = QStringLiteral("Second");
    const auto firstCreated = store.createCandidate(first);
    const auto secondCreated = store.createCandidate(second);

    QVERIFY(store.setReviewState(firstCreated.id, MemoryReviewState::Approved,
                                 QStringLiteral("Approved by user.")));
    QVERIFY(store.setReviewState(secondCreated.id, MemoryReviewState::Rejected,
                                 QStringLiteral("Rejected by user.")));
    QVERIFY(!store.setReviewState(QStringLiteral("missing"), MemoryReviewState::Approved, {}));

    QCOMPARE(store.candidates().at(0).reviewState, MemoryReviewState::Approved);
    QCOMPARE(store.candidates().at(1).reviewState, MemoryReviewState::Rejected);
}

void InMemoryMemoryCandidateStoreTest::returnsDeterministicSummaries() {
    InMemoryMemoryCandidateStore store;
    MemoryCandidate candidate;
    candidate.title = QStringLiteral("Project fact");
    candidate.category = MemoryCandidateCategory::ProjectContext;
    candidate.confidence = MemoryCandidateConfidence::High;
    const auto created = store.createCandidate(candidate);

    const auto summary = memoryCandidateSummary(created);

    QCOMPARE(summary.id, QStringLiteral("memory-candidate-1"));
    QCOMPARE(summary.state, QStringLiteral("Pending Review"));
    QCOMPARE(summary.category, QStringLiteral("Project Context"));
    QCOMPARE(summary.confidence, QStringLiteral("High"));
    QCOMPARE(summary.summary,
             QStringLiteral("Project fact / Pending Review / Project Context / High"));
}

QTEST_MAIN(InMemoryMemoryCandidateStoreTest)

#include "test_in_memory_memory_candidate_store.moc"
