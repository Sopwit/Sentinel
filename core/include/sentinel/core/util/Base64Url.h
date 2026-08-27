#pragma once

#include <QByteArray>

namespace sentinel::core {

class Base64Url final {
public:
    static QByteArray encode(const QByteArray& input) {
        QByteArray result =
            input.toBase64(QByteArray::Base64UrlEncoding | QByteArray::OmitTrailingEquals);
        return result;
    }

    static QByteArray decode(const QByteArray& input) {
        return QByteArray::fromBase64(input, QByteArray::Base64UrlEncoding);
    }
};

} // namespace sentinel::core
