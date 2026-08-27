#pragma once

#include "sentinel/core/util/FnvHash.h"
#include <QByteArray>
#include <QString>

namespace Sentinel {

class SampledChecksum {
public:
    static constexpr int SAMPLE_SIZE = 4096;
    static constexpr int NUM_SAMPLES = 5;

    struct ChecksumResult {
        uint32_t hash;
        qint64 fileSize;
        int sampleCount;
    };

    static ChecksumResult compute(const QByteArray& data) {
        qint64 size = data.size();
        if (size == 0)
            return {FnvHash::fnv1a32(QByteArray("")), 0, 0};

        if (size <= SAMPLE_SIZE * NUM_SAMPLES) {
            return {FnvHash::fnv1a32(data), size, 1};
        }

        uint32_t combined = FnvHash::FNV1A_32_BASE;
        int samplesFound = 0;

        auto sampleAt = [&](qint64 offset) -> QByteArray {
            qint64 end = qMin(offset + SAMPLE_SIZE, size);
            return data.mid(static_cast<int>(offset), static_cast<int>(end - offset));
        };

        uint32_t h1 = FnvHash::fnv1a32(sampleAt(0));
        combined = FnvHash::fnv1a32Combine(combined, h1);
        samplesFound++;

        uint32_t h2 = FnvHash::fnv1a32(sampleAt(size / 4));
        combined = FnvHash::fnv1a32Combine(combined, h2);
        samplesFound++;

        uint32_t h3 = FnvHash::fnv1a32(sampleAt(size / 2));
        combined = FnvHash::fnv1a32Combine(combined, h3);
        samplesFound++;

        uint32_t h4 = FnvHash::fnv1a32(sampleAt((size * 3) / 4));
        combined = FnvHash::fnv1a32Combine(combined, h4);
        samplesFound++;

        uint32_t h5 = FnvHash::fnv1a32(sampleAt(size - SAMPLE_SIZE));
        combined = FnvHash::fnv1a32Combine(combined, h5);
        samplesFound++;

        uint32_t sizeHash = FnvHash::fnv1a32(QByteArray::number(size));
        combined = FnvHash::fnv1a32Combine(combined, sizeHash);

        return {combined, size, samplesFound};
    }

    static uint32_t computeHash(const QByteArray& data) {
        return compute(data).hash;
    }

    static bool hasChanged(const QByteArray& oldData, const QByteArray& newData) {
        return computeHash(oldData) != computeHash(newData);
    }
};

} // namespace Sentinel
