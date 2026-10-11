# SENTINEL — TUI V4-A.1 graceful termination

2026-10-11. The V4-A external SIGTERM cleanup defect is corrected. Recommendation: **Ready for V4-B evaluation**, subject to the existing physical acceptance and distribution gates. This is not public release approval. No V4-B work was begun.

## Root cause and correction

Unix's default SIGTERM disposition terminated the process before Rust drops could execute. Existing normal-exit and handled-error cleanup was already functional.

A scoped TUI `Termination` registers SIGTERM and SIGHUP notification through `signal-hook` (already in the resolved dependency graph, now a direct Unix-only TUI dependency). Signal actions only store a signal number in an atomic. No allocation, terminal I/O, IPC, locks or cleanup occurs in the asynchronous action; no thread or self-pipe loop is introduced. SIGINT is not registered, preserving existing Ctrl+C editing/cancellation behavior and CLI run cancellation.

Registration precedes worker creation and raw mode. The existing loop checks notification before work and after the existing input poll; notification takes precedence over an incidental interrupted-poll error. The existing maximum 50 ms poll is unchanged. Initialization/capability negotiation retains its existing bounded operations; this is not a hard real-time guarantee for terminal output or PNG-loader joining.

Termination returns through the ordinary scope. Player drop deletes owned graphics; terminal drop restores a hidden cursor; the existing `TerminalCleanup` disables bracketed paste, pops keyboard enhancement flags and calls Ratatui's raw-mode/alternate-screen restoration. Signal registrations outlive these guards and are unregistered afterward. Repeated notifications are sticky, not another cleanup path. No screen clearing, new timer, forced Kitty support, recurring capability probe or daemon cancellation was added.

External termination is reported by the existing local `Error::Failed` channel, exit **1**, with `TUI terminated by SIGTERM (15)` or `TUI terminated by SIGHUP (1)`. It is deliberately not reported as successful normal exit, model-run cancellation (130), or death by an unhandled signal (-15 / shell 143). No shared IPC error/schema variant was introduced. Normal Ctrl+D remains exit 0.

## Modified files

- `cli/crates/sentinel-tui/src/termination.rs`: scoped Unix signal notification, non-Unix no-op stub and sticky-notification unit regression.
- `cli/crates/sentinel-tui/src/lib.rs`: early registration and notification checks around the existing bounded poll.
- `cli/crates/sentinel-tui/Cargo.toml`, `cli/Cargo.lock`: direct Unix-only signal-hook dependency (resolved version 0.3.18); other existing dirty dependency changes preserved.
- `docs/reviews/tui-v4a1/`: actual-process signal harness, terminal/mascot/performance wrappers and bounded evidence.
- This report. V4-A's report remains historical evidence; its SIGTERM blocker is superseded by this correction.

No AgentRuntime, daemon, provider, Desktop, Blender or icon changes. No global install, commit, push, merge or release publication.

## Actual executable and SIGTERM evidence

Executable: `<LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel`, `sentinel 1.0.0-rc.8`, native Mach-O arm64. SHA-256: `45087e81132d23b934c88b152195f5f59f5dd0276c7639358ef8e370a690491c`. The older V4-A rc-stage was intentionally left unchanged.

**18 real-signal scenarios passed:** five immediate post-spawn attempts; alternate-screen, enhancement/paste and capability-probe startup milestones; disconnected idle, overlay and resize; connected Off/Static/Animated/Auto; ten repeated SIGTERM requests; SIGHUP idle and connected overlay. No user key is sent before signal except to open an overlay. Connected animation cases require at least four placements before termination; disconnected static presentation is not counted as animation.

The 13 handler-installed scenarios exit 1, report the correct signal, restore input and terminal modes, and cover every uploaded image ID with owned deletion. Signal-to-process-exit measured **6.29–53.73 ms**, excluding the final output-drain window. The five immediate cases exit -15 before any terminal mutation, with unchanged ECHO/ICANON. No duplicate alternate-screen/paste cleanup or global image deletion was emitted. Evidence: `signal-results.json`, `signals.log`, bounded `*-operations.json`.

The fixture creates disposable PTYs, sends real `os.kill` SIGTERM/SIGHUP to child PIDs and compares pre/post termios. It checks ECHO/ICANON explicitly and all other fields except macOS's transient PENDIN retype bit. Alternate-screen leave, cursor state, paste disable and keyboard-pop bytes are checked when applicable. Actual Kitty upload IDs are parsed independently of header field order and must be covered by owned delete IDs; no global delete is allowed. Cleanup output is bounded to one alternate-screen leave / paste disable. Full PNG payloads are excluded from retained operation evidence.

Immediate post-spawn signals can precede user-code handler registration. In those cases the process retains default signal exit and no terminal mutations have occurred. Startup milestones (alternate-screen entry, paste/enhancement setup and pending capability query) exercise externally observable race boundaries; they do not claim deterministic injection into every instruction of `try_init`.

SIGHUP is verified while the PTY remains available. If the real terminal has disappeared or writes fail, output cleanup is best effort. SIGKILL, process corruption and hard OS termination cannot guarantee cleanup. Physical absence of ghost images requires terminal observation; this phase proves emitted owned-resource deletion and restored PTY state.

## Tests and performance

| Gate | Result |
| --- | --- |
| Full Rust workspace | PASS: 121 tests (14 CLI, 11 IPC, 21 mascot, 75 TUI), zero failures. |
| Formatting | PASS: `cargo fmt --all --check`. |
| Clippy | PASS: workspace/all-targets with `-D warnings`. |
| Debug / release | PASS: native workspace builds. |
| Real external signal PTYs | PASS: 18 scenarios described above. |
| Terminal compatibility | PASS: 29 production scenarios, including fragmented/late responses, resize, keyboard/local-command paths and normal terminal restoration. |
| Core Warden | PASS: 15 production scenarios, including modes, disconnected/connected, overlay transitions and owned cleanup. |
| Diff whitespace | PASS: `git diff --check`. |

CW-04.5's removed CSI 5n query and V3.4's scoped nonblocking terminal polling were not changed. Compatibility records affirm no status query. Tests use existing disposable IPC fixtures without modifying a daemon or performing inference.

The first restricted-sandbox Rust test invocation could not bind fixture sockets; the authorized local-socket rerun passed. The signal fixture's initial cursor assertion incorrectly required a Show byte even before any Hide occurred; it now checks final cursor state. The initial image-ID parser assumed field order; it now parses complete Kitty headers and explicitly requires uploads in graphical cases. Final evidence uses the corrected assertions.

The event-loop notification checks were measured on the actual release process using the existing macOS Mach-timebase / wait4 cross-checked helper. Four-second steady windows, simulated Kitty and typed IPC, diagnostics disabled; the terminal emulator/GPU is excluded. Long transcript is 1,500 lines, followed by eight mode/overlay cycles. One sample per case, not a fresh paired statistical comparison.

| Mode / transcript | CPU | Peak RSS MiB | Steady draws / 4s | Max input output-byte latency ms | Max resize ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| off / empty | 0.70% | 9.33 | 0 | 2.16 | 3.13 |
| static / empty | 0.67% | 11.75 | 0 | 1.45 | 2.23 |
| animated / empty | 1.75% | 18.55 | 32 | 1.62 | 2.95 |
| animated / long | 2.05% | 21.30 | 32 | 3.06 | 10.88 |

Off/Static had zero steady draws and output bytes. Animated cases retained 32 draws in four seconds (8 FPS). No steady full-screen clear or warmed-frame retransmission occurred. Historical V4-A empty Off/Static/Animated measured 0.46/0.51/1.72%; long Animated 0.51%. These single-run numbers vary, so the long-run difference is not attributed to two atomic checks. The current long Animated result is also close to the recorded V3.4 2.17% sample. No new thread, poll or frame decoding was introduced. Physical input/display latency is not measured by byte receipt.

Cycle RSS samples: 21.14, 21.19, 21.22, 21.23, 21.25, 21.28, 21.28, 21.28 MiB. This short bounded observation is not a long-running leak guarantee. Evidence: `performance-results.json`.


## Platform limits and readiness

- macOS arm64: actual release execution, real external signals, restored PTY state and simulated terminal/IPC matrices verified.
- Linux: Unix implementation is portable, but no Linux build or physical run was performed in this phase.
- Windows: signal-hook dependency and implementation are Unix-only; non-Unix stub preserves the source boundary. No Windows build/physical claim, no new Windows signal mechanism; existing unavailable native named-pipe transport remains a release limitation.
- Ghostty: earlier CW-04.5 physical initial-image acceptance remains historical. Latest V4-A.1 physical graphics/exit acceptance is pending.
- Apple Terminal.app: user previously confirmed readable text fallback. Latest physical graceful termination acceptance is pending.

The demonstrated SIGTERM blocker is removed for continued V4-B evaluation. Public release still needs physical latest-candidate acceptance, distribution-license review and packaging/signing gates from V4-A. No V4-B or AgentRuntime work starts automatically.

## Physical acceptance

In Ghostty or Terminal.app, use the actual rebuilt executable (the older V4-A rc-stage is not this candidate):

```sh
SENTINEL_TUI_MASCOT=auto <LOCAL_HOME>/Desktop/Projects/Sentinel/cli/target/release/sentinel tui
```

Touch no keys for five seconds. Check automatic graphics in confirmed Kitty environments or readable fallback otherwise. In another terminal, identify this exact TUI PID with `ps -axo pid,command`, then run `kill -TERM PID`. Verify the original terminal returns to the shell with echo, canonical input and cursor functioning, alternate screen closed, no stale images, and `$?` equal to 1 with the signal message. Repeat during a connected animation and an open overlay; test normal Ctrl+D separately (0). A closed terminal/HUP is best effort; never use SIGKILL as a cleanup acceptance test.
