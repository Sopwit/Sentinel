# Sentinel TUI keyboard contract

2026-10-09 implementation. Ctrl+S is not Send and has no application binding. Paste never executes. No Ctrl+Enter portability claim.

| Key | Context and action |
|---|---|
| Enter | Composer: send non-empty text or execute an exact slash command; active/disconnected requests preserve draft |
| Ctrl+O | Composer: insert newline |
| Alt+Enter / Shift+Enter | Newline only when terminal reports distinct modifiers; Ctrl+O is portable alternative |
| Ctrl+P / Tab | Composer: command palette |
| Ctrl+R / Ctrl+F | Search displayed transcript (not a new global-history backend) |
| Ctrl+A / Ctrl+E | Current input line start/end |
| Ctrl+U / Ctrl+K | Delete to current line start/end; Ctrl+K never opens palette |
| Arrows / Home / End / Ctrl+Left/Right | Character, line and word navigation |
| Alt+Up/Down | Memory-only input recall; restore unfinished draft |
| Ctrl+L / Ctrl+W | Model / workspace picker |
| /sessions / Ctrl+N | Session picker / create session, while idle |
| /reconnect | Explicit connection retry, no work replay |
| /remove | Remove last selected file reference |
| PgUp / PgDn / Ctrl+B | Scroll transcript / follow latest |
| F1 / /help / ? on empty composer | Discover actual commands and keys |
| Escape | Close picker/search without changing draft; pending approval remains open |
| Ctrl+C | Active run: request cancellation; starting request: cancel after acknowledged run ID; idle draft: preserve text and dismiss overlays; idle/empty: exit |
| Ctrl+D | Exit only idle, not busy, empty composer and no references; otherwise delete character; approval/picker capture keys first |
| y / n | Pending approval only: Allow Once / Deny; wait for authoritative acknowledgement |
| Tab / Shift+Tab / Up / Down | Picker: next/previous selection; Enter selects; unavailable/disconnected mutations rejected |

Keyboard precedence: cancellation → pending approval → picker → transcript search → composer. Work never completes merely because cancellation or approval was acknowledged. Errors do not discard drafts. Terminal cleanup uses a scope guard and Ratatui panic handling; SIGKILL cannot restore a terminal.
