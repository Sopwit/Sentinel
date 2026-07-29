#include "sentinel/core/DpapiEncryptedSettingsStore.h"

#include <QtGlobal>

#if defined(Q_OS_WIN)
#include <windows.h>
#include <dpapi.h>
#endif

namespace sentinel::core {

namespace {

constexpr QLatin1String encryptedPrefix("$dpapi$");

bool isProbablySecret(const QString& key) {
    const auto lower = key.toLower();
    return lower.contains(QStringLiteral("apikey"))
        || lower.contains(QStringLiteral("secret"))
        || lower.contains(QStringLiteral("token"))
        || lower.contains(QStringLiteral("password"));
}

} // namespace

DpapiEncryptedSettingsStore::DpapiEncryptedSettingsStore(std::unique_ptr<ISettingsStore> inner)
    : inner_(std::move(inner)) {}

bool DpapiEncryptedSettingsStore::isSecretKey(const QString& key) {
    return isProbablySecret(key);
}

QString DpapiEncryptedSettingsStore::value(const QString& key, const QString& defaultValue) const {
    const auto raw = inner_->value(key, QString());
    if (raw.isEmpty()) {
        return defaultValue;
    }

    if (!isSecretKey(key) || !raw.startsWith(encryptedPrefix)) {
        return raw;
    }

    const auto cipherText = QByteArray::fromBase64(raw.mid(encryptedPrefix.size()).toLatin1());
    const auto decrypted = decrypt(cipherText);
    return decrypted.isEmpty() ? defaultValue : decrypted;
}

void DpapiEncryptedSettingsStore::setValue(QString key, QString value) {
    if (isSecretKey(key) && !value.isEmpty()) {
        const auto cipherData = encrypt(value);
        const auto encoded = encryptedPrefix + QString::fromLatin1(cipherData.toBase64());
        inner_->setValue(key, encoded);
    } else {
        inner_->setValue(key, value);
    }
}

#if defined(Q_OS_WIN)

QByteArray DpapiEncryptedSettingsStore::encrypt(const QString& plainText) {
    const QByteArray utf8 = plainText.toUtf8();
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(utf8.constData()));
    in.cbData = static_cast<DWORD>(utf8.size());

    DATA_BLOB out;
    if (!CryptProtectData(&in, L"Sentinel Settings", nullptr, nullptr, nullptr, 0, &out)) {
        return {};
    }

    QByteArray result(reinterpret_cast<const char*>(out.pbData), out.cbData);
    LocalFree(out.pbData);
    return result;
}

QString DpapiEncryptedSettingsStore::decrypt(const QByteArray& cipherData) {
    DATA_BLOB in;
    in.pbData = reinterpret_cast<BYTE*>(const_cast<char*>(cipherData.constData()));
    in.cbData = static_cast<DWORD>(cipherData.size());

    DATA_BLOB out;
    if (!CryptUnprotectData(&in, nullptr, nullptr, nullptr, nullptr, 0, &out)) {
        return {};
    }

    const QByteArray utf8(reinterpret_cast<const char*>(out.pbData), out.cbData);
    LocalFree(out.pbData);
    return QString::fromUtf8(utf8);
}

#else

QByteArray DpapiEncryptedSettingsStore::encrypt(const QString& plainText) {
    Q_UNUSED(plainText)
    return {};
}

QString DpapiEncryptedSettingsStore::decrypt(const QByteArray& cipherData) {
    Q_UNUSED(cipherData)
    return {};
}

#endif

} // namespace sentinel::core
