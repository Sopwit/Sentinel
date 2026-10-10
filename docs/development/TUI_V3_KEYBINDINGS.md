# TUI V3 keyboard and focus contract

Status: IMPLEMENTED + VERIFIED by deterministic routing tests; native emulator acceptance NOT VALIDATED.

| Context | Binding | Behavior |
|---|---|---|
| Composer | Enter | Send ordinary text, or validate exact typed slash command |
| Composer | Ctrl+O | Insert newline; Alt/Shift+Enter remain optional aliases |
| Composer | Ctrl+P / plain Tab | Unified palette; Tab completes when slash discovery is open |
| Composer | F1 / empty `?` | Searchable help |
| Composer | Ctrl+N / Ctrl+L / Ctrl+W | New session / model / workspace via registry |
| Composer | Ctrl+G | Chat/Agent switch via registry, idle only |
| Composer | Ctrl+R / Ctrl+F | Displayed transcript search |
| Composer | Left/Right, Home/End, Ctrl+A/E | Character/line movement |
| Composer | Ctrl+Left/Right | Word movement |
| Composer | Ctrl+U/K | Erase to line start/end; Ctrl+K is never a global action |
| Composer | Up/Down | Multiline movement; single-line recall as before |
| Composer | Alt+Up/Down | In-memory prompt recall, restoring unfinished draft |
| Composer | `@` | Authorized file picker; backslash-prefixed `@` stays literal |
| Composer | `//` prefix | Literal leading slash on send |
| Composer | bracketed paste | Insert only; pasted leading slash stays literal through recall/failure |
| Slash popup | Up/Down, Tab, Enter, Esc | Navigate, complete only, complete partial/execute exact, dismiss preserving input |
| Picker | text, Up/Down, Tab/Shift+Tab, Enter, Esc | Filter, select, select, dispatch available row, close preserving draft |
| Search | text, Enter, Esc | Query, find first match, close |
| Transcript | PageUp/PageDown, Ctrl+B | Scroll / follow latest |
| Permission | y / n | Allow Once / Deny; remains pending until daemon acceptance |
| Permission | Esc | Preserve pending request, never implicit approval |
| Any | Ctrl+C | Cancel active task through daemon; otherwise close context/preserve draft; exit only idle/empty |
| Composer | Ctrl+D | Exit only idle/empty, otherwise delete |
| Inspection view | Esc | Return to conversation without losing streamed content |

Priority: Ctrl+C safety handling → approval → picker → search → slash discovery → registry bindings → composer editing. Contextual UI consumes its keys, preventing accidental sends. No leader key or new terminal sequence was added. Ctrl+S is not Send. Mouse is not captured, preserving native selection/copy; no application clipboard API was added.

Command bindings are defined once in `commands.rs::Binding`; help displays those same labels. Editor-specific keys remain in the established editor contract. See [V2 contract](TUI_KEYBINDING_CONTRACT.md). No persistent key remapping is claimed: the shared settings mechanism has no scoped TUI binding schema, validation or migration. Adding a second config file now would create competing sources. Current theme/fallback variables remain the only persisted launch preferences; `/settings` reports them without mutation.

Drafts and recall are memory-only; no crash/restart recovery is promised. Up to 32 sessions with unsent text/references are cached. Switching refuses to exceed this budget, and never evicts drafts silently. Pasted text provenance and per-session mode/references/cursor are retained. No selection buffer, clipboard bridge or external-editor lifecycle is implemented.
