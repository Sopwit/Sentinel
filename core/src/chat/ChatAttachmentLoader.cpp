// SPDX-License-Identifier: GPL-3.0-or-later
#include "sentinel/core/chat/ChatAttachmentLoader.h"
#include "sentinel/core/runtime/IFileSystemService.h"
#include "sentinel/core/runtime/ProcessExecutor.h"
#include <QBuffer>
#include <QDir>
#include <QEventLoop>
#include <QFileInfo>
#include <QImageReader>
#include <QMimeDatabase>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QUrl>
#include <QTemporaryDir>
#include <QXmlStreamReader>
#ifdef SENTINEL_HAS_QT_PDF
#include <QPdfDocument>
#endif
namespace sentinel::core {
namespace {
QByteArray docxXml(const QString& path) {
    const auto unzip = QStandardPaths::findExecutable(QStringLiteral("unzip"));
    if (unzip.isEmpty()) return {};
    ProcessExecutor executor;
    ProcessRequest request;
    request.program = unzip;
    request.arguments = {QStringLiteral("-p"), path, QStringLiteral("word/document.xml")};
    request.workingDirectory = QFileInfo(path).absolutePath();
    request.timeoutMs = 5000;
    request.unconfinedPermitted = true;
    QEventLoop loop;
    QByteArray output;
    bool done = false, success = false, exceeded = false;
    const auto id = executor.start(request, [&](const ProcessRecord& record) {
        if (record.state == ProcessState::Exited || record.state == ProcessState::Failed || record.state == ProcessState::Cancelled) {
            done = true;
            success = record.state == ProcessState::Exited && record.exitCode == 0 && !record.timedOut && !exceeded;
            loop.quit();
        }
    }, [&](const QString& id, ProcessStream stream, const QByteArray& bytes) {
        if (stream != ProcessStream::Stdout) return;
        if (output.size() + bytes.size() > 4 * 1024 * 1024) {
            exceeded = true;
            executor.kill(id);
        } else output += bytes;
    });
    if (!done && !id.isEmpty()) loop.exec();
    return success ? output : QByteArray{};
}
}
ChatAttachmentLoadResult loadChatAttachments(const QStringList& paths) {
    ChatAttachmentLoadResult result;
    if (paths.size() > 4) { result.error = QStringLiteral("Attach at most four files per message."); return result; }
    QtFileSystemService files;
    for (const auto& raw : paths) {
        const QUrl url(raw);
        const auto path = url.isLocalFile() ? url.toLocalFile() : raw;
        const QFileInfo info(path);
        const auto authorized = files.resolve(info.absoluteFilePath(), info.absolutePath(), FileSystemAccess::Read);
        if (!authorized.ok()) { result.error = QStringLiteral("Cannot read attachment: %1").arg(info.fileName()); return result; }
        const auto read = files.readFile(*authorized.value, 4 * 1024 * 1024);
        if (!read.ok() || !read.value->complete || read.value->truncated) {
            result.error = QStringLiteral("Attachment is unreadable or exceeds 4 MB: %1").arg(info.fileName()); return result;
        }
        ChatAttachment attachment;
        attachment.fileName = info.fileName();
        attachment.localReference = authorized.value->canonicalPath;
        attachment.sizeBytes = read.value->content.size();
        attachment.mimeType = QMimeDatabase().mimeTypeForFile(info.fileName(), QMimeDatabase::MatchExtension).name();
        const auto suffix = info.suffix().toLower();
        QBuffer source;
        source.setData(read.value->content);
        source.open(QIODevice::ReadOnly);
        if (attachment.mimeType.startsWith(QStringLiteral("image/"))) {
            QImageReader reader(&source);
            const auto size = reader.size();
            if (!size.isValid() || size.width() > 8192 || size.height() > 8192 || qint64(size.width()) * size.height() > 16777216) {
                result.error = QStringLiteral("Invalid or oversized image: %1").arg(info.fileName()); return result;
            }
            reader.setScaledSize(size.scaled(1536, 1536, Qt::KeepAspectRatio));
            const auto image = reader.read();
            QBuffer encoded(&attachment.imageBytes);
            encoded.open(QIODevice::WriteOnly);
            if (image.isNull() || !image.save(&encoded, "PNG") || attachment.imageBytes.size() > 4 * 1024 * 1024) {
                result.error = QStringLiteral("Cannot decode image: %1").arg(info.fileName()); return result;
            }
            attachment.mimeType = QStringLiteral("image/png");
        } else if (suffix == QLatin1String("pdf")) {
#ifdef SENTINEL_HAS_QT_PDF
            QPdfDocument pdf;
            pdf.load(&source);
            if (pdf.error() != QPdfDocument::Error::None || pdf.pageCount() > 300) {
                result.error = QStringLiteral("PDF is encrypted, invalid or too long: %1").arg(info.fileName()); return result;
            }
            for (int i = 0; i < pdf.pageCount() && attachment.text.size() < 24000; ++i)
                attachment.text += pdf.getAllText(i).text() + QLatin1Char('\n');
#else
            result.error = QStringLiteral("This build needs Qt PDF to read PDF attachments. Use a text export."); return result;
#endif
        } else if (suffix == QLatin1String("docx")) {
            const auto revalidated = files.revalidateAuthorized(*authorized.value, info.absolutePath());
            QTemporaryDir snapshot;
            const auto snapshotPath = files.resolve(snapshot.filePath("document.docx"), snapshot.path(), FileSystemAccess::Write);
            const bool copied = revalidated.ok() && snapshot.isValid() && snapshotPath.ok() && files.writeFile(*snapshotPath.value, read.value->content).ok();
            const auto xml = copied ? docxXml(snapshotPath.value->canonicalPath) : QByteArray{};
            QXmlStreamReader reader(xml);
            while (!reader.atEnd()) {
                reader.readNext();
                if (reader.isStartElement() && reader.name() == QLatin1String("t")) attachment.text += reader.readElementText();
                else if (reader.isEndElement() && reader.name() == QLatin1String("p")) attachment.text += QLatin1Char('\n');
                if (attachment.text.size() > 24000) break;
            }
            if (xml.isEmpty() || reader.hasError()) { result.error = QStringLiteral("Cannot extract DOCX; install unzip or attach a text export: %1").arg(info.fileName()); return result; }
        } else {
            QStringDecoder decoder(QStringDecoder::Utf8);
            attachment.text = decoder(read.value->content);
            if (decoder.hasError() || attachment.text.contains(QChar::Null)) { result.error = QStringLiteral("Attachment is not readable UTF-8 text: %1").arg(info.fileName()); return result; }
        }
        if (attachment.imageBytes.isEmpty() && attachment.text.trimmed().isEmpty()) {
            result.error = QStringLiteral("No readable text in %1. Scanned PDFs require OCR.").arg(info.fileName()); return result;
        }
        if (attachment.text.size() > 24000) attachment.text = attachment.text.left(24000) + QStringLiteral("\n[Attachment truncated at 24000 characters]");
        attachment.state = ChatAttachmentState::Ready;
        result.attachments.append(attachment);
    }
    return result;
}
}
