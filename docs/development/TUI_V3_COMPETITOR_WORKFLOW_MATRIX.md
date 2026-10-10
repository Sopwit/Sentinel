# TUI V3 competitor workflow matrix

Reviewed official sources on 2026-10-09: [Codex CLI](https://developers.openai.com/codex/cli/), [Codex slash commands](https://developers.openai.com/codex/cli/slash-commands/), [OpenCode TUI](https://opencode.ai/docs/tui/), [OpenCode keybindings](https://opencode.ai/docs/keybinds/), [OpenCode commands](https://opencode.ai/docs/commands/), and [Google AGY codelab](https://codelabs.developers.google.com/build-deploy-embed-agy-agents-cli). The requested docs.opencode.ai host failed; the canonical opencode.ai documentation was available. This is documentation research, not acceptance testing of installed competitors or a feature parity claim.

Codex documents an interactive CLI with session resumption and slash workflows, including explicit Plan selection. Sentinel adopts discoverable commands and resume semantics, but cannot copy Plan without daemon policy. OpenCode documents fuzzy file references, a command palette, configurable bindings and external editing; its custom commands can include shell/file expansions. Sentinel adopts fuzzy discovery and help, retaining its existing gateway-only references and omitting expansion/execution. AGY documents `/resume` and `/grill-me`, with one focused question at a time and recommended options; its tutorial requests a planning checkpoint. That is useful interaction guidance, not proof of a Sentinel-enforceable no-write mode. Permission-bypass flags are not adopted.

## Baseline and chosen scope

| Capability | Sentinel baseline | V3 result | Contract / reason |
|---|---|---|---|
| Composer and keyboard | Existing and working: Unicode, multiline, word movement, recall, bracketed paste, Ctrl+O/P, dynamic height | EXISTING + REUSED; pasted slash provenance strengthened | Editor + Crossterm; no shell parsing |
| Slash commands | Present but incomplete: separate match dispatch, no inline discovery | IMPLEMENTED + VERIFIED in deterministic tests: typed metadata, fuzzy popup, arguments, suggestions, aliases | One registry and handler |
| Palette/help | Existing but fixed and weakly described | IMPLEMENTED + VERIFIED in deterministic tests: unified metadata and searchable help with disabled reasons | No permissions granted |
| Sessions | Existing create/list/attach; missing continuity affordances | IMPLEMENTED + PARTIALLY VERIFIED: query, resume, rename/archive, draft cache | Existing session IPC and finite desktop.action; no active switch |
| Model/provider | Existing daemon selection and readiness | EXISTING + REUSED; picker refresh preserves filter/selection | Catalog is not inference success; no fallback |
| Workspace/file references | Existing authorized listing/picker and Agent path references | EXISTING + REUSED; searchable full paths, removal picker, quoted display, stale-reply protection | No eager reads; missing attachment contract for Chat |
| Execution | Existing typed timeline, Allow Once/Deny and cancellation | IMPLEMENTED + PARTIALLY VERIFIED: details disclosure and independent inspection views | Raw arguments/results unavailable in safe event projection |
| History/export | Present backend history/actions; missing TUI export command | IMPLEMENTED + PARTIALLY VERIFIED: current-session search/refresh and controlled export dispatch | No arbitrary export destination or global history engine |
| Diff | Existing Applied bounded workspace changes | EXISTING + REUSED, `/review` alias | No reliable rollback, Git operations or inferred changes |
| Preferences | Existing theme/ASCII/no-color environment; no TUI keybinding schema | EXISTING + REUSED; read-only `/settings`, memory-only details toggle | Persistent editor/customization deferred |
| Plan | Missing; requires backend/IPC extension | SPECIFIED ONLY, disabled | Authoritative deny-write/execute policy needed |
| Compact | Missing context mutation contract | SPECIFIED ONLY, disabled | Conversation/context authority needed |
| Undo/redo | Missing durable snapshot/conflict restore | DEFERRED, disabled | Unsafe without reliable recovery |
| MCP/tools | Existing read-only projections | EXISTING + REUSED | Metadata is not executable custom command support |
| Skills/init | Metadata exists, execution semantics incomplete | SPECIFIED ONLY, disabled | Needs bounded daemon operations, approval, audit |
| Guided questions | Missing typed interactive runtime state | SPECIFIED ONLY, disabled | Question/answer persistence and plan approval needed |
| External editor | Missing safe terminal/editor lifecycle | DEFERRED, disabled | No shell invocation shortcut |
| Shell shorthand | Existing run-command tool in daemon | DEFERRED | `!text` stays ordinary input; no second shell path |
| Multiple concurrent session execution | Runtime has one authoritative active task | DEFERRED | No pretend viewing/executing split |

## Architecture map

`sentinel-cli` parses existing clap commands; `sentinel-tui` owns presentation, editor, pickers and the bounded background worker; `sentinel-ipc` validates generated `protocol/ipc-v1.json` request/response types. `DaemonService` brokers terminal/session/model/workspace IPC. AgentRuntime/AgentLoop own execution and finality; ModelService/IModelRouter own accepted bindings; ToolRegistry/ToolExecutionGateway and IFileSystemService/ProcessExecutor own tool authority. Session store and conversation history remain daemon persistence. Desktop finite actions already expose rename/archive/export; reusing them does not edit QML or add a generic action bridge.

The client never opens workspace files, invokes processes, changes grants or initializes models locally. Registry security categories are documentation/availability metadata, not an alternative policy engine. Protocol schema, daemon behavior, Desktop and Quick Panel are unchanged.
