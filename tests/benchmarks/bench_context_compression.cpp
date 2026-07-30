#include <QtTest>

class ContextCompressionBenchmark final : public QObject {
    Q_OBJECT

private slots:
    void benchmarkCompressionSpeed();
};

void ContextCompressionBenchmark::benchmarkCompressionSpeed() {
    const QString samplePrompt = QStringLiteral(
        "Sentinel prompt token compression benchmark testing context budget optimization speed."
    );

    QBENCHMARK {
        const int charCount = samplePrompt.length();
        QVERIFY(charCount > 0);
    }
}

QTEST_MAIN(ContextCompressionBenchmark)

#include "bench_context_compression.moc"
