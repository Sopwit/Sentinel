// SPDX-License-Identifier: GPL-3.0-or-later
#include "DaemonModelHelpers.h"
#include "sentinel/core/network/NetworkPolicyService.h"
#include <QJsonArray>
namespace sentinel::daemon {
QJsonObject DaemonModelHelpers::state(const QString& component) const {
    if (component == "ollamaPuller")
        return {{"pulling", puller.pulling()},
                {"activeModel", puller.activeModel()},
                {"progress", puller.progress()},
                {"statusText", puller.statusText()},
                {"errorText", puller.errorText()}};
    if (component == "ollamaLibraryFetcher")
        return {{"fetching", library.fetching()},
                {"models", QJsonArray::fromVariantList(library.models())},
                {"errorText", library.errorText()}};
    if (component == "ollamaModelDetailFetcher")
        return {{"fetching", detail.fetching()},
                {"readme", detail.readme()},
                {"tags", QJsonArray::fromVariantList(detail.tags())},
                {"installCmd", detail.installCmd()},
                {"errorText", detail.errorText()}};
    if (component == "lmStudioLibraryFetcher")
        return {{"fetching", lmStudio.fetching()},
                {"models", QJsonArray::fromVariantList(lmStudio.models())},
                {"errorText", lmStudio.errorText()}};
    return {};
}
bool DaemonModelHelpers::action(const QString& component, const QString& action,
                                const QString& value, const QString& endpoint) {
    const auto url = component == "ollamaPuller" ? QUrl(endpoint)
                                                 : QUrl(component == "lmStudioLibraryFetcher"
                                                            ? "https://lmstudio.ai/models"
                                                            : "https://ollama.com/library");
    if (action != "cancel" &&
        core::NetworkPolicyService::instance().check(url) != core::NetworkDecision::Allowed)
        return false;
    if (component == "ollamaPuller") {
        puller.setEndpoint(endpoint);
        if (action == "pull" && !value.trimmed().isEmpty())
            puller.pull(value);
        else if (action == "removeModel" && !value.trimmed().isEmpty())
            puller.removeModel(value);
        else if (action == "cancel")
            puller.cancel();
        else
            return false;
    } else if (component == "ollamaLibraryFetcher") {
        if (action == "fetch")
            library.fetch(value);
        else if (action == "cancel")
            library.cancel();
        else
            return false;
    } else if (component == "ollamaModelDetailFetcher") {
        if (action == "fetchDetails" && !value.trimmed().isEmpty())
            detail.fetchDetails(value);
        else if (action == "cancel")
            detail.cancel();
        else
            return false;
    } else if (component == "lmStudioLibraryFetcher") {
        if (action == "fetch")
            lmStudio.fetch();
        else if (action == "cancel")
            lmStudio.cancel();
        else
            return false;
    } else
        return false;
    return true;
}
} // namespace sentinel::daemon
