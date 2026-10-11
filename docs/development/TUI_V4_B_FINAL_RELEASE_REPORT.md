# SENTINEL — TUI V4-B packaging and final acceptance

2026-10-11. Recommendation: **CONDITIONAL — suitable for local RC evaluation, not cleared for public release.** The evaluation artifact is built and relocatable. Physical latest-candidate acceptance, complete dependency/resource licensing and source-distribution arrangements, Developer ID signing/notarization and distribution approval remain open. No installation, signing with credentials, notarization, publication, Git commit/push/merge/tag or next phase occurred.

## Packaging implementation

`tools/terminal/package-evaluation.py NEW_OUTPUT_DIRECTORY` reuses `stage-release.sh`, the existing release executable and canonical Core Warden resources. It does not build, install or publish. It refuses an existing output, validates native Mach-O arm64 and obtains the version from the executable. Cargo metadata/tree are read offline with the lockfile for the macOS arm64 normal/build dependency graph. No application version changes were made.

The helper adds relevant standalone evaluation guidance, CLI and mascot documentation, project GPL text, original available third-party license/notice files and an explicit unresolved-obligations document. It produces sorted file hashes/sizes/modes in `INVENTORY.json`, internal `SHA256SUMS`, and a `.tar.gz` plus external checksum. Inventory excludes itself; internal checksums cover inventory and all other files except the checksum file itself. The archive has sorted entries, fixed owner IDs/names, fixed timestamps and gzip timestamp zero; **independent bit-for-bit build reproducibility was not verified**. This is reproducible packaging procedure/inventory, not a reproducible-build claim.

The native linker ad-hoc signature is retained unmodified; no user credentials were accessed. Only macOS system libraries/frameworks are dynamically linked; none are copied. The daemon, models, credentials and backend configuration are separate and not included. The artifact cannot independently provide inference.

## Artifact and integrity

Local archive: `<LOCAL_HOME>/Desktop/Projects/Sentinel/docs/reviews/tui-v4b/artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation.tar.gz`.

Extracted evaluation directory: `<LOCAL_HOME>/Desktop/Projects/Sentinel/docs/reviews/tui-v4b/artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation`.

- Version: **1.0.0-rc.8**, native Mach-O arm64.
- Archive size: **4,791,016 bytes** (4.57 MiB).
- Unpacked files: **342**, total **9,074,528 bytes** (filesystem allocation differs).
- Archive SHA-256: `a0ca836726364dd9e4c72eaad83c5081b4c005c58d1417b5bfcb476e14b0a33f`.
- Executable SHA-256: `45087e81132d23b934c88b152195f5f59f5dd0276c7639358ef8e370a690491c` — matches the verified V4-A.1 release, no runtime source changes.
- Review evidence: `package-summary.json`, `inventory.json`, `artifact-SHA256SUMS`, `package-audit.json`, `dependency-licenses.json`, `signing-audit.txt`, `linked-libraries.txt` in `docs/reviews/tui-v4b/`.

Contents:

- `bin/sentinel` — actual release executable.
- `share/sentinel/core-warden/state-contract.json` and canonical animation manifest/132 PNGs.
- `share/licenses/sentinel/LICENSE` and available original third-party texts.
- `share/doc/sentinel/CLI.md`, `CORE_WARDEN.md`, standalone `README.txt`.
- `THIRD_PARTY_NOTICES.md`, `dependency-licenses.json`, `DISTRIBUTION_GATES.md`, inventory and checksums.

No duplicate PNG asset tree, Blender models, concept boards, source render batches, Desktop or FA-3 icon changes. All 132 packaged PNG bytes match canonical inputs.

Seven actual relocated-package journeys passed: extracted archive under an unrelated path with spaces; read-only installation (write access denied and hashes unchanged); missing resources; invalid manifest; invalid nonempty PNG data; traversal in contract manifest path; escaping frame symlink. Valid installations load actual frames. Corrupt PNGs pass file-presence validation but decoding fails safely with zero cached frames; malformed/missing/path-invalid inputs fail earlier and remain text fallback. All cases retain keyboard/local-command, resize and normal terminal cleanup behavior. Explicit override semantics and path containment remain unchanged.

Archive member names/types are checked for duplicates, absolute/traversal paths, links and unexpected types before extraction. All checksum entries and inventory hashes/sizes are verified after extraction. This proves integrity of the produced archive; runtime does not cryptographically authenticate assets on every launch. Startup from arbitrary working directories never uses developer-home asset lookup.

## Dependency and resource licensing

The conservative locked macOS arm64 normal + build graph contains 110 workspace/dependency packages, including **106 third-party crates**. Dev-only and other-platform-only packages are excluded; build-only tools may be included conservatively. Cargo metadata is a declaration audit, not proof of which source symbols are linked. Original files are copied byte-for-byte, retaining copyright/permission/notice text. No generic license text is fabricated to fill gaps and no license choice is silently made.

Most metadata declares MIT/Apache alternatives; other expressions include Zlib, BSD alternatives, Unlicense, 0BSD, BSL-1.0 alternatives, LLVM exception alternatives and `(MIT OR Apache-2.0) AND Unicode-3.0`. `icy_sixel 0.1.3` has no SPDX license field but does provide `license_file = LICENSE`; its original permission/copyright text is included without inventing a metadata declaration. License expressions using historical slash notation are preserved literally.

**Six missing original texts:** `base64-simd 0.8.0`, `block2 0.6.2`, `dispatch2 0.3.1`, `objc2 0.6.4`, `objc2-encode 4.1.0`, `vsimd 0.8.0`. Their metadata declares licenses, but their locked local crate contents do not contain corresponding standalone texts. Exact-version upstream verification and inclusion remain required before public distribution. This phase did not download or invent replacement declarations.

Attribution/compatibility status: available copyright/permission texts and notices are included; compatibility and chosen-license compliance are **not certified**. Apache/Unicode/BSD and alternative-license terms require review in context. No positive GPL-incompatibility finding was established from this metadata audit, which is not equivalent to clearance. The project GPL's Corresponding Source requirements (see included LICENSE, especially sections 1 and 6) need an actual compliant source/build/install distribution arrangement; this archive contains neither complete source nor a formal source offer. Rust toolchain/standard-library notices and any source obligations beyond Cargo packages also remain to be reviewed.

Core Warden is supplied project artwork; the root project GPL text is bundled, but a complete ownership/reference provenance and redistribution-permission audit was not established. No artwork license declaration beyond existing project material was invented. The package's `DISTRIBUTION_GATES.md` records these open items. Dependency licensing gate therefore remains **OPEN**, and public redistribution is blocked pending resolution.

## Regression and performance

| Check | Result |
| --- | --- |
| Full locked Rust workspace tests | PASS: 121 tests, zero failures (14 CLI, 11 IPC, 21 mascot, 75 TUI). |
| Formatting / Clippy | PASS: fmt check; locked workspace/all-target Clippy with `-D warnings`. |
| Debug / release builds | PASS: locked native arm64 workspace builds. |
| Packaged terminal compatibility PTYs | PASS: 29 scenarios. |
| Packaged interaction PTYs | PASS: 14 journeys across six sizes, including ASCII/disconnected paths. |
| Packaged mascot PTYs | PASS: 15 scenarios. |
| Packaged real signal PTYs | PASS: 18 scenarios; SIGTERM/SIGHUP, startup/idle/playback/overlay/resize/repeated requests. |
| Extracted archive / resource integrity | PASS: seven journeys plus inventory/checksum/member verification. |
| Packaging safety | PASS: existing destination refused with exit 2; no overwrite/install. |

All matrices invoke the packaged executable, not a separate fixture player. IPC and terminal responses are simulated; signals are real OS signals and termios is actually measured. Immediate signals before handler installation preserve default signal exit while the terminal is unmodified; after registration graceful exits report signal and exit 1. Raw ECHO/ICANON, alternate-screen, cursor, paste/keyboard state and owned image deletion are checked. No public daemon/inference capability is inferred from fixture success.

Actual packaged process on macOS arm64, four-second steady windows after startup, diagnostics disabled, disposable typed IPC/simulated Kitty. Mach timebase CPU is cross-checked against wait4; peak RSS is wait4 process peak. Terminal emulator/GPU work is excluded. Latencies below measure output-byte receipt, not physical display. Long transcript has 1,500 lines and eight mode/overlay cycles. One sample per case; no statistical reproducibility claim.

| Presentation | CPU | Peak RSS MiB | Steady draws / 4s | Max input ms | Max resize ms |
| --- | ---: | ---: | ---: | ---: | ---: |
| off / empty | 0.37% | 9.31 | 0 | 1.97 | 3.40 |
| static / empty | 0.40% | 12.31 | 0 | 1.83 | 3.35 |
| animated / empty | 1.73% | 17.89 | 32 | 1.37 | 3.43 |
| animated / long | 1.86% | 21.20 | 32 | 2.62 | 10.80 |

Off/Static produced zero steady draws or output bytes. Animated cases produced 32 draws in four seconds (8 FPS). No steady full-screen clears or warmed-frame retransmissions occurred. First-loop previously unused frame uploads are expected and are not repeated decoding. Runtime code/binary is byte-identical to V4-A.1; only resources/notices packaging changed. Its recorded CPU samples were 0.70/0.67/1.75% for empty Off/Static/Animated and 2.05% for long Animated. Current samples do not show a material resource regression; this is historical comparison, not a fresh paired statistical test. No optimization was introduced.

Long-cycle RSS samples: 21.08, 21.12, 21.14, 21.16, 21.19, 21.19, 21.19, 21.19 MiB; this short observation is not a long-term leak guarantee. Full data: `performance-results.json`.


## Release gates

| Gate | Status | Evidence / limit |
| --- | --- | --- |
| 1. Build and tests | PASS | Native arm64 Rust gates and packaged PTY matrices below. |
| 2. Package integrity | PASS | Architecture/version, archive/member checks, SHA-256 and inventory verified. |
| 3. Resource relocation | PASS | Seven extraction/space/read-only/corruption/path-safety journeys. |
| 4. Dependency licensing | OPEN | Six original texts missing; source/toolchain/resource obligations not cleared. |
| 5. Ghostty physical acceptance | PENDING | Prior CW-04.5 initial graphics acceptance is historical; latest package not physically observed. |
| 6. Apple Terminal physical acceptance | PENDING | Prior user report confirms text fallback; latest package/signal UX not physically observed. |
| 7. Signal cleanup | PASS (PTY) | Real external SIGTERM/SIGHUP on packaged executable; physical visual cleanup pending. |
| 8. Signing/notarization readiness | OPEN | Linker ad-hoc only, no TeamIdentifier/Developer ID/notarization; not Gatekeeper-approved. |
| 9. Supported-platform declaration | PASS (scope) | Only native macOS arm64 artifact; no native Linux or Windows release claim. |
| 10. Remaining known issues | OPEN | Physical, license/source and signing gates; daemon required; terminal encoding limits. |

## Platform boundaries and physical checklist

Native macOS arm64 release execution and actual PTY processes are verified. Linux-like terminal profiles on macOS PTYs are **simulation**, not native Linux execution. No Linux build or physical terminal run occurred. Windows native IPC transport is not implemented; this package does not support Windows TUI. SSH/tmux/screen safety gates are simulated only. Shift+Enter depends on terminal encoding; existing Ctrl+O/Alt+Enter remain alternatives.

Physical GUI access was unavailable for this phase; earlier terminal automation access was blocked and was not bypassed. No physical PASS is claimed from PTY bytes. The available automated checks above were executed instead.

From Ghostty (use a fresh diagnostics destination):

```sh
cd /tmp && SENTINEL_TUI_MASCOT=animated SENTINEL_TUI_MASCOT_DIAGNOSTICS=/tmp/sentinel-v4b-ghostty.jsonl <LOCAL_HOME>/Desktop/Projects/Sentinel/docs/reviews/tui-v4b/artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation/bin/sentinel tui
```

From Apple Terminal.app:

```sh
cd /tmp && SENTINEL_TUI_MASCOT=auto SENTINEL_TUI_MASCOT_DIAGNOSTICS=/tmp/sentinel-v4b-terminal.jsonl <LOCAL_HOME>/Desktop/Projects/Sentinel/docs/reviews/tui-v4b/artifacts/sentinel-cli-1.0.0-rc.8-macos-arm64-evaluation/bin/sentinel tui
```

1. Confirm `--version` is `1.0.0-rc.8`. Touch no keys for five seconds after launch. Ghostty should present graphics only when confirmed; Terminal.app readable fallback is expected. Disconnected state stays truthful/static.
2. Start the existing daemon separately for connected tests, or pass `--socket PATH`. Check normal and disconnected startup and reconnect. Verify animation only in legitimately connected eligible state.
3. Run local `/settings mascot static`, `/settings mascot off`, `/settings mascot animated`, `/settings mascot auto`. They must stay local. Check empty/populated views and resize 60×20 → 80×24 → 120×40 → 180×48.
4. Open Ctrl+P/F1 and session/model/settings overlays, navigate and Esc back. Confirm no stale image remnants. Enter Turkish Unicode and multiline paste; Ctrl+O/Alt+Enter preserve drafts. Do not infer readiness from a model catalog alone.
5. Exit idle/empty with Ctrl+D (0). Verify shell echo, cursor and canonical input. On fresh launches, identify the exact packaged TUI PID with `ps -axo pid,command` from another terminal, then run `kill -TERM PID` and separately `kill -HUP PID` while the original terminal remains open. Expect exit 1 and signal message, restored terminal and no stale graphics. SIGKILL has no cleanup guarantee; HUP after actual terminal disappearance is best effort.
6. Repeat with `NO_COLOR=1 SENTINEL_TUI_ASCII=1`, reduced motion and static/off launch modes. Record terminal version, dimensions, connected state and observed result separately.

Do not delete an existing diagnostics file simply to reuse its name; use a fresh path. The previous V4-A stage is not this candidate.

## Files changed and final recommendation

Only packaging tooling (`tools/terminal/package-evaluation.py`), this report and `docs/reviews/tui-v4b/` fixtures/evidence were added/updated. Existing runtime implementation, version, assets and all unrelated dirty work were preserved. The artifact directory is locally ignored and retained for evaluation; no global install occurred.

**CONDITIONAL:** local RC evaluation is supported by automated evidence. Public release is not approved until physical latest-package acceptance, license/source/artwork obligations and signing/notarization/distribution policy gates are resolved. No further phase starts automatically.
