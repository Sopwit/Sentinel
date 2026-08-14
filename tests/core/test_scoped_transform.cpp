// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/state/ScopedTransform.h"

#include <QtTest>

using Sentinel::ScopedTransform;

class ScopedTransformTest final : public QObject {
    Q_OBJECT

private slots:
    void materializeReturnsInitialState();
    void addTransformModifiesState();
    void removeTransformRestoresState();
    void resetClearsAllTransforms();
    void multipleTransformsCompose();
};

void ScopedTransformTest::materializeReturnsInitialState() {
    ScopedTransform st({{"key", "value"}});
    auto state = st.materialize();
    QCOMPARE(state["key"].toString(), QStringLiteral("value"));
}

void ScopedTransformTest::addTransformModifiesState() {
    ScopedTransform st({{"key", "value"}});
    st.addTransform([](const QJsonObject &s) {
        QJsonObject copy = s;
        copy["key"] = "modified";
        return copy;
    });
    auto state = st.materialize();
    QCOMPARE(state["key"].toString(), QStringLiteral("modified"));
}

void ScopedTransformTest::removeTransformRestoresState() {
    ScopedTransform st({{"key", "value"}});
    QString id = st.addTransform([](const QJsonObject &s) {
        QJsonObject copy = s;
        copy["key"] = "modified";
        return copy;
    });
    st.removeTransform(id);
    auto state = st.materialize();
    QCOMPARE(state["key"].toString(), QStringLiteral("value"));
}

void ScopedTransformTest::resetClearsAllTransforms() {
    ScopedTransform st({{"key", "value"}});
    st.addTransform([](const QJsonObject &s) {
        QJsonObject copy = s;
        copy["key"] = "modified";
        return copy;
    });
    st.reset();
    auto state = st.materialize();
    QCOMPARE(state["key"].toString(), QStringLiteral("value"));
}

void ScopedTransformTest::multipleTransformsCompose() {
    ScopedTransform st({{"counter", 0}});
    st.addTransform([](const QJsonObject &s) {
        QJsonObject copy = s;
        copy["counter"] = s["counter"].toInt() + 1;
        return copy;
    });
    st.addTransform([](const QJsonObject &s) {
        QJsonObject copy = s;
        copy["counter"] = s["counter"].toInt() + 10;
        return copy;
    });
    auto state = st.materialize();
    QCOMPARE(state["counter"].toInt(), 11);
}

QTEST_MAIN(ScopedTransformTest)
#include "test_scoped_transform.moc"
