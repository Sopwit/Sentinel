# SENTINEL — TUI V4-A release readiness

> SEC-P1.1 privacy edit (2026-10-12): machine-specific paths are shown as role placeholders; recorded measurements and acceptance results are unchanged. Command examples use `SENTINEL_REPO`; set `export SENTINEL_REPO="$(git rev-parse --show-toplevel)"` from a checkout first. This edited report has a new file digest; original raw evidence is unchanged.

2026-10-11. Recommendation: **Needs targeted correction** before unconditional release-candidate approval. Production startup/assets and normal/handled-error flows were stabilized; external SIGTERM cleanup, physical acceptance and distribution-license/packaging gates remain open. No V4-B work, publication, global installation, shell changes, commit, push or merge.

## Implemented fixes and causes

1. **Remove runtime dependence on the build machine's home/workspace.** Production asset discovery previously fell back to `env!("CARGO_MANIFEST_DIR")/../../../assets/core-warden`. An installed binary could silently use the developer checkout or lose graphics when moved. Lookup now uses an authoritative explicit override, executable-relative installed `../share/sentinel/core-warden`, or the bounded standard runtime checkout path `cli/target/debug|release` → repository assets. It never recursively searches or infers assets from the working directory. An invalid explicit override stays authoritative; missing assets remain text fallback. Existing contract/frame validation and path containment are unchanged.
2. **Cleanup covers failed terminal initialization.** With PTY stdin and a closed stdout pipe, the old production executable exited 3 and left echo/canonical input disabled, because the cleanup guard was created only after `try_init`. The guard now exists before initialization. The same fixture exits 1 with `Terminal initialization failed: Broken pipe` and restored input settings. The error no longer incorrectly claims a daemon connection failure. Ratatui already restores raw input before alternate-screen writes; no extra restore implementation was needed. macOS's transient PENDIN retype flag is excluded from the fixture comparison; ECHO/ICANON and all other termios fields are verified.
3. **Provide bounded local evaluation staging.** `tools/terminal/stage-release.sh NEW_DIRECTORY` copies the already-built Rust executable, existing canonical animation/contract files and project license. It refuses an existing destination, does not rebuild/install/publish and does not copy Blender/reference/render sources. Documentation distinguishes source installation, text-only binary copies and staged graphical evaluation. This is not a new animation pipeline or signed installer.

Both established terminal fixes remain unchanged: no trailing CSI 5n, and scoped Unix nonblocking polling prevents late cell-size replies from exceeding Crossterm's wait. No timer, busy polling, repeated decoding or unconditional clear was introduced.

## Executable, resources and installation audit

- Actual executable: `<repository-root>/cli/target/release/sentinel`.
- Identity: Mach-O 64-bit **arm64**, `sentinel 1.0.0-rc.8`. User-visible version comes from the canonical CMake version through the existing IPC build script; Cargo workspace versions agree.
- Final SHA-256: `6229bccc0ebaf37f34d7811b15fa799e3dc972a9517f1de9b6ffc8704d06cc81`.
- Local evaluation stage: `<repository-root>/docs/reviews/tui-v4a/rc-stage`. Its `bin/sentinel` hash exactly matches the final release. [Inventory](../reviews/tui-v4a/stage-inventory.json) records hashes/sizes for 137 files: executable, 132 PNGs, animation manifest, shared state contract, GPL project LICENSE and local-stage README.
- Stage moved to a disposable directory and started outside the repository: actual production graphics cache loads. Bare binary outside the repository: safe text fallback, even while the original developer checkout still exists. Missing explicit override also stays fallback rather than leaking back to checkout assets. Checkout release itself still discovers assets when launched from an unrelated working directory.
- There is no CLI-owned runtime config file needed for startup. Existing daemon endpoint resolution uses the runtime HOME/default socket or explicit `--socket`; TUI preferences remain environment/process-derived. Themes are builtin, terminal fonts are host-owned. Daemon configuration, provider credentials/models and authoritative persistence are separate and are not staged.
- `command -v` / `which -a` found no `sentinel` on this host's PATH; common Cargo/Homebrew/local bin locations contained no tested shadowing copy. No global binary was installed. An arbitrary shell/host may differ: use the absolute path and `--version` for acceptance.
- `cargo install` / existing `install-dev.sh` installs the binary only; it does not bundle mascot assets. This limitation is now documented. An explicit asset root or staged layout is needed for graphics.
- Existing release automation packages Desktop through CMake/CPack, with no Rust CLI staging step. It must not be advertised as distributing this terminal candidate. Cargo registry publication is not validated; local path dependencies/build-time version inputs assume the source workspace. No registry/package support was invented.
- Project GPL license is included. A complete distributable third-party notice/source-obligation review is **not complete**; the local evaluation stage is not publication-cleared. Signing/notarization, final installer integration and formal release checksums/artifact publication remain outside this local stage.

## Runtime, IPC and exit audit

The actual executable is tested, not only library functions. CLI help/version work without a daemon. Invalid commands, empty task input and invalid TUI output mode return usage code 2. An absent daemon and a closed peer return 3; malformed JSON, oversized data, incompatible major, non-object payload and missing mandatory response fields return 4. A typed valid fixture status returns 0. Failed TUI initialization returns 1 and restores input. Existing typed IPC bounds/correlation/event validation were preserved; schema generation check passes.

Production recovery fixture: initially absent backend → connected/history → connection loss → reconnected/history, with no keyboard input before recovery. It uses disposable typed IPC, no real daemon or inference. Sessions remain authoritative, and no successful connection/catalog state is presented as verified inference readiness. Normal exits restore termios/alternate screen; mascot ownership/cleanup is regression tested.

**External SIGTERM remains a demonstrated blocker.** The production process exits via signal (-15 / shell status 143), bypasses Rust drops and leaves ECHO/ICANON disabled and alternate screen active. The disposable test restores its PTY afterward. This is not a normal/handled-error PASS and must be addressed with scoped graceful signal handling before unconditional RC approval. SIGKILL cannot offer cleanup guarantees. No daemon cancellation or AgentRuntime changes were made to mask forced exit behavior.

## Verification results

| Gate | Result |
| --- | --- |
| Full Rust workspace tests | PASS: 120 tests (14 CLI, 11 IPC, 21 mascot, 74 TUI), zero failures. |
| Formatting / Clippy | PASS: `cargo fmt --all --check`; workspace/all-target Clippy with `-D warnings`. |
| Debug / release builds | PASS: native arm64 workspace builds. |
| IPC schema | PASS: `python3 tools/ipc/generate.py --check`. |
| Production terminal compatibility | PASS: 29 simulated terminal scenarios, including delayed/fragmented replies, fallback, resize and automatic updates before input. |
| Production interaction journeys | PASS: 14 at six sizes, normal/ASCII and disconnected variants; local commands, drafts, model success/failure, sessions, archive and modal focus. |
| Production mascot regressions | PASS: 15; startup without input, connected/disconnected, modes, overlays, resize and owned cleanup. |
| CLI/startup/error cases | PASS: 14 actual-process cases, including malformed IPC and failed terminal initialization. |
| Relocation/resource cases | PASS: 5, including moved stage, bare binary, missing override and unrelated working directory. |
| Backend recovery | PASS: one automatic disconnect/reconnect journey, no inference. |
| Local staging safety | PASS: existing destination rejected with exit 2; shell syntax valid. |
| External SIGTERM | FAIL: demonstrated cleanup blocker; separate from normal-exit PASS. |
| Physical latest candidate | PENDING; simulated output is not physical acceptance. |

Evidence and reproducible fixtures are in `docs/reviews/tui-v4a/`. Final compatibility/interaction/relocation and performance records identify SHA-256 `6229bccc…d06cc81`. Tests use disposable typed IPC rather than a changed daemon.

A restricted-shell CLI fixture initially could not bind a Unix socket; the authorized local-socket rerun passed. Early exact termios comparisons counted macOS PENDIN as an error; source inspection and field-level comparison corrected the fixture. Earlier exploratory stages remain ignored local evidence; the named rc-stage and final-hash results are authoritative. Personal-Brain was unavailable; the supplied workspace and local architecture/security/build/testing instructions were used.

## Performance

Actual release process on this macOS arm64 host, diagnostics disabled, disposable typed IPC and simulated Kitty. Each mode has a four-second steady window after initialization; CPU uses Mach timebase conversion (125/3 ns) and is cross-checked against `wait4`. RSS is process peak from `wait4`. Input/resize figures are output-byte completion latency, not physical display latency. The long case contains 1,500 transcript lines and eight mode/overlay cycles. Terminal emulator/GPU CPU is excluded. One sample per case; this is not a statistical or freshly paired baseline benchmark.

| Presentation | CPU | Peak RSS MiB | Max input ms | Max resize ms | Steady draws / 4s |
| --- | ---: | ---: | ---: | ---: | ---: |
| Off / empty | 0.46% | 9.36 | 1.54 | 3.83 | 0 |
| Static / empty | 0.51% | 12.31 | 1.49 | 3.84 | 0 |
| Animated / empty | 1.72% | 18.38 | 1.65 | 3.62 | 32 |
| Animated / long transcript | 0.51% | 20.72 | 0.40 | 2.93 | 32 |

Off/Static produced zero steady output bytes or draws. Animated cases produced 32 draws over four seconds, consistent with the 8 FPS target. No steady full-screen clears occurred. First-loop uploads of previously unused cached frames were observed (4 empty / 6 long); the subsequent warm observation had zero late uploads. This is not repeated PNG decoding. Long-scroll completion peaked at 0.42 ms in the fixture. Eight cycle RSS samples rose from 20.55 to 20.69 MiB and then plateaued; this bounded sample is not a long-running leak guarantee.

Recorded V3.4 historical samples were Off 0.56%, Static 0.68%, empty Animated 1.87%, long Animated 2.17%. Current samples do not indicate a regression, but run-to-run variability and the absence of a fresh paired baseline preclude attributing a speedup to these changes. No performance optimization was added in V4-A. Full measurements are in `performance-results.json`.


## Platform and physical acceptance matrix

| Environment | Result and limit |
| --- | --- |
| macOS Apple Silicon | Native arm64 debug/release builds and actual-process CLI/PTY tests pass. Physical latest-release UX remains pending. |
| Ghostty | User-confirmed earlier CW-04.5 physical initial-image PASS. Final V4-A physical launch/interaction/signal acceptance pending; simulated Kitty tests pass. |
| Apple Terminal.app | User reports readable text fallback. Earlier computer-use tool explicitly blocked Terminal.app access; no bypass/retry here and no new physical capability capture/PASS. Simulated no-graphics and delayed replies pass. |
| Linux terminals | Portable Rust paths and response-based policy audited; local macOS fixtures pass. Existing CI declares Ubuntu/macOS targets, but this phase did not execute Linux CI/build/physical tests. Only aarch64-apple-darwin is installed locally. |
| Native Windows Terminal | **Backend transport unavailable**: non-Unix Client explicitly reports Windows named-pipe IPC not implemented. Text graphics fallback alone does not make native Windows CLI/TUI operational. No Windows build or physical PASS. |
| SSH / tmux / screen | Existing no-probe/text safety gates pass in simulation; physical remote/multiplexer acceptance pending. |
| ASCII / NO_COLOR / reduced motion | Automated gates/player tests and production PTY fallbacks pass; host contrast/keyboard behavior still requires observation. |

Shift+Enter remains terminal-encoding dependent. Ctrl+O and Alt+Enter are existing multiline alternatives; no universal Shift+Enter claim.

## Modified files

- `cli/crates/sentinel-tui/src/mascot.rs`: executable-relative lookup and relocatable-path regression.
- `cli/crates/sentinel-tui/src/lib.rs`: initialization guard ordering and truthful terminal error classification only.
- `tools/terminal/stage-release.sh`: local stage-only helper.
- `docs/interfaces/CLI.md`, `cli/crates/sentinel-mascot/README.md`: installation/resource guidance and removal of obsolete timer/paste descriptions.
- This report and `docs/reviews/tui-v4a/`: reproduction/validation artifacts.

All other preexisting uncommitted files, including concurrent C++/AgentRuntime work, were preserved. No C++ daemon, provider, AgentRuntime, Desktop/Quick Panel, icon or Blender changes in this phase.

## Remaining gates and recommendation

**Needs targeted correction**, not an unconditional RC-ready claim:

1. Fix and revalidate external graceful termination without changing daemon/AgentRuntime semantics. Forced signal cleanup currently fails.
2. Complete physical latest-release Ghostty and Terminal.app acceptance. Linux physical verification remains unexecuted; native Windows transport is explicitly unavailable and must stay out of supported release claims.
3. Before public distribution, complete third-party license/source obligations, Rust artifact integration and platform signing/packaging verification. The local stage only supports evaluation.

Normal/handled-error automated flows are suitable for continued RC evaluation now. No V4-B or AgentRuntime work was automatically begun.

## Exact physical commands

In Ghostty, from an unrelated working directory:

```sh
cd /tmp && SENTINEL_TUI_MASCOT=animated SENTINEL_TUI_MASCOT_DIAGNOSTICS=/tmp/sentinel-v4a-ghostty.jsonl "$SENTINEL_REPO/docs/reviews/tui-v4a/rc-stage/bin/sentinel" tui
```

In Terminal.app:

```sh
cd /tmp && SENTINEL_TUI_MASCOT=auto SENTINEL_TUI_MASCOT_DIAGNOSTICS=/tmp/sentinel-v4a-terminal.jsonl "$SENTINEL_REPO/docs/reviews/tui-v4a/rc-stage/bin/sentinel" tui
```

Use a fresh diagnostics destination per launch; existing files are intentionally not overwritten. Start the existing daemon separately if connected-session checks are needed; the terminal client never launches a daemon/provider fallback.

1. Run the staged executable with `--version` first and confirm `1.0.0-rc.8`. Touch no keys for five seconds on TUI launch. Ghostty should show Core Warden automatically when capabilities/assets permit; Terminal.app text fallback is acceptable. Disconnected presentation stays truthful/static.
2. Inspect empty and populated views at 60×20, 80×24, 120×40 and 180×48. Page-scroll a long transcript and resize. Check Markdown/code blocks, Turkish Unicode and draft retention.
3. Open Ctrl+P/F1, navigate pickers and Esc back. Test multiline using Ctrl+O/Alt+Enter, plus Shift+Enter only where supported. Paste local settings commands; invalid slash commands must not become inference. Check session switching, model Selected versus readiness, settings scope and archive cancellation.
4. Exercise existing backend stop/restart and `/reconnect` without sending a task. Verify honest disconnect/readiness state and recovered authoritative history. No inference result is implied by catalog/connection success.
5. Open/close overlays during eligible connected animation; resize repeatedly. Check no stale images. Exit normally using Ctrl+D with an idle empty draft; verify shell echo/cursor/scrolling. Do **not** use external SIGTERM as a passing cleanup test until the documented blocker is corrected.
6. Repeat with `NO_COLOR=1 SENTINEL_TUI_ASCII=1` and separately `SENTINEL_TUI_REDUCED_MOTION=1`. SSH/tmux must retain safe fallback.

Physical results remain pending; PTY output is not physical terminal acceptance.
