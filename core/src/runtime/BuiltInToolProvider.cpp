// SPDX-FileCopyrightText: 2026 Sopwit <sopwith.osdev@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "sentinel/core/runtime/BuiltInToolProvider.h"
#include <QDir>
#include <QHash>
#include <QJsonArray>
#include <QJsonObject>
#include <QSet>
namespace sentinel::core {
namespace {
QJsonObject builtInSchema(const ToolDescriptor& tool) {
    QJsonObject properties;
    QJsonArray required;
    for (const auto& parameter : tool.parameters) {
        QJsonObject field{{QStringLiteral("type"), QStringLiteral("string")},
                          {QStringLiteral("description"), parameter.description}};
        if (parameter.required) {
            required.append(parameter.id);
            if (parameter.id != QLatin1String("content") &&
                parameter.id != QLatin1String("oldString") &&
                parameter.id != QLatin1String("newString"))
                field.insert(QStringLiteral("minLength"), 1);
        }
        if (parameter.id == QLatin1String("includeHidden") ||
            parameter.id == QLatin1String("replaceAll")) {
            field.insert(QStringLiteral("type"), QStringLiteral("boolean"));
            field.insert(QStringLiteral("default"), false);
            field.remove(QStringLiteral("minLength"));
        } else if (parameter.id == QLatin1String("timeout") ||
                   parameter.id == QLatin1String("offset") ||
                   parameter.id == QLatin1String("limit")) {
            field.insert(QStringLiteral("type"), QStringLiteral("integer"));
            field.insert(QStringLiteral("minimum"), 1);
            field.remove(QStringLiteral("minLength"));
            if (parameter.id == QLatin1String("timeout")) {
                field.insert(QStringLiteral("minimum"), 1000);
                field.insert(QStringLiteral("maximum"), 600000);
                field.insert(QStringLiteral("default"), 60000);
            } else if (parameter.id == QLatin1String("offset"))
                field.insert(QStringLiteral("default"), 1);
            else if (tool.id == QLatin1String("memory-search") ||
                     tool.id == QLatin1String("history-search")) {
                field.insert(QStringLiteral("maximum"), 50);
                field.insert(QStringLiteral("default"), 10);
            } else
                field.insert(QStringLiteral("default"), 2000);
        } else if (tool.id == QLatin1String("todo-write") &&
                   parameter.id == QLatin1String("todos")) {
            field = QJsonObject{
                {QStringLiteral("type"), QStringLiteral("array")},
                {QStringLiteral("items"),
                 QJsonObject{
                     {QStringLiteral("type"), QStringLiteral("object")},
                     {QStringLiteral("properties"),
                      QJsonObject{
                          {QStringLiteral("content"),
                           QJsonObject{{QStringLiteral("type"), QStringLiteral("string")}}},
                          {QStringLiteral("status"),
                           QJsonObject{
                               {QStringLiteral("type"), QStringLiteral("string")},
                               {QStringLiteral("enum"),
                                QJsonArray{QStringLiteral("pending"), QStringLiteral("in_progress"),
                                           QStringLiteral("completed"),
                                           QStringLiteral("cancelled")}}}},
                          {QStringLiteral("priority"),
                           QJsonObject{{QStringLiteral("type"), QStringLiteral("string")},
                                       {QStringLiteral("default"), QStringLiteral("medium")}}}}},
                     {QStringLiteral("required"),
                      QJsonArray{QStringLiteral("content"), QStringLiteral("status")}},
                     {QStringLiteral("additionalProperties"), false}}}};
        } else if (tool.id == QLatin1String("mcp-call") &&
                   parameter.id == QLatin1String("arguments")) {
            field = QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                                {QStringLiteral("default"), QJsonObject{}}};
        } else if (tool.id == QLatin1String("run-command") &&
                   parameter.id == QLatin1String("sandbox")) {
            field.insert(QStringLiteral("enum"),
                         QJsonArray{QStringLiteral(""), QStringLiteral("docker")});
        }
        properties.insert(parameter.id, field);
    }
    return QJsonObject{{QStringLiteral("type"), QStringLiteral("object")},
                       {QStringLiteral("properties"), properties},
                       {QStringLiteral("required"), required},
                       {QStringLiteral("additionalProperties"), false}};
}
} // namespace
QList<ToolDescriptor> BuiltInToolProvider::descriptors() {
    QList<ToolDescriptor> tools = {
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
        ToolDescriptor{
            QStringLiteral("list-directory"),
            QStringLiteral("List Directory"),
            QStringLiteral("Lists files and folders at a path. Use for directory contents."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {ToolParameterDescriptor{QStringLiteral("path"),
                                     QStringLiteral("Directory path, including ~/Desktop."), true},
             ToolParameterDescriptor{QStringLiteral("includeHidden"),
                                     QStringLiteral("Include hidden entries (true/false)."),
                                     false}}},
        ToolDescriptor{
            QStringLiteral("read-file"),
            QStringLiteral("Read File"),
            QStringLiteral("Reads a text file (or lists a directory) inside the workspace. "
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
                ToolParameterDescriptor{QStringLiteral("offset"),
                                        QStringLiteral("1-based line to start from (default 1)."),
                                        false},
                ToolParameterDescriptor{QStringLiteral("limit"),
                                        QStringLiteral("Max lines to return (default 2000)."),
                                        false},
            }},
        ToolDescriptor{
            QStringLiteral("write-file"),
            QStringLiteral("Write File"),
            QStringLiteral("Writes a full file. Prefer edit-file for small changes to existing "
                           "files."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("path"),
                                        QStringLiteral("Absolute path to write."), true},
                ToolParameterDescriptor{QStringLiteral("content"), QStringLiteral("File contents."),
                                        true},
            }},
        ToolDescriptor{
            QStringLiteral("edit-file"),
            QStringLiteral("Edit File"),
            QStringLiteral("Edits a file by replacing oldString with newString. Fuzzy matching "
                           "handles minor whitespace/indentation differences, but provide the "
                           "exact text including surrounding context when possible. oldString "
                           "must be unique in the file unless replaceAll is true. Use an empty "
                           "oldString to create a new file."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("File to edit."),
                                        true},
                ToolParameterDescriptor{
                    QStringLiteral("oldString"),
                    QStringLiteral("Text to replace (empty to create a new file)."), true},
                ToolParameterDescriptor{QStringLiteral("newString"),
                                        QStringLiteral("Replacement text."), true},
                ToolParameterDescriptor{QStringLiteral("replaceAll"),
                                        QStringLiteral("true to replace every occurrence."), false},
            }},
        ToolDescriptor{
            QStringLiteral("grep"),
            QStringLiteral("Grep"),
            QStringLiteral("Searches file contents with a regular expression under a directory "
                           "(default: workspace). Returns up to 100 matches as 'Line N: text'. "
                           "Use this instead of grep/find in run-command."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("pattern"),
                                        QStringLiteral("Regular expression."), true},
                ToolParameterDescriptor{QStringLiteral("path"),
                                        QStringLiteral("Directory to search (default: workspace)."),
                                        false},
                ToolParameterDescriptor{QStringLiteral("include"),
                                        QStringLiteral("Glob filter like *.cpp or *.{ts,tsx}."),
                                        false},
            }},
        ToolDescriptor{
            QStringLiteral("glob"),
            QStringLiteral("Glob"),
            QStringLiteral("Finds files by glob pattern (e.g. *.cpp, src/**/*.h) under a "
                           "directory. Returns up to 100 absolute paths."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("pattern"), QStringLiteral("Glob pattern."),
                                        true},
                ToolParameterDescriptor{QStringLiteral("path"),
                                        QStringLiteral("Directory to search (default: workspace)."),
                                        false},
            }},
        ToolDescriptor{
            QStringLiteral("delete-file"),
            QStringLiteral("Delete File"),
            QStringLiteral("Deletes a single file inside the workspace. Directories are "
                           "refused. Deletion is permanent; prefer move-file when unsure."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("File to delete."),
                                        true},
            }},
        ToolDescriptor{QStringLiteral("move-file"),
                       QStringLiteral("Move File"),
                       QStringLiteral("Moves or renames a file inside the workspace. Existing "
                                      "destination files are never overwritten."),
                       ToolRiskLevel::High,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("source"),
                                                   QStringLiteral("File to move."), true},
                           ToolParameterDescriptor{
                               QStringLiteral("destination"),
                               QStringLiteral("Target path (may use a new file name)."), true},
                       }},
        ToolDescriptor{
            QStringLiteral("apply-patch"),
            QStringLiteral("Apply Patch"),
            QStringLiteral("Applies a unified diff patch to workspace files. Supports update "
                           "(--- a/file + +++ b/file), add (--- /dev/null), and delete "
                           "(+++ /dev/null) with @@ hunk headers. Prefer this over many "
                           "edit-file calls for multi-file or multi-hunk changes."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("patch"),
                                        QStringLiteral("The complete unified diff patch text."),
                                        true},
            }},
        ToolDescriptor{
            QStringLiteral("list-code-definitions"),
            QStringLiteral("List Code Definitions"),
            QStringLiteral("Lists classes, functions, and other top-level symbols with line "
                           "numbers for a source file (C/C++, Python, JavaScript/"
                           "TypeScript, Rust). Use it to understand file structure before "
                           "reading or editing."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("path"), QStringLiteral("Source file path."),
                                        true},
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
                    QStringLiteral("Timeout in milliseconds (default 60000, max 600000)."), false},
                ToolParameterDescriptor{QStringLiteral("workdir"),
                                        QStringLiteral("Working directory for the command."),
                                        false},
                ToolParameterDescriptor{
                    QStringLiteral("sandbox"),
                    QStringLiteral("empty for local shell, or 'docker' for container sandbox."),
                    false},
            }},
        ToolDescriptor{
            QStringLiteral("app-launch"),
            QStringLiteral("Launch App"),
            QStringLiteral("Launches a desktop application by name (e.g. 'Spotify', 'Firefox'). "
                           "Use this for opening apps; not for scripts."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("app"),
                                        QStringLiteral("Application name (e.g. Spotify)."), true},
                ToolParameterDescriptor{QStringLiteral("args"),
                                        QStringLiteral("Optional argument to pass."), false},
            }},
        ToolDescriptor{
            QStringLiteral("app-quit"),
            QStringLiteral("Quit App"),
            QStringLiteral("Quits a running desktop application by name (e.g. 'Spotify')."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("app"),
                                        QStringLiteral("Application name (e.g. Spotify)."), true},
            }},
        ToolDescriptor{
            QStringLiteral("open-url"),
            QStringLiteral("Open URL"),
            QStringLiteral("Opens a URL in the user's default web browser (http/https only). "
                           "Use for website requests like 'sahibinden.com aç' — domains are "
                           "websites, not applications. Use web-fetch instead when the page "
                           "content itself is needed."),
            ToolRiskLevel::Medium,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("url"), QStringLiteral("The URL to open."),
                                        true},
            }},
        ToolDescriptor{QStringLiteral("system-notify"),
                       QStringLiteral("System Notify"),
                       QStringLiteral("Shows an immediate desktop notification. For reminders at a "
                                      "future time use set-alarm instead."),
                       ToolRiskLevel::Medium,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("title"),
                                                   QStringLiteral("Notification title."), false},
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
        ToolDescriptor{
            QStringLiteral("system-info"),
            QStringLiteral("System Info"),
            QStringLiteral("Reports the operating system, kernel, CPU architecture, hostname, "
                           "user, total RAM, and root volume usage. Read-only."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {}},
        ToolDescriptor{
            QStringLiteral("process-list"),
            QStringLiteral("Process List"),
            QStringLiteral("Lists currently running processes (pid, CPU, name). Read-only; use "
                           "app-quit to close an application."),
            ToolRiskLevel::Medium,
            ToolExecutionMode::Local,
            {}},
        ToolDescriptor{
            QStringLiteral("current-time"),
            QStringLiteral("Current Time"),
            QStringLiteral("Returns the current local date and time with the timezone, UTC "
                           "time, and epoch seconds."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {}},
        ToolDescriptor{
            QStringLiteral("set-alarm"),
            QStringLiteral("Set Alarm"),
            QStringLiteral("Schedules a future alarm/reminder. The user is notified in chat "
                           "with a desktop notification when it fires. Time formats: 'HH:mm' "
                           "(next occurrence), 'HH:mm:ss', or ISO 'yyyy-MM-ddTHH:mm'."),
            ToolRiskLevel::Medium,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("time"),
                                        QStringLiteral("When the alarm fires."), true},
                ToolParameterDescriptor{QStringLiteral("label"),
                                        QStringLiteral("What the alarm is about."), true},
            }},
        ToolDescriptor{QStringLiteral("list-alarms"),
                       QStringLiteral("List Alarms"),
                       QStringLiteral("Lists all active scheduled alarms."),
                       ToolRiskLevel::Low,
                       ToolExecutionMode::Local,
                       {}},
        ToolDescriptor{
            QStringLiteral("cancel-alarm"),
            QStringLiteral("Cancel Alarm"),
            QStringLiteral("Cancels an active alarm by its id (see list-alarms for ids)."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("id"),
                                        QStringLiteral("The alarm id returned by set-alarm."),
                                        true},
            }},
        ToolDescriptor{
            QStringLiteral("todo-write"),
            QStringLiteral("Todo Write"),
            QStringLiteral("Records the task checklist for this goal as a JSON array of "
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
                    QStringLiteral("JSON array, e.g. [{\"content\":\"step\",\"status\":"
                                   "\"pending\",\"priority\":\"high\"}]"),
                    true},
            }},
        ToolDescriptor{QStringLiteral("todo-read"),
                       QStringLiteral("Todo Read"),
                       QStringLiteral("Reads the current task checklist for this goal."),
                       ToolRiskLevel::Low,
                       ToolExecutionMode::Local,
                       {}},
        ToolDescriptor{
            QStringLiteral("memory-search"),
            QStringLiteral("Memory Search"),
            QStringLiteral("Searches Sentinel's long-term memory entries (keys and values) for "
                           "the query text. Use this before asking the user for information "
                           "that may already be remembered."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("query"),
                                        QStringLiteral("Text to search for."), true},
                ToolParameterDescriptor{
                    QStringLiteral("limit"),
                    QStringLiteral("Max matches to return (default 10, max 50)."), false},
            }},
        ToolDescriptor{
            QStringLiteral("history-search"),
            QStringLiteral("History Search"),
            QStringLiteral("Searches earlier messages of this chat history (all roles) for "
                           "the query text. Use it to recall what the user said before."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("query"),
                                        QStringLiteral("Text to search for."), true},
                ToolParameterDescriptor{
                    QStringLiteral("limit"),
                    QStringLiteral("Max matches to return (default 10, max 50)."), false},
            }},
        ToolDescriptor{
            QStringLiteral("ask-question"),
            QStringLiteral("Ask Question"),
            QStringLiteral("Asks the user a single clarifying question when the goal is "
                           "ambiguous. Provide 2-5 short options, one per line, when the "
                           "answer is a choice. After calling this, end the run with the "
                           "question as your final answer and wait for the user's reply."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("question"),
                                        QStringLiteral("The question to ask."), true},
                ToolParameterDescriptor{QStringLiteral("options"),
                                        QStringLiteral("Optional answer choices, one per line."),
                                        false},
            }},
        ToolDescriptor{
            QStringLiteral("mcp-list"),
            QStringLiteral("MCP List"),
            QStringLiteral("Lists configured MCP (Model Context Protocol) servers and the "
                           "tools they expose. Call this first to discover extendable "
                           "capabilities before using mcp-call."),
            ToolRiskLevel::Low,
            ToolExecutionMode::Local,
            {}},
        ToolDescriptor{
            QStringLiteral("mcp-call"),
            QStringLiteral("MCP Call"),
            QStringLiteral("Calls a tool on a configured MCP server. Find server and tool "
                           "names with mcp-list first. Arguments must be a JSON object "
                           "matching the tool's schema."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("server"),
                                        QStringLiteral("MCP server name from mcp-list."), true},
                ToolParameterDescriptor{QStringLiteral("tool"),
                                        QStringLiteral("Tool name on that server."), true},
                ToolParameterDescriptor{
                    QStringLiteral("arguments"),
                    QStringLiteral("JSON object of tool arguments, e.g. {\"key\": \"value\"}."),
                    false},
            }},
        ToolDescriptor{
            QStringLiteral("spawn-agent"),
            QStringLiteral("Spawn Agent"),
            QStringLiteral("Delegates a self-contained subtask to a bounded read-only "
                           "subagent and returns its final answer. Use for parallelizable "
                           "research or verification subtasks (e.g. 'check whether X is "
                           "documented anywhere in the workspace'). Subagents cannot spawn "
                           "further subagents or edit files."),
            ToolRiskLevel::Medium,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{
                    QStringLiteral("task"),
                    QStringLiteral("The complete, self-contained task description."), true},
            }},
        ToolDescriptor{
            QStringLiteral("browser-screenshot"),
            QStringLiteral("Browser Screenshot"),
            QStringLiteral("Takes a full-page screenshot of a URL in a headless Chromium "
                           "browser via Playwright and saves it as PNG. Requires Node.js "
                           "(npx) and 'npx playwright install chromium'. Use open-url when "
                           "the user just wants to see the page."),
            ToolRiskLevel::Medium,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("url"), QStringLiteral("The page URL."),
                                        true},
                ToolParameterDescriptor{QStringLiteral("path"),
                                        QStringLiteral("Output PNG path (default: temp file)."),
                                        false},
            }},
        ToolDescriptor{
            QStringLiteral("browser-pdf"),
            QStringLiteral("Browser PDF"),
            QStringLiteral("Renders a URL to a PDF document in a headless Chromium browser "
                           "via Playwright. Requires Node.js (npx) and 'npx playwright "
                           "install chromium'."),
            ToolRiskLevel::Medium,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("url"), QStringLiteral("The page URL."),
                                        true},
                ToolParameterDescriptor{QStringLiteral("path"),
                                        QStringLiteral("Output PDF path (default: temp file)."),
                                        false},
            }},
        ToolDescriptor{
            QStringLiteral("web-fetch"),
            QStringLiteral("Web Fetch"),
            QStringLiteral("Fetches a single http(s) URL and returns page content (markdown by "
                           "default). Use after web-search to open a result."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("url"), QStringLiteral("The URL to fetch."),
                                        true},
                ToolParameterDescriptor{
                    QStringLiteral("format"),
                    QStringLiteral("markdown | text | html (default markdown)."), false},
            }},
        ToolDescriptor{
            QStringLiteral("web-search"),
            QStringLiteral("Web Search"),
            QStringLiteral("Web lookup querying the network via local client. Use for current "
                           "news, weather, or anything requiring fresh information."),
            ToolRiskLevel::High,
            ToolExecutionMode::Local,
            {
                ToolParameterDescriptor{QStringLiteral("query"),
                                        QStringLiteral("The search query."), true},
            }},
        ToolDescriptor{
            QStringLiteral("open-workspace"),
            QStringLiteral("Open Workspace"),
            QStringLiteral("Sets the active workspace directory. File tools (read/write/edit/"
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
                       QStringLiteral("Microphone audio capture and STT speech-to-text pipeline."),
                       ToolRiskLevel::High,
                       ToolExecutionMode::Local,
                       {
                           ToolParameterDescriptor{QStringLiteral("path"),
                                                   QStringLiteral("Path to the audio file."), true},
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
                       QStringLiteral("Conversation summary generation and prompt injection."),
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
    const QSet<QString> filesystem{
        QStringLiteral("list-directory"), QStringLiteral("read-file"),
        QStringLiteral("write-file"),     QStringLiteral("edit-file"),
        QStringLiteral("delete-file"),    QStringLiteral("move-file"),
        QStringLiteral("apply-patch"),    QStringLiteral("grep"),
        QStringLiteral("glob"),           QStringLiteral("list-code-definitions")};
    const QSet<QString> network{QStringLiteral("web-fetch"), QStringLiteral("web-search")};
    const QSet<QString> voice{QStringLiteral("voice-transcribe"), QStringLiteral("voice-speak")};
    const QSet<QString> system{QStringLiteral("run-command"),  QStringLiteral("app-launch"),
                               QStringLiteral("app-quit"),     QStringLiteral("system-info"),
                               QStringLiteral("process-list"), QStringLiteral("current-time")};
    for (auto& tool : tools) {
        tool.inputSchema = builtInSchema(tool);
        tool.source = ToolSource::BuiltIn;
        tool.providerId = QStringLiteral("builtin");
        if (filesystem.contains(tool.id)) {
            tool.category = QStringLiteral("Filesystem");
            tool.requiredPermissionDomain = tool.riskLevel == ToolRiskLevel::Low
                                                ? QStringLiteral("workspace-access")
                                                : QStringLiteral("filesystem-write");
        } else if (network.contains(tool.id)) {
            tool.category = QStringLiteral("Network");
            tool.scope = ToolScope::Cloud;
            tool.requiredPermissionDomain = QStringLiteral("cloud-provider-access");
        } else if (voice.contains(tool.id)) {
            tool.category = QStringLiteral("Voice");
            tool.requiredPermissionDomain = tool.id == QLatin1String("voice-transcribe")
                                                ? QStringLiteral("voice-capture")
                                                : QStringLiteral("voice-playback");
        } else if (system.contains(tool.id)) {
            tool.category = QStringLiteral("System");
            tool.requiredPermissionDomain = QStringLiteral("subprocess-execution");
        } else {
            tool.category = QStringLiteral("Agent");
            tool.requiredPermissionDomain = QStringLiteral("context-injection");
        }
    }
    return tools;
}

namespace {
struct BuiltInOperation {
    RealToolExecutor::BuiltInMethod method;
    bool processBacked;
};
const QHash<QString, BuiltInOperation> kMethods = {
    {QStringLiteral("local-plan-summary"), {&RealToolExecutor::executeLocalPlanSummary, false}},
    {QStringLiteral("list-directory"), {&RealToolExecutor::executeListDirectory, false}},
    {QStringLiteral("read-file"), {&RealToolExecutor::executeReadFile, false}},
    {QStringLiteral("write-file"), {&RealToolExecutor::executeWriteFile, false}},
    {QStringLiteral("edit-file"), {&RealToolExecutor::executeEditFile, false}},
    {QStringLiteral("grep"), {&RealToolExecutor::executeGrep, false}},
    {QStringLiteral("glob"), {&RealToolExecutor::executeGlob, false}},
    {QStringLiteral("delete-file"), {&RealToolExecutor::executeDeleteFile, false}},
    {QStringLiteral("move-file"), {&RealToolExecutor::executeMoveFile, false}},
    {QStringLiteral("apply-patch"), {&RealToolExecutor::executeApplyPatch, false}},
    {QStringLiteral("list-code-definitions"),
     {&RealToolExecutor::executeListCodeDefinitions, false}},
    {QStringLiteral("run-command"), {&RealToolExecutor::executeRunCommand, true}},
    {QStringLiteral("app-launch"), {&RealToolExecutor::executeAppLaunch, true}},
    {QStringLiteral("app-quit"), {&RealToolExecutor::executeAppQuit, true}},
    {QStringLiteral("open-url"), {&RealToolExecutor::executeOpenUrl, false}},
    {QStringLiteral("system-notify"), {&RealToolExecutor::executeSystemNotify, true}},
    {QStringLiteral("clipboard-read"), {&RealToolExecutor::executeClipboardRead, false}},
    {QStringLiteral("clipboard-write"), {&RealToolExecutor::executeClipboardWrite, false}},
    {QStringLiteral("system-info"), {&RealToolExecutor::executeSystemInfo, false}},
    {QStringLiteral("process-list"), {&RealToolExecutor::executeProcessList, true}},
    {QStringLiteral("current-time"), {&RealToolExecutor::executeCurrentTime, false}},
    {QStringLiteral("set-alarm"), {&RealToolExecutor::executeSetAlarm, false}},
    {QStringLiteral("list-alarms"), {&RealToolExecutor::executeListAlarms, false}},
    {QStringLiteral("cancel-alarm"), {&RealToolExecutor::executeCancelAlarm, false}},
    {QStringLiteral("todo-write"), {&RealToolExecutor::executeTodoWrite, false}},
    {QStringLiteral("todo-read"), {&RealToolExecutor::executeTodoRead, false}},
    {QStringLiteral("memory-search"), {&RealToolExecutor::executeMemorySearch, false}},
    {QStringLiteral("history-search"), {&RealToolExecutor::executeHistorySearch, false}},
    {QStringLiteral("ask-question"), {&RealToolExecutor::executeAskQuestion, false}},
    {QStringLiteral("mcp-list"), {&RealToolExecutor::executeMcpList, false}},
    {QStringLiteral("mcp-call"), {&RealToolExecutor::executeMcpCall, false}},
    {QStringLiteral("spawn-agent"), {&RealToolExecutor::executeSpawnAgent, false}},
    {QStringLiteral("browser-screenshot"), {&RealToolExecutor::executeBrowserScreenshot, true}},
    {QStringLiteral("browser-pdf"), {&RealToolExecutor::executeBrowserPdf, true}},
    {QStringLiteral("web-fetch"), {&RealToolExecutor::executeWebFetch, false}},
    {QStringLiteral("web-search"), {&RealToolExecutor::executeWebSearch, false}},
    {QStringLiteral("open-workspace"), {&RealToolExecutor::executeOpenWorkspace, false}},
    {QStringLiteral("voice-transcribe"), {&RealToolExecutor::executeVoiceTranscribe, true}},
    {QStringLiteral("voice-speak"), {&RealToolExecutor::executeVoiceSpeak, true}},
    {QStringLiteral("summarize-current-conversation"),
     {&RealToolExecutor::executeSummarizeCurrentConversation, false}},
    {QStringLiteral("provider-test-call"), {&RealToolExecutor::executeProviderTestCall, false}},
    {QStringLiteral("export-conversation"), {&RealToolExecutor::executeExportConversation, false}}};
class BuiltInHandler final : public IToolHandler {
public:
    BuiltInHandler(RealToolExecutor& executor, RealToolExecutor::BuiltInMethod method,
                   bool processBacked)
        : executor_(executor), method_(method), processBacked_(processBacked) {}

    IToolExecutor::Cancel execute(const ToolExecutionRequest& request, const QString& sessionId,
                                  const QString& toolCallId, IToolExecutor::Output output,
                                  IToolExecutor::Completion completion) override {
        if (processBacked_)
            return executor_.executeAsync(request, sessionId, toolCallId, std::move(output),
                                          std::move(completion));
        QString currentWorkingDirectory = QDir::currentPath();
        completion((executor_.*method_)(request.plan.invocations.first(), currentWorkingDirectory));
        return {};
    }

private:
    RealToolExecutor& executor_;
    RealToolExecutor::BuiltInMethod method_;
    bool processBacked_;
};
} // namespace
RealToolExecutor::BuiltInMethod BuiltInToolProvider::methodFor(const QString& id) {
    return kMethods.value(id).method;
}
bool BuiltInToolProvider::registerTools(IToolRegistry& registry, RealToolExecutor& executor) {
    const auto tools = descriptors();
    if (tools.size() != static_cast<int>(kMethods.size()))
        return false;
    for (int i = 0; i < tools.size(); ++i) {
        auto descriptor = tools.at(i);
        descriptor.source = ToolSource::BuiltIn;
        descriptor.providerId = QStringLiteral("builtin");
        const auto operation = kMethods.value(descriptor.id);
        if (!operation.method)
            return false;
        auto handler =
            std::make_shared<BuiltInHandler>(executor, operation.method, operation.processBacked);
        if (!registry.registerTool({std::move(descriptor), std::move(handler)}))
            return false;
    }
    return true;
}
} // namespace sentinel::core
