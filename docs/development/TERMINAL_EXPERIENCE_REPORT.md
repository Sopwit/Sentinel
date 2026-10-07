# SENTINEL — TERMINAL EXPERIENCE COMPLETION REPORT

Validation dates: 2026-10-07–08. Host: macOS arm64, Qt 6.11.2. Fedora remains the primary target; Linux/Windows native acceptance was not performed. The existing dirty Desktop/branding work was preserved. No commit or push was made during the initial validation. A subsequent user-authorized follow-up groups the changes into separate commits; no push was requested.

## 1. CLI Installation

The canonical global executable is `/Users/emir/.cargo/bin/sentinel`, on PATH, reporting `sentinel 1.0.0-rc.8`. `tools/terminal/install-dev.sh --force` installs the Cargo package with its lockfile. No shell configuration is changed. Uninstall: `cargo uninstall sentinel-cli`. Source-release installation is documented; crates.io/prebuilt packaging is not claimed.

## 2. CLI UX

Clap provides successful offline help/version and subcommand help, validates input before connecting, and preserves the default status command and run/agent aliases. Human errors explain daemon, protocol, model/provider, permission, runtime and disconnect categories. Stable exits are 0 success, 1 failed task, 2 usage, 3 connectivity/timeout, 4 protocol, 5 provider/model/authentication, 6 permission, 7 runtime and 130 cancellation. Automated coverage includes parser, help and error/exit behavior.

## 3. Output / Automation

Text, single-document JSON and newline-delimited stream JSON are supported. Successful query payload shapes remain compatible; machine failures have a versioned typed error document. Stream events retain identifiers and protocol envelopes. Positional input wins over stdin, without concatenation; input is bounded to 65536 UTF-8 bytes. Real global status/models/doctor JSON and piped Agent stream output were checked for parseability and stdout/stderr separation.

## 4. Daemon

Startup stays explicit; no duplicate daemon is silently launched. Status exposes version, IPC version, endpoint, uptime, generation and active state. IPC is additive version 1.1. TUI requests use a background worker and bounded polling with preserved partial frames. `terminal.attach` returns a safe bounded snapshot while existing Desktop `session.attach` behavior remains compatible. Reconnect does not cancel or replay execution.

## 5. Provider / Model

Discovery, explicit provider/model selection and endpoint configuration use ModelService and existing daemon settings. Empty/unavailable catalogs remain truthful. Cached ModelService snapshots prevent opening the TUI from reading every cloud credential; unobserved credentials/catalogs remain unverified until explicit discovery/preflight. Preflight distinguishes unavailable models/providers. Active binding mutation is refused by the daemon and covered by tests.

## 6. TUI

A real Ratatui terminal flow passed with header, transcript, multiline composer, footer and modal pickers. Chat/Agent switching is explicit. UTF-8 cursor editing, Home/End, line/word navigation, delete, bracketed paste and bounded in-memory history with draft restoration are implemented. Enter inserts a newline; Ctrl+S sends. Ctrl+L opens models (Ctrl+M is terminal Return and is intentionally avoided). Layout tests cover 80×24, 100×30, 120×40, 40×12 and 10×4; live PTY resize checks cover the first four. Terminal-default colors and bold approvals preserve background compatibility.

## 7. Slash Commands

`/model`, `/provider` and `/workspace` open authoritative pickers. `/tools`, `/mcp`, `/permissions`, `/context`, `/memory` and `/tasks` show safe read-only backend state. `/status` and `/doctor` inspect daemon status/diagnostics. `/compact` explains that an authoritative compaction operation is unavailable and changes nothing. `/help` documents keys and commands. Additional `/sessions`, `/new`, `/chat`, `/agent`, `/activity`, `/files` and `/diff` are available. Unknown commands never become Agent or shell execution.

## 8. Pickers

Model, provider, workspace, session, file and change-review pickers have fuzzy filtering, navigation, Enter and Esc. Models expose provider, capabilities, readiness and selection. Unavailable choices are distinct. Sessions show titles, mode/state and pin/archive markers; identifiers, creation time and binding metadata are accessible. An authoritative updated timestamp is not fabricated when absent.

## 9. File / Workspace

File selection returns authorized workspace paths through registered glob handling; quoted/space-containing references remain intact. Selected paths appear in the footer and can be removed before send. Agent observes content through normal tools; Rust never reads it. Chat with attached references requires explicit mode switching. Workspace root comes from WorkspaceService. Hidden/sensitive, outside-root and disabled-tool tests pass. Repository awareness includes root and observed changed files; Git branch/status is not invented.

## 10. Agent Activity

Safe state transitions, tool requested/running/completed/failed, approval decisions, timestamps/durations and bounded activity are projected from runtime events. Raw arguments/results and hidden reasoning are omitted. Subagent identity, role, parent and terminal state are projected only from actual backend events; a real model-driven subagent run was not required or fabricated. Accepted AgentLoop finals alone complete Agent execution.

## 11. Approvals

The modal shows tool, resource, risk, pending-operation scope and run/session identity. Allow Once succeeded on real fixture writes. Deny produced an authoritative failed run with `User denied the required action.` Reconnect preserved the same pending approval; Esc kept it pending. In-flight duplicate protection and stale/duplicate daemon rejection are tested. No persistent grant action is introduced. Pending approval events are labelled approval, not completed.

## 12. Cancellation

Real CLI SIGINT produced run.cancelled and exit 130. Real TUI Ctrl+C, including repetition, produced the daemon's cancelled state; idle exit succeeded. The client waits for authoritative termination and never kills daemon-owned execution locally. Cancelling while start is pending is retained until the run ID arrives.

## 13. Sessions

Create, list, attach and continuation use daemon conversations. A completed coding session resumed in Chat and returned READY. Reconnect restores snapshot, mode, output and pending approval. Generation/sequence deduplication and foreign-session filtering are tested. Daemon restart recovers conversations without restarting interrupted execution or retaining change baselines.

## 14. Coding Workflow

An isolated project at `/private/tmp/sentinel-terminal-e2e/project` contained greeting.txt. The existing Nemotron model used normal write-file approval and read-file observation, changed the harmless text, and returned an accepted grounded final. The global-binary coding run's session was `14d71f3c-eb92-4aad-9e93-954a0ce3a7a2`. The observed final text was `Final acceptance passed` (no trailing punctuation/newline). Daemon review returned a unified Applied diff. The TUI changed-file picker and diff view were exercised. No Sentinel source was used as the acceptance workspace.

## 15. Doctor

CLI and TUI diagnostics cover connectivity/protocol, PATH, model/provider snapshot, workspace, registry, MCP, permissions, context/memory and network policy without exposing credentials. JSON is parseable. Unobserved cloud credential availability stays explicitly unverified.

## 16. Shell Completion

Clap generates zsh, bash and fish completion scripts offline. Installation is documented and manual; no shell startup files are modified.

## 17. Performance

One live 100×30 PTY sample observed first usable/readiness frame at approximately 102 ms; warm picker responses were approximately 103–110 ms. These are upper-bound observations using 100 ms sampling, not benchmark-grade timings. Five CLI status samples were 18.64, 11.18, 8.65, 7.38 and 6.11 ms. The observed session switch was approximately 105 ms at the same sampling resolution. No blocking IPC or provider enumeration occurs in the Rust render loop. Backend file inspection uses bounded registered read-only handlers inline; large/slow filesystems may increase daemon request latency.

## 18. Real E2E

No model was downloaded. Existing NVIDIA Nemotron-3-Nano-4B Q4_K_M GGUF was served by the installed llama-server as `sentinel-nemotron` through the normal llama-cpp-server provider. Discovery/selection, Chat, Agent observation, approved coding, Deny, cancellation, session continuation, TUI reconnect and JSON automation passed. Existing Ollama/qwen2.5:3b Chat also passed; its Agent trial failed truthfully with an invalid-next-action result. Nemotron was selected explicitly, without implementation fallback or provider-specific shortcuts. Owned test daemon/server processes were cleaned up; the user's existing daemon was left untouched.

## 19. Security Boundaries

Rust remains an IPC presentation client: no provider/model/permission database, plugin execution, filesystem access, subprocess tools or natural-language shell planning. Daemon inspection validates built-in read-only descriptors, normalized plans, workspace resources, authorization policy, explicit denials, approval posture, sandbox and ToolExecutionGateway, including hooks. The existing IFileSystemService executes file access. Inline inspection is limited to glob/read-file and does not create a mutation/revert path. Runtime bindings, approvals, sandboxing, grounding and cancellation remain C++ authority.

## 20. Regression

Configure/build passed. Full C++: 113/113 executable suites passed (106.18 seconds); Controller 127, Shell 74, Desktop IPC 38, Native integration 3. The final full run includes the approval-state and thread-publication fixes; the affected daemon/Desktop IPC/generated-contract check also passed 3/3. Rust: 36 tests passed (CLI 14, IPC 11, TUI 11), increased from 17. Cargo fmt and strict clippy passed. Generated IPC/Desktop checks and git diff --check passed. QML lint exited zero with existing context-property warnings; this phase changed no QML. Clang-tidy remains BLOCKED by the previously documented Apple PCH/toolchain incompatibility, not PASS. Existing platform-specific skipped cases are not claimed as native validation.

## 21. Remaining Work

Authoritative compaction, robust authorized rollback, Git branch/dirty metadata, durable/full/minimal change snapshots, timestamp enrichment and richer per-tool safe summaries remain future work. Change baselines are bounded to 8 sessions, 32 text files, 8192 characters/file and 32768 characters/baseline; review response diffs are capped at 50000 characters. Hidden/binary/denied/oversized/deleted/unreadable resources may be omitted, truncation is explicit, and baselines do not survive restart. Diffs show observed changes, including possible external edits, not proven Agent attribution. Windows Rust transport and Linux/Windows native acceptance remain outside this phase. Distribution packaging/updating remains deferred as requested.

## 22. Commit Readiness

The terminal implementation is validated and reviewable. CLI/TUI user guides and the interface guide match the implemented keys, contracts and limitations. The shared checkout includes pre-existing Desktop/branding changes and should be reviewed by scope before committing. The initial validation did not create commits or pushes; commits were subsequently authorized by the user. The user’s previously running daemon was not restarted; restart it from the updated build before using the new TUI operations.

FINAL VERDICT: FIXED + PASS
