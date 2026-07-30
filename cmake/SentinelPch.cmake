# cmake/SentinelPch.cmake
# Precompiled Headers (PCH) configuration for Sentinel targets to accelerate build performance

function(sentinel_enable_pch target_name)
    if(NOT TARGET ${target_name})
        return()
    endif()

    target_precompile_headers(${target_name} PRIVATE
        <vector>
        <string>
        <memory>
        <unordered_map>
        <optional>
        <variant>
        <functional>
        <utility>
        <QString>
        <QObject>
        <QList>
        <QVariant>
        <QJsonObject>
        <QJsonArray>
        <QJsonDocument>
        <QDateTime>
        <QUrl>
        <QDebug>
        <QPointer>
    )
endfunction()
