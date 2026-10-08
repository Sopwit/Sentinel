// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
#include <QStringList>

namespace sentinel::core {
// Task metadata takes precedence over family-name fallbacks.
inline QString modelCategory(const QString& name, const QStringList& capabilities) {
    const auto id = name.toLower();
    const auto tags = capabilities.join(' ').toLower();
    if (tags.contains("text-to-video") || tags.contains("image-to-video"))
        return "Video";
    if (tags.contains("text-to-image") || tags.contains("image-generation"))
        return "Image";
    if (tags.contains("automatic-speech-recognition") || id.contains("whisper"))
        return "STT";
    if (tags.contains("text-to-speech"))
        return "TTS";
    if (tags.contains("feature-extraction") || tags.contains("embedding") || id.contains("embed"))
        return "Embedding";
    if (tags.contains("image-text-to-text") || tags.contains("vision") || id.contains("llava"))
        return "Vision";
    if (tags.contains("thinking") || tags.contains("reasoning") || id.contains("deepseek-r1"))
        return "Think";
    return "LLM";
}
} // namespace sentinel::core
