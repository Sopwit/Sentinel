# Sentinel Terminal Experience V2

The existing Rust/Ratatui client remains a thin IPC client. C++ daemon owns provider selection, immutable bindings, Agent execution, tools, permissions, sessions and persistence. Protocol schema is unchanged.

## Implemented layout

Three-line borderless header: Sentinel, explicit Chat/Agent mode, session/run/connection state; provider/model readiness; workspace. Unknown readiness remains unknown. Borderless transcript gives space back to content. Empty/disconnected states offer actionable commands, never auto-launch or auto-install. Composer starts at three rows, grows to at most one third of available height, and scrolls long lines using terminal cell width. Footer shows draft references, runtime notices or new-content-below guidance and essential keys. Under 24×8, an enlarge-window message replaces unusable panes.

Active output distinguishes You, Assistant/Agent and daemon state, with the latest three safe activity summaries inline; /activity exposes the bounded 200-entry metadata timeline. Historical canonical message roles remain daemon-owned. No raw private reasoning or tool output is newly exposed. Tool completion and approval cannot complete a run.

Pickers retain subsequence filtering, selected-row marker, disabled-row explanation and selected identity details. Tab/Shift+Tab navigate picker focus; Escape preserves choices. Commands cover provider/model/workspace/session/tools/MCP/permissions/context/activity/help. No fabricated provider availability.

Rendering is dirty-driven, coalescing queued events before redraw rather than repainting every 50ms while idle. Display output stays bounded at 262144 bytes, activity at 200 entries, editor at 65536 bytes, recall at 100 entries, update queue at 256 and request queue at 32. Queue failure restores pending submission text/references. Reconnection refreshes diagnostics and reattaches authoritative session state without replaying mutations. Foreign-generation and late closed-run deltas are ignored.

Default terminal foreground/background are preserved for unknown themes. SENTINEL_TUI_THEME=dark uses Glacier #A9CAD3; light uses Obsidian #151719. NO_COLOR preserves the terminal palette. Porcelain #ECEFEE remains the identity reference for light content, not a forced terminal background. SENTINEL_TUI_ASCII=1 uses ASCII panel borders and the selection marker is ASCII. Message text and some punctuation remain Unicode; this is a border fallback, not transliteration or a complete ASCII renderer. Wide-character cursor/scroll uses Ratatui cell widths; grapheme-cluster navigation is not claimed.

See [keyboard contract](TUI_KEYBINDING_CONTRACT.md) and [compatibility matrix](TERMINAL_COMPATIBILITY_MATRIX.md). These improvements retain the current interface and authority boundaries, without copying another product.

## V2.1 composition refinement

The header now uses `SENTINEL / Chat` or `SENTINEL / Agent`, with transport connection state aligned right, a thin separator, and a subdued provider/model/readiness row. The empty state is centered within the conversation viewport. Message-role labels receive accent emphasis, including canonical daemon history roles. The three-row expanding composer has a short `Message` label; shortcuts live in the footer, with authoritative task state aligned right. Workspace selection remains available through the existing picker. Existing runtime, protocol and key handling are unchanged.

Structural TestBackend coverage checks the header, centered disconnected state, composer position and Agent timeline at 80×24, 120×40 and 160×48; the existing size/approval matrix also covers smaller windows. These tests are render checks, not native terminal screenshots. V2.1 native before/after capture is blocked because computer-use refuses Ghostty access. The approved reference attachment was unavailable in this execution context, so screenshot comparison and live flow visual acceptance remain outstanding.
