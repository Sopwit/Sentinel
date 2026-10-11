# V3.3-B deterministic interaction evidence

Run `python3 docs/reviews/tui-v33b/pty_journeys.py` from the repository. It uses the release binary and disposable local Unix sockets; no real daemon or inference is started. It requires local socket/PTY access. Four dimensions are repeated in normal and ASCII/NO_COLOR environments, plus a disconnected journey. Assertions use a bounded text VT replay and typed IPC fixtures. Replays are not physical screenshots or color acceptance.

`pty-results.json` records release hash and nine journey outcomes. Per-stage .txt files, .ansi captures, request records and bounded mascot diagnostics support the assertions. Build/test/lint logs and `cw045-results.json` record scoped revalidation. See [delivery report](../../development/TUI_V3_3_B_REPORT.md).
