// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/chat/ChatAttachmentLoader.h"
#include "sentinel/core/chat/ChatImageContent.h"
#include "sentinel/core/chat/SQLiteConversationStore.h"
#include "sentinel/core/app/IClock.h"
#include <QBuffer>
#include <QFile>
#include <QImage>
#include <QJsonDocument>
#include <QTemporaryDir>
#include <QUrl>
#include <QtTest>
using namespace sentinel::core;
class RecordingProvider final : public IChatProvider {
public:
    QString prompt;
    QList<ChatImage> images;
    QString name() const override { return "recording"; }
    ChatProviderStatus status() const override { return ChatProviderStatus::Ready; }
    ChatProviderReply sendMessage(const QString&) override { return {}; }
    ChatProviderReply sendRequest(const QString& text, const ChatRequestOptions& options) override {
        prompt = text; images = options.images;
        ChatProviderReply reply; reply.success = true; reply.message = "inspected attachment";
        reply.lifecycle = ChatRequestLifecycle::Completed; return reply;
    }
};
class ChatAttachmentsTest final : public QObject {
    Q_OBJECT
private slots:
    void explicitFilesReachProviderAndSurviveReload() {
        QTemporaryDir directory;
        const auto path = directory.filePath("dosya özel.txt");
        QFile file(path); QVERIFY(file.open(QIODevice::WriteOnly)); file.write("unique attachment content"); file.close();
        const auto imagePath = directory.filePath("picture.png");
        QImage image(32, 32, QImage::Format_RGB32); image.fill(Qt::red); QVERIFY(image.save(imagePath));
        const auto loaded = loadChatAttachments({QUrl::fromLocalFile(path).toString(), imagePath});
        QVERIFY2(loaded.ok(), qPrintable(loaded.error));
        QCOMPARE(loaded.attachments.size(), 2);
        QCOMPARE(loaded.attachments.first().text, "unique attachment content");
        QVERIFY(!loaded.attachments.last().imageBytes.isEmpty());
        SQLiteConversationStore store(directory.filePath("conversations.db"));
        const auto id = store.createConversation("Attachments").id;
        ChatSession session(std::make_unique<SystemClock>());
        ModelService models;
        auto provider = std::make_shared<RecordingProvider>();
        models.registerProvider("ollama", [provider](const ModelBinding&) { return provider; });
        ChatModeService service(models, session, store);
        QVERIFY(service.send(id, "Read these", loaded.attachments, {"ollama", "fixture"}));
        QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
        QVERIFY(provider->prompt.contains("unique attachment content"));
        QCOMPARE(provider->images.size(), 1);
        QCOMPARE(provider->images.first().bytes, loaded.attachments.last().imageBytes);
        QCOMPARE(session.messages().first().content, "Read these");
        const auto restored = QJsonDocument::fromJson(store.loadMessages(id).first().attachmentsJson.toUtf8()).array();
        QCOMPARE(restored.size(), 2);
        QCOMPARE(QByteArray::fromBase64(restored.last().toObject().value("image").toString().toLatin1()), loaded.attachments.last().imageBytes);
        ChatMessage user = session.messages().first();
        user.attachmentData = restored;
        session.loadMessages({user});
        provider->images.clear();
        QVERIFY(service.regenerate(id, user.id, {"ollama", "fixture"}));
        QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
        QCOMPARE(provider->images.size(), 1);
        QVERIFY(service.send(id, "Describe that picture again", {}, {"ollama", "fixture"}));
        QTRY_VERIFY_WITH_TIMEOUT(!service.busy(), 3000);
        QCOMPARE(provider->images.size(), 1);
        QVERIFY(provider->prompt.contains("unique attachment content"));
    }
    void rejectsMissingBinaryAndOversizedInputs() {
        QTemporaryDir directory;
        QVERIFY(!loadChatAttachments({directory.filePath("missing.txt")}).ok());
        QFile file(directory.filePath("binary.txt")); QVERIFY(file.open(QIODevice::WriteOnly)); file.write(QByteArray("abc\0def",7)); file.close();
        QVERIFY(!loadChatAttachments({file.fileName()}).ok());
        QVERIFY(file.open(QIODevice::WriteOnly)); QVERIFY(file.resize(4 * 1024 * 1024 + 1)); file.close();
        QVERIFY(!loadChatAttachments({file.fileName()}).ok());
        QVERIFY(!loadChatAttachments({"a","b","c","d","e"}).ok());
    }
    void serializesActualImageContentForProviders() {
        const QList<ChatImage> images{{"image/png", QByteArray("image bytes")}};
        const auto openai = chatImageContent("inspect", images).toArray();
        QCOMPARE(openai.at(1).toObject().value("image_url").toObject().value("url").toString(), "data:image/png;base64," + QString::fromLatin1(images.first().bytes.toBase64()));
        const auto claude = chatImageContent("inspect", images, true).toArray();
        QCOMPARE(claude.at(1).toObject().value("source").toObject().value("media_type").toString(), "image/png");
        QJsonArray gemini; appendGeminiImageParts(gemini, images);
        QCOMPARE(gemini.first().toObject().value("inlineData").toObject().value("data").toString(), QString::fromLatin1(images.first().bytes.toBase64()));
    }
};
QTEST_GUILESS_MAIN(ChatAttachmentsTest)
#include "test_chat_attachments.moc"
