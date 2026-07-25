#include "sentinel/desktop/GraphicsBackend.h"

#include <QtTest>

class GraphicsBackendTest : public QObject {
    Q_OBJECT

private slots:
    void linuxDefaultsToOpenGl() {
        const auto api = sentinel::desktop::linuxDefaultGraphicsApi({}, {});

        QVERIFY(api.has_value());
        QCOMPARE(*api, QSGRendererInterface::OpenGL);
    }

    void linuxPreservesExplicitRhiOverride() {
        const auto api =
            sentinel::desktop::linuxDefaultGraphicsApi(QByteArrayLiteral("vulkan"), {});

        QVERIFY(!api.has_value());
    }

    void linuxPreservesExplicitSoftwareOverride() {
        const auto api =
            sentinel::desktop::linuxDefaultGraphicsApi({}, QByteArrayLiteral("software"));

        QVERIFY(!api.has_value());
    }
};

QTEST_APPLESS_MAIN(GraphicsBackendTest)

#include "test_graphics_backend.moc"
