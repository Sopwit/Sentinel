#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>

namespace Sentinel {

class FnvHash {
public:
    static constexpr uint32_t FNV1A_32_BASE = 2166136261u;

    static uint32_t fnv1a32(const QByteArray &data) {
        uint32_t hash = FNV1A_32_BASE;
        for (char c : data) {
            hash ^= static_cast<uint32_t>(static_cast<unsigned char>(c));
            hash *= 16777619u;
        }
        return hash;
    }

    static uint32_t fnv1a32(const QString &text) {
        return fnv1a32(text.toUtf8());
    }

    static QString fnv1a32Hex(const QByteArray &data) {
        return QString::number(fnv1a32(data), 16);
    }

    static QString fnv1a32Base36(const QByteArray &data) {
        uint32_t hash = fnv1a32(data);
        return toBase36(hash);
    }

    static uint32_t fnv1a32Combine(uint32_t h1, uint32_t h2) {
        uint32_t hash = h1;
        hash ^= h2;
        hash *= 16777619u;
        return hash;
    }

private:
    static QString toBase36(uint32_t value) {
        static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";
        if (value == 0)
            return "0";
        QString result;
        while (value > 0) {
            result.prepend(digits[value % 36]);
            value /= 36;
        }
        return result;
    }
};

} // namespace Sentinel
