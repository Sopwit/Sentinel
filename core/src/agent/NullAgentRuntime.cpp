// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/agent/NullAgentRuntime.h"

#include <QRegularExpression>

#include <utility>

namespace sentinel::core {

namespace {

bool isAutomaticWebSearchIntent(const QString& prompt) {
    const auto normalized = prompt.toLower().simplified();
    static const QStringList intentPhrases{
        QStringLiteral("en son"), QStringLiteral("en güncel"), QStringLiteral("en guncel"),
        QStringLiteral("bugün"), QStringLiteral("bugun"), QStringLiteral("son dakika"),
        QStringLiteral("hava durumu"), QStringLiteral("hava nasıl"), QStringLiteral("hava nasil"),
        QStringLiteral("weather"), QStringLiteral("forecast"), QStringLiteral("temperature"),
        QStringLiteral("haber"), QStringLiteral("internette"), QStringLiteral("webde"),
        QStringLiteral("web'de"), QStringLiteral("araştır"), QStringLiteral("arastir"),
        QStringLiteral("online araştır"), QStringLiteral("online arastir"),
        QStringLiteral("latest"), QStringLiteral("current"), QStringLiteral("today"),
        QStringLiteral("breaking news"), QStringLiteral("news"), QStringLiteral("recent"),
        QStringLiteral("research online"), QStringLiteral("search online"),
        QStringLiteral("look up"), QStringLiteral("on the web")};
    for (const auto& phrase : intentPhrases) {
        if (normalized.contains(phrase)) {
            return true;
        }
    }
    return false;
}

} // namespace

namespace {

// Returns a cleaned URL for domain-like targets ("sahibinden.com",
// "www.x.com", "https://x.com", "sahibinden.com'u") or an empty string when
// the text is not a website address. Used to route "X aç" requests to
// open-url instead of app-launch.
QString urlTargetFromName(const QString& text) {
    QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.contains(QLatin1Char(' '))) {
        return QString();
    }

    const QString lowered = trimmed.toLower();
    if (lowered.startsWith(QStringLiteral("http://")) ||
        lowered.startsWith(QStringLiteral("https://")) ||
        lowered.startsWith(QStringLiteral("www."))) {
        return trimmed;
    }

    // Strip a trailing Turkish possessive suffix: "sahibinden.com'u".
    const int apos = trimmed.lastIndexOf(QLatin1Char('\''));
    if (apos > 0) {
        trimmed = trimmed.left(apos);
    }

    const int dot = trimmed.lastIndexOf(QLatin1Char('.'));
    if (dot < 1 || dot >= trimmed.size() - 2) {
        return QString();
    }

    const QString suffix = trimmed.mid(dot + 1);
    if (suffix.size() > 10) {
        return QString();
    }
    for (const QChar c : suffix) {
        if (!c.isLetter()) {
            return QString();
        }
    }

    for (const QChar c : trimmed.left(dot)) {
        if (!c.isLetterOrNumber() && c != QLatin1Char('-') && c != QLatin1Char('.')) {
            return QString();
        }
    }
    return trimmed;
}

} // namespace

NullAgentRuntime::NullAgentRuntime()
    : NullAgentRuntime(QList<ToolDescriptor>{ToolDescriptor{
          QStringLiteral("local-plan-summary"),
          QStringLiteral("Local Plan Summary"),
          QStringLiteral("Local planning summary tool."),
          ToolRiskLevel::Low,
          ToolExecutionMode::MetadataOnly,
          {
              ToolParameterDescriptor{QStringLiteral("topic"),
                                      QStringLiteral("Short user topic for local summary."), true},
          },
      }}) {}

NullAgentRuntime::NullAgentRuntime(QList<ToolDescriptor> tools) {
    for (auto& tool : tools) {
        toolRegistry_.registerTool(std::move(tool));
    }
}

QString NullAgentRuntime::name() const {
    return QStringLiteral("NullAgentRuntime");
}

AgentStatus NullAgentRuntime::status() const {
    return AgentStatus::Ready;
}

QList<AgentCapabilityDescriptor> NullAgentRuntime::capabilities() const {
    return {
        {
            QStringLiteral("local-plan-execution"),
            QStringLiteral("Executes approved local planning tools through the tool gateway."),
            true,
        },
    };
}

QList<ToolDescriptor> NullAgentRuntime::availableTools() const {
    return toolRegistry_.listTools();
}

QList<ToolDescriptor> NullAgentRuntime::standardTools() {
    return {
        ToolDescriptor{QStringLiteral("local-plan-summary"),
                        QStringLiteral("Local Plan Summary"),
                        QStringLiteral("Local planning summary tool."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::MetadataOnly,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("topic"),
                                QStringLiteral("Short user topic for local summary."), true},
                        }},
        ToolDescriptor{QStringLiteral("read-file"),
                        QStringLiteral("Read File"),
                        QStringLiteral(
                            "Reads a text file (or lists a directory) inside the workspace. "
                            "Output has line numbers. Use offset/limit to page through long "
                            "files instead of re-reading the whole file. Handles directories by "
                            "listing entries."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("Absolute or workspace-relative "
                                                                   "file/directory path."),
                                                    true},
                            ToolParameterDescriptor{
                                QStringLiteral("offset"),
                                QStringLiteral("1-based line to start from (default 1)."), false},
                            ToolParameterDescriptor{
                                QStringLiteral("limit"),
                                QStringLiteral("Max lines to return (default 2000)."), false},
                        }},
        ToolDescriptor{QStringLiteral("write-file"),
                        QStringLiteral("Write File"),
                        QStringLiteral(
                            "Writes a full file. Prefer edit-file for small changes to existing "
                            "files."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("Absolute path to write."), true},
                            ToolParameterDescriptor{QStringLiteral("content"),
                                                    QStringLiteral("File contents."), true},
                        }},
        ToolDescriptor{QStringLiteral("edit-file"),
                        QStringLiteral("Edit File"),
                        QStringLiteral(
                            "Edits a file by replacing oldString with newString. Fuzzy matching "
                            "handles minor whitespace/indentation differences, but provide the "
                            "exact text including surrounding context when possible. oldString "
                            "must be unique in the file unless replaceAll is true. Use an empty "
                            "oldString to create a new file."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("File to edit."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("oldString"),
                                QStringLiteral("Text to replace (empty to create a new file)."),
                                true},
                            ToolParameterDescriptor{QStringLiteral("newString"),
                                                    QStringLiteral("Replacement text."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("replaceAll"),
                                QStringLiteral("true to replace every occurrence."), false},
                        }},
        ToolDescriptor{QStringLiteral("grep"),
                        QStringLiteral("Grep"),
                        QStringLiteral(
                            "Searches file contents with a regular expression under a directory "
                            "(default: workspace). Returns up to 100 matches as 'Line N: text'. "
                            "Use this instead of grep/find in run-command."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("pattern"),
                                                    QStringLiteral("Regular expression."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("path"),
                                QStringLiteral("Directory to search (default: workspace)."),
                                false},
                            ToolParameterDescriptor{
                                QStringLiteral("include"),
                                QStringLiteral("Glob filter like *.cpp or *.{ts,tsx}."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("glob"),
                        QStringLiteral("Glob"),
                        QStringLiteral(
                            "Finds files by glob pattern (e.g. *.cpp, src/**/*.h) under a "
                            "directory. Returns up to 100 absolute paths."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("pattern"),
                                                    QStringLiteral("Glob pattern."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("path"),
                                QStringLiteral("Directory to search (default: workspace)."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("delete-file"),
                        QStringLiteral("Delete File"),
                        QStringLiteral(
                            "Deletes a single file inside the workspace. Directories are "
                            "refused. Deletion is permanent; prefer move-file when unsure."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("File to delete."), true},
                        }},
        ToolDescriptor{QStringLiteral("move-file"),
                        QStringLiteral("Move File"),
                        QStringLiteral(
                            "Moves or renames a file inside the workspace. Existing "
                            "destination files are never overwritten."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("source"),
                                                    QStringLiteral("File to move."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("destination"),
                                QStringLiteral("Target path (may use a new file name)."),
                                true},
                        }},
        ToolDescriptor{QStringLiteral("apply-patch"),
                        QStringLiteral("Apply Patch"),
                        QStringLiteral(
                            "Applies a unified diff patch to workspace files. Supports update "
                            "(--- a/file + +++ b/file), add (--- /dev/null), and delete "
                            "(+++ /dev/null) with @@ hunk headers. Prefer this over many "
                            "edit-file calls for multi-file or multi-hunk changes."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("patch"),
                                QStringLiteral("The complete unified diff patch text."),
                                true},
                        }},
        ToolDescriptor{QStringLiteral("list-code-definitions"),
                        QStringLiteral("List Code Definitions"),
                        QStringLiteral(
                            "Lists classes, functions, and other top-level symbols with line "
                            "numbers for a source file (C/C++, Python, JavaScript/"
                            "TypeScript, Rust). Use it to understand file structure before "
                            "reading or editing."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("Source file path."), true},
                        }},
        ToolDescriptor{
            QStringLiteral("run-command"),
            QStringLiteral("Run Command"),
            QStringLiteral(
                "Runs a shell command and returns stdout/stderr. Prefer dedicated tools "
                "(read-file, edit-file, grep, glob) over cat/grep/find. Use the timeout "
                "parameter (milliseconds) for long-running commands; default 60000, max "
                "600000. Use workdir instead of 'cd x && ...'. Set sandbox=docker to run the "
                "command inside an isolated, network-disabled Docker container with the "
                "workspace mounted at /workspace (requires Docker)."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("command"),
                                        QStringLiteral("The shell command to run."), true},
                ToolParameterDescriptor{
                    QStringLiteral("timeout"),
                    QStringLiteral("Timeout in milliseconds (default 60000, max 600000)."),
                    false},
                ToolParameterDescriptor{
                    QStringLiteral("workdir"),
                    QStringLiteral("Working directory for the command."), false},
                ToolParameterDescriptor{
                    QStringLiteral("sandbox"),
                    QStringLiteral("empty for local shell, or 'docker' for container sandbox."),
                    false},
            }},
        ToolDescriptor{QStringLiteral("app-launch"),
                        QStringLiteral("Launch App"),
                        QStringLiteral(
                            "Launches a desktop application by name (e.g. 'Spotify', 'Firefox'). "
                            "Use this for opening apps; not for scripts."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("app"),
                                QStringLiteral("Application name (e.g. Spotify)."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("args"),
                                QStringLiteral("Optional argument to pass."), false},
                        }},
        ToolDescriptor{QStringLiteral("app-quit"),
                        QStringLiteral("Quit App"),
                        QStringLiteral(
                            "Quits a running desktop application by name (e.g. 'Spotify')."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("app"),
                                QStringLiteral("Application name (e.g. Spotify)."), true},
                        }},
        ToolDescriptor{QStringLiteral("open-url"),
                        QStringLiteral("Open URL"),
                        QStringLiteral(
                            "Opens a URL in the user's default web browser (http/https only). "
                            "Use for website requests like 'sahibinden.com aç' — domains are "
                            "websites, not applications. Use web-fetch instead when the page "
                            "content itself is needed."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("url"),
                                                    QStringLiteral("The URL to open."), true},
                        }},
        ToolDescriptor{QStringLiteral("system-notify"),
                        QStringLiteral("System Notify"),
                        QStringLiteral(
                            "Shows an immediate desktop notification. For reminders at a "
                            "future time use set-alarm instead."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("title"),
                                                    QStringLiteral("Notification title."),
                                                    false},
                            ToolParameterDescriptor{QStringLiteral("message"),
                                                    QStringLiteral("Notification body."), true},
                        }},
        ToolDescriptor{QStringLiteral("clipboard-read"),
                        QStringLiteral("Clipboard Read"),
                        QStringLiteral("Reads the current system clipboard text."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("clipboard-write"),
                        QStringLiteral("Clipboard Write"),
                        QStringLiteral("Replaces the system clipboard text."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("text"),
                                                    QStringLiteral("Text to copy."), true},
                        }},
        ToolDescriptor{QStringLiteral("system-info"),
                        QStringLiteral("System Info"),
                        QStringLiteral(
                            "Reports the operating system, kernel, CPU architecture, hostname, "
                            "user, total RAM, and root volume usage. Read-only."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("process-list"),
                        QStringLiteral("Process List"),
                        QStringLiteral(
                            "Lists currently running processes (pid, CPU, name). Read-only; use "
                            "app-quit to close an application."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("current-time"),
                        QStringLiteral("Current Time"),
                        QStringLiteral(
                            "Returns the current local date and time with the timezone, UTC "
                            "time, and epoch seconds."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("set-alarm"),
                        QStringLiteral("Set Alarm"),
                        QStringLiteral(
                            "Schedules a future alarm/reminder. The user is notified in chat "
                            "with a desktop notification when it fires. Time formats: 'HH:mm' "
                            "(next occurrence), 'HH:mm:ss', or ISO 'yyyy-MM-ddTHH:mm'."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("time"),
                                                    QStringLiteral("When the alarm fires."), true},
                            ToolParameterDescriptor{QStringLiteral("label"),
                                                    QStringLiteral("What the alarm is about."),
                                                    true},
                        }},
        ToolDescriptor{QStringLiteral("list-alarms"),
                        QStringLiteral("List Alarms"),
                        QStringLiteral("Lists all active scheduled alarms."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("cancel-alarm"),
                        QStringLiteral("Cancel Alarm"),
                        QStringLiteral(
                            "Cancels an active alarm by its id (see list-alarms for ids)."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("id"),
                                QStringLiteral("The alarm id returned by set-alarm."), true},
                        }},
        ToolDescriptor{QStringLiteral("todo-write"),
                        QStringLiteral("Todo Write"),
                        QStringLiteral(
                            "Records the task checklist for this goal as a JSON array of "
                            "{content, status, priority} objects. status: pending | in_progress "
                            "| completed | cancelled. Use for goals with 3+ steps: create the "
                            "list first, mark exactly one item in_progress at a time, and mark "
                            "completed only after verifying. Always pass the full list (it "
                            "replaces the previous list)."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("todos"),
                                QStringLiteral(
                                    "JSON array, e.g. [{\"content\":\"step\",\"status\":"
                                    "\"pending\",\"priority\":\"high\"}]"),
                                true},
                        }},
        ToolDescriptor{QStringLiteral("todo-read"),
                        QStringLiteral("Todo Read"),
                        QStringLiteral("Reads the current task checklist for this goal."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("memory-search"),
                        QStringLiteral("Memory Search"),
                        QStringLiteral(
                            "Searches Sentinel's long-term memory entries (keys and values) for "
                            "the query text. Use this before asking the user for information "
                            "that may already be remembered."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("query"),
                                                    QStringLiteral("Text to search for."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("limit"),
                                QStringLiteral("Max matches to return (default 10, max 50)."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("history-search"),
                        QStringLiteral("History Search"),
                        QStringLiteral(
                            "Searches earlier messages of this chat history (all roles) for "
                            "the query text. Use it to recall what the user said before."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("query"),
                                                    QStringLiteral("Text to search for."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("limit"),
                                QStringLiteral("Max matches to return (default 10, max 50)."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("ask-question"),
                        QStringLiteral("Ask Question"),
                        QStringLiteral(
                            "Asks the user a single clarifying question when the goal is "
                            "ambiguous. Provide 2-5 short options, one per line, when the "
                            "answer is a choice. After calling this, end the run with the "
                            "question as your final answer and wait for the user's reply."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("question"),
                                                    QStringLiteral("The question to ask."),
                                                    true},
                            ToolParameterDescriptor{
                                QStringLiteral("options"),
                                QStringLiteral("Optional answer choices, one per line."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("mcp-list"),
                        QStringLiteral("MCP List"),
                        QStringLiteral(
                            "Lists configured MCP (Model Context Protocol) servers and the "
                            "tools they expose. Call this first to discover extendable "
                            "capabilities before using mcp-call."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("mcp-call"),
                        QStringLiteral("MCP Call"),
                        QStringLiteral(
                            "Calls a tool on a configured MCP server. Find server and tool "
                            "names with mcp-list first. Arguments must be a JSON object "
                            "matching the tool's schema."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("server"),
                                QStringLiteral("MCP server name from mcp-list."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("tool"),
                                QStringLiteral("Tool name on that server."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("arguments"),
                                QStringLiteral(
                                    "JSON object of tool arguments, e.g. {\"key\": \"value\"}."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("spawn-agent"),
                        QStringLiteral("Spawn Agent"),
                        QStringLiteral(
                            "Delegates a self-contained subtask to a bounded read-only "
                            "subagent and returns its final answer. Use for parallelizable "
                            "research or verification subtasks (e.g. 'check whether X is "
                            "documented anywhere in the workspace'). Subagents cannot spawn "
                            "further subagents or edit files."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{
                                QStringLiteral("task"),
                                QStringLiteral("The complete, self-contained task description."),
                                true},
                        }},
        ToolDescriptor{QStringLiteral("browser-screenshot"),
                        QStringLiteral("Browser Screenshot"),
                        QStringLiteral(
                            "Takes a full-page screenshot of a URL in a headless Chromium "
                            "browser via Playwright and saves it as PNG. Requires Node.js "
                            "(npx) and 'npx playwright install chromium'. Use open-url when "
                            "the user just wants to see the page."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("url"),
                                                    QStringLiteral("The page URL."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("path"),
                                QStringLiteral("Output PNG path (default: temp file)."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("browser-pdf"),
                        QStringLiteral("Browser PDF"),
                        QStringLiteral(
                            "Renders a URL to a PDF document in a headless Chromium browser "
                            "via Playwright. Requires Node.js (npx) and 'npx playwright "
                            "install chromium'."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("url"),
                                                    QStringLiteral("The page URL."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("path"),
                                QStringLiteral("Output PDF path (default: temp file)."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("web-fetch"),
                        QStringLiteral("Web Fetch"),
                        QStringLiteral(
                            "Fetches a single http(s) URL and returns page content (markdown by "
                            "default). Use after web-search to open a result."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("url"),
                                                    QStringLiteral("The URL to fetch."), true},
                            ToolParameterDescriptor{
                                QStringLiteral("format"),
                                QStringLiteral("markdown | text | html (default markdown)."),
                                false},
                        }},
        ToolDescriptor{QStringLiteral("web-search"),
                        QStringLiteral("Web Search"),
                        QStringLiteral(
                            "Web lookup querying the network via local client. Use for current "
                            "news, weather, or anything requiring fresh information."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("query"),
                                                    QStringLiteral("The search query."), true},
                        }},
        ToolDescriptor{QStringLiteral("open-workspace"),
                        QStringLiteral("Open Workspace"),
                        QStringLiteral(
                            "Sets the active workspace directory. File tools (read/write/edit/"
                            "grep/glob) are scoped to this directory. Call this before file "
                            "tools when the goal refers to a specific folder."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("The workspace path."), true},
                        }},
        ToolDescriptor{QStringLiteral("voice-transcribe"),
                        QStringLiteral("Voice Transcribe"),
                        QStringLiteral(
                            "Microphone audio capture and STT speech-to-text pipeline."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("path"),
                                                    QStringLiteral("Path to the audio file."),
                                                    true},
                        }},
        ToolDescriptor{QStringLiteral("voice-speak"),
                        QStringLiteral("Voice Speak"),
                        QStringLiteral("TTS text-to-speech audio synthesis and playback."),
                        ToolRiskLevel::High,
                        ToolExecutionMode::Local,
                        {
                            ToolParameterDescriptor{QStringLiteral("text"),
                                                    QStringLiteral("The text to speak."), true},
                        }},
        ToolDescriptor{QStringLiteral("summarize-current-conversation"),
                        QStringLiteral("Summarize Current Conversation"),
                        QStringLiteral(
                            "Conversation summary generation and prompt injection."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("provider-test-call"),
                        QStringLiteral("Provider Test Call"),
                        QStringLiteral("Connectivity check passed."),
                        ToolRiskLevel::Low,
                        ToolExecutionMode::Local,
                        {}},
        ToolDescriptor{QStringLiteral("export-conversation"),
                        QStringLiteral("Export Conversation"),
                        QStringLiteral("Explicit transcript export to local disk file."),
                        ToolRiskLevel::Medium,
                        ToolExecutionMode::Local,
                        {}}};
}

ToolInvocationPlan NullAgentRuntime::plan(const AgentRequest& request) const {
    const auto trimmed = request.prompt.trimmed();
    if (trimmed.isEmpty()) {
        return {
            ToolInvocationPlanStatus::EmptyRequest,
            QStringLiteral("Agent request was empty."),
            {},
        };
    }

    const auto tools = toolRegistry_.listTools();
    if (tools.isEmpty()) {
        return {
            ToolInvocationPlanStatus::NoToolsAvailable,
            QStringLiteral("No tool metadata is available for planning."),
            {},
        };
    }

    QString selectedToolId;
    QList<ToolInvocationArgument> arguments;

    const auto requestedToolId = request.requestedToolId.trimmed();
    if (!requestedToolId.isEmpty()) {
        const auto tool = toolRegistry_.findToolById(requestedToolId);
        if (!tool.has_value()) {
            return {
                ToolInvocationPlanStatus::UnknownTool,
                QStringLiteral("Requested tool metadata was not found: %1").arg(requestedToolId),
                {},
            };
        }
        selectedToolId = requestedToolId;
        for (const auto& param : tool->parameters) {
            arguments.append(
                ToolInvocationArgument{param.id, param.required ? trimmed : QString()});
        }
    } else {
        auto isLaunchVerb = [](const QString& word) {
            return word == QStringLiteral("aç") || word == QStringLiteral("ac") ||
                   word == QStringLiteral("open") ||
                   word == QStringLiteral("başlat") || word == QStringLiteral("baslat") ||
                   word == QStringLiteral("launch") || word == QStringLiteral("start");
        };
        auto isQuitVerb = [](const QString& word) {
            return word == QStringLiteral("kapat") || word == QStringLiteral("kapat.") ||
                   word == QStringLiteral("quit") || word == QStringLiteral("close") ||
                   word == QStringLiteral("exit") || word == QStringLiteral("sonlandır") ||
                   word == QStringLiteral("durdur");
        };

        QString programName;
        bool wantsQuit = false;
        QStringList words = trimmed.split(QChar(' '), Qt::SkipEmptyParts);
        if (words.size() >= 2) {
            if (isLaunchVerb(words.first().toLower())) {
                programName = words.mid(1).join(QChar(' '));
            } else if (isLaunchVerb(words.last().toLower())) {
                programName = words.mid(0, words.size() - 1).join(QChar(' '));
            } else if (isQuitVerb(words.first().toLower())) {
                programName = words.mid(1).join(QChar(' '));
                wantsQuit = true;
            } else if (isQuitVerb(words.last().toLower())) {
                programName = words.mid(0, words.size() - 1).join(QChar(' '));
                wantsQuit = true;
            }
        }

        auto stripSuffix = [](QString& name) {
            if (name.endsWith(QChar('\'')) || name.endsWith(QStringLiteral("'ı")) ||
                name.endsWith(QStringLiteral("'i")) ||
                name.endsWith(QStringLiteral("'u")) ||
                name.endsWith(QStringLiteral("'ü")) ||
                name.endsWith(QChar('.')) || name.endsWith(QChar('!'))) {
                int index = name.indexOf(QChar('\''));
                if (index > 0) {
                    name = name.left(index);
                }
                while (!name.isEmpty() && !name.at(name.size() - 1).isLetterOrNumber()) {
                    name.chop(1);
                }
            }
            if (!name.isEmpty() && name.at(0).isLower()) {
                name[0] = name[0].toUpper();
            }
        };

        QString urlTarget;
        if (!programName.isEmpty()) {
            // 'open -a X' is an explicit shell pattern; leave it to run-command.
            if (programName.startsWith(QLatin1Char('-')) ||
                trimmed.contains(QStringLiteral("open -a"))) {
                programName.clear();
            } else {
                // Website targets keep their original form; app names get the
                // Turkish suffix stripping and capitalization treatment.
                urlTarget = urlTargetFromName(programName);
                if (urlTarget.isEmpty()) {
                    stripSuffix(programName);
                }
            }
        }

        const bool hasAppTools =
            toolRegistry_.findToolById(QStringLiteral("app-quit")).has_value();

        if (!programName.isEmpty() && wantsQuit && hasAppTools) {
            selectedToolId = QStringLiteral("app-quit");
            arguments.append(
                ToolInvocationArgument{QStringLiteral("app"), programName});
        } else if (!urlTarget.isEmpty() && !wantsQuit &&
                   toolRegistry_.findToolById(QStringLiteral("open-url")).has_value()) {
            // Website requests like 'sahibinden.com aç' open in the browser,
            // never as desktop applications.
            selectedToolId = QStringLiteral("open-url");
            arguments.append(ToolInvocationArgument{QStringLiteral("url"), urlTarget});
        } else if (!programName.isEmpty() && !wantsQuit &&
                   toolRegistry_.findToolById(QStringLiteral("app-launch")).has_value()) {
            selectedToolId = QStringLiteral("app-launch");
            arguments.append(
                ToolInvocationArgument{QStringLiteral("app"), programName});
        } else if (!programName.isEmpty() && !wantsQuit) {
            selectedToolId = QStringLiteral("run-command");
            arguments.append(ToolInvocationArgument{
                QStringLiteral("command"), QStringLiteral("open -a \"%1\"").arg(programName)});
        } else if ((trimmed.contains(QStringLiteral("alarm")) ||
                    trimmed.contains(QStringLiteral("hatırlat")) ||
                    trimmed.contains(QStringLiteral("hatirlat")) ||
                    trimmed.contains(QStringLiteral("reminder")) ||
                    trimmed.contains(QStringLiteral("remind me")) ||
                    trimmed.contains(QStringLiteral("uyandır")) ||
                    trimmed.contains(QStringLiteral("uyandir"))) &&
                   toolRegistry_.findToolById(QStringLiteral("set-alarm")).has_value()) {
            static const QRegularExpression timePattern(
                QStringLiteral("\\b(\\d{1,2}):(\\d{2})\\b"));
            const auto timeMatch = timePattern.match(trimmed);
            if (timeMatch.hasMatch()) {
                selectedToolId = QStringLiteral("set-alarm");
                arguments.append(ToolInvocationArgument{QStringLiteral("time"),
                                                        timeMatch.captured(0)});
                arguments.append(
                    ToolInvocationArgument{QStringLiteral("label"), trimmed});
            } else {
                selectedToolId = QStringLiteral("set-alarm");
                arguments.append(ToolInvocationArgument{QStringLiteral("time"), trimmed});
                arguments.append(ToolInvocationArgument{QStringLiteral("label"), trimmed});
            }
        } else if ((trimmed.startsWith(QStringLiteral("bildir ")) ||
                    trimmed.startsWith(QStringLiteral("notify ")) ||
                    trimmed.contains(QStringLiteral("bildirim göster")) ||
                    trimmed.contains(QStringLiteral("notification göster"))) &&
                   toolRegistry_.findToolById(QStringLiteral("system-notify")).has_value()) {
            selectedToolId = QStringLiteral("system-notify");
            QString message = trimmed;
            for (const auto& prefix : {QStringLiteral("bildir "), QStringLiteral("notify "),
                                       QStringLiteral("bildirim göster"),
                                       QStringLiteral("notification göster")}) {
                if (message.startsWith(prefix)) {
                    message = message.mid(prefix.size()).trimmed();
                    break;
                }
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("title"),
                                                    QStringLiteral("Sentinel")});
            arguments.append(ToolInvocationArgument{QStringLiteral("message"), message});
        } else if (trimmed.startsWith(QStringLiteral("run ")) ||
                   trimmed.startsWith(QStringLiteral("cmd ")) ||
                   trimmed.contains(QStringLiteral("open -a")) ||
                   trimmed.startsWith(QStringLiteral("ls ")) ||
                   trimmed.startsWith(QStringLiteral("pwd")) ||
                   trimmed.startsWith(QStringLiteral("echo ")) ||
                   trimmed.startsWith(QStringLiteral("open ")) ||
                   trimmed.startsWith(QStringLiteral("./")) ||
                   trimmed.startsWith(QStringLiteral("/")) ||
                   trimmed.startsWith(QStringLiteral("git ")) ||
                   trimmed.startsWith(QStringLiteral("brew ")) ||
                   trimmed.startsWith(QStringLiteral("python ")) ||
                   trimmed.startsWith(QStringLiteral("node ")) ||
                   trimmed.startsWith(QStringLiteral("npm "))) {

            selectedToolId = QStringLiteral("run-command");
            QString command = trimmed;
            if (command.startsWith(QStringLiteral("run "))) {
                command = command.mid(4).trimmed();
            } else if (command.startsWith(QStringLiteral("cmd "))) {
                command = command.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("command"), command});
        } else if (trimmed.startsWith(QStringLiteral("read ")) ||
                   trimmed.startsWith(QStringLiteral("cat "))) {
            selectedToolId = QStringLiteral("read-file");
            QString path = trimmed;
            if (path.startsWith(QStringLiteral("read "))) {
                path = path.mid(5).trimmed();
            } else if (path.startsWith(QStringLiteral("cat "))) {
                path = path.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("path"), path});
        } else if (trimmed.startsWith(QStringLiteral("search ")) ||
                   trimmed.startsWith(QStringLiteral("web "))) {
            selectedToolId = QStringLiteral("web-search");
            QString query = trimmed;
            if (query.startsWith(QStringLiteral("search "))) {
                query = query.mid(7).trimmed();
            } else if (query.startsWith(QStringLiteral("web "))) {
                query = query.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("query"), query});
        } else if (isAutomaticWebSearchIntent(trimmed) &&
                   toolRegistry_.findToolById(QStringLiteral("web-search")).has_value()) {
            selectedToolId = QStringLiteral("web-search");
            arguments.append(ToolInvocationArgument{QStringLiteral("query"), trimmed});
        } else if (trimmed.startsWith(QStringLiteral("speak ")) ||
                   trimmed.startsWith(QStringLiteral("say "))) {
            selectedToolId = QStringLiteral("voice-speak");
            QString text = trimmed;
            if (text.startsWith(QStringLiteral("speak "))) {
                text = text.mid(6).trimmed();
            } else if (text.startsWith(QStringLiteral("say "))) {
                text = text.mid(4).trimmed();
            }
            arguments.append(ToolInvocationArgument{QStringLiteral("text"), text});
        } else {
            if (toolRegistry_.findToolById(QStringLiteral("run-command")).has_value()) {
                selectedToolId = QStringLiteral("run-command");
                arguments.append(ToolInvocationArgument{QStringLiteral("command"), trimmed});
            } else {
                selectedToolId = QStringLiteral("local-plan-summary");
                arguments.append(ToolInvocationArgument{QStringLiteral("topic"), trimmed});
            }
        }
    }

    const auto toolOpt = toolRegistry_.findToolById(selectedToolId);
    if (!toolOpt.has_value()) {
        auto allTools = toolRegistry_.listTools();
        if (!allTools.isEmpty()) {
            QList<PlannedToolInvocation> invocations;
            for (const auto& tool : allTools) {
                QList<ToolInvocationArgument> argumentsList;
                for (const auto& parameter : tool.parameters) {
                    argumentsList.append(ToolInvocationArgument{
                        parameter.id,
                        parameter.required ? trimmed : QString(),
                    });
                }
                invocations.append(PlannedToolInvocation{
                    tool.id,
                    tool.name,
                    QStringLiteral("Plan metadata for %1").arg(tool.name),
                    QStringLiteral("Dynamic tool plan for: %1").arg(trimmed),
                    tool.riskLevel,
                    tool.executionMode,
                    argumentsList,
                });
            }
            return {
                ToolInvocationPlanStatus::Planned,
                QStringLiteral("Tool plan prepared: %1").arg(allTools.first().name),
                invocations,
            };
        }

        return {
            ToolInvocationPlanStatus::UnknownTool,
            QStringLiteral("Planned tool was not found in registry: %1").arg(selectedToolId),
            {},
        };
    }

    const auto& tool = *toolOpt;
    QList<PlannedToolInvocation> invocations;
    invocations.append(PlannedToolInvocation{
        tool.id,
        tool.name,
        QStringLiteral("Plan metadata for %1").arg(tool.name),
        QStringLiteral("Dynamic tool plan for: %1").arg(trimmed),
        tool.riskLevel,
        tool.executionMode,
        arguments,
    });

    return {
        ToolInvocationPlanStatus::Planned,
        QStringLiteral("Tool plan prepared: %1").arg(tool.name),
        invocations,
    };
}

AgentResponse NullAgentRuntime::execute(const AgentRequest& request) {
    const auto trimmed = request.prompt.trimmed();
    if (trimmed.isEmpty()) {
        return {
            false,
            QStringLiteral("Agent request was empty."),
            AgentStatus::Ready,
        };
    }

    const auto plan = this->plan(request);
    if (plan.status != ToolInvocationPlanStatus::Planned) {
        return {
            false,
            plan.summary,
            AgentStatus::Ready,
        };
    }

    QStringList knownToolIds;
    for (const auto& tool : toolRegistry_.listTools()) {
        knownToolIds.append(tool.id);
    }

    const auto result = executor_.execute(ToolExecutionRequest{
        plan,
        ApprovalDecision{
            ApprovalStatus::Approved,
            QStringLiteral("Runtime execution approved: approval and sandbox policies are enforced "
                           "by the caller pipeline."),
            {},
        },
        SandboxEvaluationResult{
            SandboxStatus::Allowed,
            QStringLiteral("Runtime execution allowed: sandbox policy is enforced by the caller "
                           "pipeline."),
            {},
        },
        knownToolIds,
    });

    if (result.status == ToolExecutionStatus::Succeeded) {
        return {
            true,
            result.summary,
            AgentStatus::Ready,
        };
    }
    return {
        false,
        result.summary,
        AgentStatus::Ready,
    };
}

} // namespace sentinel::core
