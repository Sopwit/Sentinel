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

## V2.2 visual clarity

- Conversation content uses a centered column capped at 88 terminal cells, with outer whitespace at 80×24 and larger sizes. Word-aware wrapping handles uninterrupted technical tokens and wide characters; rendered row counts drive scrolling, follow-latest and transcript search.
- Canonical daemon message roles become lightweight labeled blocks: bold `> You` with deeper indentation, accented Assistant/Agent labels, and secondary Execution detail/System notice sections. No message boxes are added. Fenced code receives secondary styling without removing its text.
- Active execution metadata is separated beneath an Execution timeline heading, limited to the existing latest three activity entries; `/activity` retains the bounded detail view. An empty timeline is omitted. No new tool payload or private reasoning is exposed.
- The header includes the daemon workspace name alongside identity and mode when space permits; connection stays right aligned and provider/model/readiness remain on the quieter information row. The separator honors the existing ASCII border fallback.
- The composer retains its three-row minimum and existing growth/cursor behavior, adds a Chat/Agent label and mode-specific placeholder, and accents its border while focused. Picker/approval focus quiets that border.
- The footer reserves notices/search/reference guidance for its first row. The second row contains focus-specific shortcuts and a single run-state label. Idle completion state is no longer echoed as a second notice.

Runtime authority, release configuration, protocol, dependencies, keyboard bindings, rendering cadence and retained-output limits are unchanged. Rust render checks cover role/code styling, content-preserving wrapping, final-row visibility and bounded reading width at 80×24, 120×40 and 160×48, plus the existing small-window/picker/approval matrix. Native emulator compatibility has not been newly certified; previous screenshot-access limitations still apply.

## V2.3 empty state and content grid

The empty Chat/Agent viewport always shows a centered minimal symbol (`◇ S ◇`, or `[ S ]` with the ASCII switch), “Start a new conversation”, a short prompt and command/help discovery. Actual connection/model warnings remain separate in the header/footer. Canonical system-only history no longer suppresses the welcome: system messages are retained separately and accessible through `/notices`, which also shows the current runtime notice without adding an assistant role.

A shared centered outer grid is capped at 92 cells including composer borders and padding; transcript labels occupy at most 88 cells. The padded input aligns with the assistant body-text column. At 80 cells the grid uses 76 cells; at 100 and wider it uses 92. Cursor math, horizontal input scrolling and composer expansion use the padded inner rectangle. A trailing newline now expands the composer immediately; the line count is capped before integer conversion. Picker, approval and search focus quiet the composer border and hide its editing cursor.

Header identities use explicit ellipsis and independent connection/readiness space, so long names cannot overwrite those statuses. The primary row omits verbose diagnostic summaries. Available provider health maps conservatively to `Inference unverified`, never `Ready`; other states retain explicit provider unavailable/degraded or unknown inference wording. The complete daemon summary stays in `/doctor`. Placeholder and footer text use readable secondary foregrounds for explicit dark/light themes and terminal-default foreground otherwise; NO_COLOR still preserves the terminal palette.

Deterministic rendering covers eight states at 80×24, 100×30, 120×40 and 160×48: empty connected/disconnected, populated Chat, long identity names, running Agent, pending approval, picker focus and search focus. Tests verify welcome centering, reserved labels, input/body alignment, border focus and cursor restoration. Additional tests cover system-only history, trailing-newline growth and long multiline Unicode drafts. Existing message wrapping, code/detail styles, follow-latest, cancellation/approval recovery and key contracts remain covered.

See `docs/reviews/terminal-v23-2026-10-09/README.md` for actual disconnected before/after PTY output, clearly labeled synthetic rendering fixtures, validation and visual limits. No native emulator acceptance or comparison with unavailable screenshot attachments is claimed. Runtime/IPC/security authority, dependencies and Desktop/Quick Panel are unchanged; no commit, push or merge was performed.
