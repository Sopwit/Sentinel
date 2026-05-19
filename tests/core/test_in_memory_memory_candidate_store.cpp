#include "sentinel/core/InMemoryMemoryCandidateStore.h"

#include <QtTest>

using sentinel::core::InMemoryMemoryCandidateStore;
using sentinel::core::MemoryCandidate;
using sentinel::core::MemoryCandidateCategory;
using sentinel::core::MemoryCandidateConfidence;
using sentinel::core::MemoryCandidateReviewAction;
using sentinel::core::MemoryCandidateSource;
using sentinel::core::memoryCandidateSummary;
using sentinel::core::MemoryCommitPolicy;
using sentinel::core::memoryCommitReadinessForCandidate;
using sentinel::core::memoryCommitReadinessStatusName;
using sentinel::core::MemoryReviewState;

class InMemoryMemoryCandidateStoreTest final : public QObject {
    Q_OBJECT

private slots:
    void createsCandidateWithDeterministicIdAndPendingReview();
    void recordsApprovalAndRejectionMetadata();
    void refusesInvalidReviewTransitions();
    void archivesReviewedCandidatesAsTerminalMetadata();
    void returnsDeterministicSummaries();
    void returnsCommitReadinessMetadata();
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

    const auto approved = store.reviewCandidate(
        firstCreated.id, MemoryCandidateReviewAction::Approve, QStringLiteral("Reviewer: Test"),
        QStringLiteral("Approved by user."));
    const auto rejected = store.reviewCandidate(
        secondCreated.id, MemoryCandidateReviewAction::Reject, QStringLiteral("Reviewer: Test"),
        QStringLiteral("Rejected by user."));
    const auto missing = store.reviewCandidate(QStringLiteral("missing"),
                                               MemoryCandidateReviewAction::Approve, {}, {});

    QVERIFY(approved.accepted);
    QVERIFY(rejected.accepted);
    QVERIFY(!missing.accepted);
    QCOMPARE(approved.sourceSummary, QStringLiteral("Conversation text metadata only."));
    QCOMPARE(store.candidates().at(0).reviewState, MemoryReviewState::Approved);
    QCOMPARE(store.candidates().at(1).reviewState, MemoryReviewState::Rejected);
    QVERIFY(store.candidates().at(0).reviewedAtUtc.isValid());
    QCOMPARE(store.candidates().at(0).reviewerSummary, QStringLiteral("Reviewer: Test"));
    QCOMPARE(store.candidates().at(0).decisionReason, QStringLiteral("Approved by user."));
}

void InMemoryMemoryCandidateStoreTest::refusesInvalidReviewTransitions() {
    InMemoryMemoryCandidateStore store;
    const auto created = store.createCandidate(MemoryCandidate{});

    const auto reset =
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::ResetToPending, {}, {});
    const auto archived =
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::Archive, {}, {});
    const auto approved =
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::Approve, {}, {});
    const auto rejectedAfterApprove =
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::Reject, {}, {});

    QVERIFY(!reset.accepted);
    QVERIFY(!archived.accepted);
    QVERIFY(approved.accepted);
    QVERIFY(!rejectedAfterApprove.accepted);
    QCOMPARE(store.candidates().first().reviewState, MemoryReviewState::Approved);
}

void InMemoryMemoryCandidateStoreTest::archivesReviewedCandidatesAsTerminalMetadata() {
    InMemoryMemoryCandidateStore store;
    const auto created = store.createCandidate(MemoryCandidate{});

    QVERIFY(
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::Reject, {}, {}).accepted);
    const auto archived =
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::Archive, {}, {});
    const auto resetAfterArchive =
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::ResetToPending, {}, {});

    QVERIFY(archived.accepted);
    QVERIFY(!resetAfterArchive.accepted);
    QCOMPARE(store.candidates().first().reviewState, MemoryReviewState::Archived);
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
             QStringLiteral("Project fact / Pending Review / Project Context / High / "
                            "Conversation Text / "
                            "Not reviewed / Reviewer: User review"));
}

void InMemoryMemoryCandidateStoreTest::returnsCommitReadinessMetadata() {
    InMemoryMemoryCandidateStore store;
    MemoryCandidate candidate;
    candidate.excerpt = QStringLiteral("Commit planning stays metadata only.");
    const auto created = store.createCandidate(candidate);

    const auto pending = memoryCommitReadinessForCandidate(&created, true, MemoryCommitPolicy{});
    QCOMPARE(memoryCommitReadinessStatusName(pending.status), QStringLiteral("Pending Review"));

    QVERIFY(
        store.reviewCandidate(created.id, MemoryCandidateReviewAction::Approve, {}, {}).accepted);
    const auto approved = store.candidates().first();
    const auto readiness = memoryCommitReadinessForCandidate(&approved, true, MemoryCommitPolicy{});

    QCOMPARE(memoryCommitReadinessStatusName(readiness.status), QStringLiteral("Disabled"));
    QCOMPARE(readiness.plan.candidateId, QStringLiteral("memory-candidate-1"));
    QCOMPARE(readiness.plan.key, QStringLiteral("memory-candidate-1"));
    QVERIFY(readiness.plan.summary.contains(QStringLiteral("Key-value Memory")));
    QVERIFY(readiness.summary.contains(QStringLiteral("disabled")));
}

QTEST_MAIN(InMemoryMemoryCandidateStoreTest)

#include "test_in_memory_memory_candidate_store.moc"
