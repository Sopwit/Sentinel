// SPDX-License-Identifier: GPL-3.0-or-later
// Presentation rules shared by the catalogue view and its regression tests.
function matchesSource(model, source) {
    if (source === "all") return true;
    if (source === "llamacpp") return !!model.gguf;
    if (model.catalogSource) return model.catalogSource === source;
    if (source === "huggingface") return !!model.repositoryId || (model.externalUrl || "").indexOf("https://huggingface.co/") === 0;
    if (source === "lmstudio") return model.provider === "LM Studio";
    return source === "ollama" && !!model.ollamaId && model.provider !== "LM Studio";
}
function matchesCategory(model, category) {
    if (category === "All" || model.category === category) return true;
    return category === "Think" && (model.tags || []).some(function(tag) { return tag.toLowerCase() === "thinking" || tag.toLowerCase() === "reasoning"; });
}
function matchesSearch(model, text) {
    var query = text.trim().toLowerCase();
    if (!query.length) return true;
    var fields = [model.name, model.id, model.ollamaId, model.repositoryId, model.description, (model.tags || []).join(" ")].join(" ").toLowerCase();
    if (fields.indexOf(query) >= 0) return true;
    var normalized = query.replace(/[-_.\s]/g, "");
    return !!model.gguf && normalized.length > 0 &&
        ((model.name || "") + " " + (model.repositoryId || "")).toLowerCase().replace(/[-_.\s]/g, "").indexOf(normalized) >= 0;
}
function knownNumber(value) {
    return typeof value === "number" && isFinite(value) && value >= 0 ? value : null;
}
function sortModels(models, mode, installed) {
    var rows = models.map(function(model, index) {
        var updated = model.lastUpdated ? Date.parse(model.lastUpdated) : NaN;
        return {model: model, index: index, installed: !!installed[index], downloads: knownNumber(model.downloads), size: knownNumber(model.sizeBytes), updated: isFinite(updated) ? updated : null};
    });
    rows.sort(function(a, b) {
        if (mode === "installed") return a.installed !== b.installed ? (a.installed ? -1 : 1) : a.index - b.index;
        var key = mode === "downloads" ? "downloads" : mode === "updated" ? "updated" : mode === "size" ? "size" : "";
        if (key) {
            if ((a[key] === null) !== (b[key] === null)) return a[key] === null ? 1 : -1;
            if (a[key] !== null && a[key] !== b[key]) return mode === "size" ? a[key] - b[key] : b[key] - a[key];
        }
        var name = (a.model.name || "").localeCompare(b.model.name || "");
        if (mode === "nameDesc") name = -name;
        return name || (a.model.id || "").localeCompare(b.model.id || "") || a.index - b.index;
    });
    return rows.map(function(row) { return row.model; });
}
