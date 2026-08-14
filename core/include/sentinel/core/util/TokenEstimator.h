#pragma once

#include <QString>
#include <cmath>

namespace Sentinel {

class TokenEstimator {
public:
    static int estimateTokenCount(const QString &text) {
        if (text.isEmpty())
            return 0;
        return std::max(0, static_cast<int>(std::round(text.length() / 4.0)));
    }

    static double estimateTokensPerChar() {
        return 0.25;
    }

    static int estimateFromChars(int charCount) {
        return std::max(0, static_cast<int>(std::round(charCount / 4.0)));
    }

    static int estimateFromBytes(int byteCount, const QByteArray &data = QByteArray()) {
        if (data.isEmpty())
            return std::max(0, byteCount / 4);
        return estimateTokenCount(QString::fromUtf8(data));
    }
};

} // namespace Sentinel
