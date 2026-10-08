// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <QString>
#include <QStringList>

namespace sentinel::core {
// Task metadata takes precedence over family-name fallbacks.
inline QString modelCategory(const QString& name, const QStringList& capabilities,
                             const QString& pipelineTask = {}) {
    const auto task = pipelineTask.toLower();
    if (!task.isEmpty()) {
        if (task == "text-to-video" || task == "image-to-video" || task == "video-to-video")
            return "Video";
        if (task == "text-to-image" || task == "image-to-image")
            return "Image";
        if (task == "automatic-speech-recognition")
            return "STT";
        if (task == "text-to-speech")
            return "TTS";
        if (task == "feature-extraction" || task == "sentence-similarity")
            return "Embedding";
        if (task == "image-text-to-text" || task == "object-detection" ||
            task == "image-classification" || task == "image-segmentation" ||
            task == "visual-question-answering")
            return "Vision";
        if (task != "text-generation" && task != "text2text-generation")
            return "Other";
        // A declared text-generation task may still advertise reasoning capabilities.
        const auto reasoning = capabilities.join(' ').toLower();
        return reasoning.contains("thinking") || reasoning.contains("reasoning") ||
                       name.toLower().contains("deepseek-r1")
                   ? "Think"
                   : "LLM";
    }
    const auto id = name.toLower();
    const auto tags = capabilities.join(' ').toLower();
    if (tags.contains("text-to-video") || tags.contains("image-to-video") ||
        tags.contains("video-to-video"))
        return "Video";
    if (tags.contains("text-to-image") || tags.contains("image-to-image") ||
        tags.contains("image-generation"))
        return "Image";
    if (tags.contains("automatic-speech-recognition") || id.contains("whisper"))
        return "STT";
    if (tags.contains("text-to-speech"))
        return "TTS";
    if (tags.contains("feature-extraction") || tags.contains("sentence-similarity") ||
        tags.contains("embedding") || id.contains("embed"))
        return "Embedding";
    if (tags.contains("image-text-to-text") || tags.contains("object-detection") ||
        tags.contains("image-classification") || tags.contains("image-segmentation") ||
        tags.contains("visual-question-answering") || tags.contains("vision") ||
        id.contains("llava"))
        return "Vision";
    if (tags.contains("thinking") || tags.contains("reasoning") || id.contains("deepseek-r1"))
        return "Think";
    return "LLM";
}
} // namespace sentinel::core
