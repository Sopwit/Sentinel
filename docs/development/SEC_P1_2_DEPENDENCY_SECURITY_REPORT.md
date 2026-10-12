# SEC-P1.2 — Rust dependency security and compatibility

Date: 2026-10-12. Baseline HEAD: `541b8db91902066840093425109e9d9d93ae3b5c`.

Final recommendation: **BLOCKED**. No dependency or production-source correction is retained. The maintained upstream candidate removed the advisory findings but introduced a reproduced Sixel/input regression. It was reverted, and a narrower redraw adaptation was rejected in temporary storage after a second compatibility failure. The original graph remains affected; **public binary distribution remains blocked**.

SEC-P1 and SEC-P1.1 were read before dependency edits. Personal-Brain was unavailable (`brain` executable absent); the explicitly supplied checkout and local architecture/runtime/security/build/test instructions were used. Existing cleanup work and concurrent changes were preserved. No AgentRuntime, C++ daemon, Desktop, IPC contract or artwork edits were made.

## 1. Current dependency graph and constraints

| Dependency | Retained version | Constraint / role |
| --- | --- | --- |
| Ratatui | 0.29.0 | Workspace `0.29`; mascot independently declares `0.29`. |
| ratatui-image | 8.0.2 | Exact mascot pin; constrains Ratatui to `^0.29.0`. |
| Crossterm | 0.28.1 | Workspace/mascot `0.28`; shared terminal backend/event integration. |
| lru | 0.12.5 | Ratatui's `^0.12.0`; both memory-safety advisories remain open. |
| paste | 1.0.15 | Ratatui's build-time proc macro; maintenance advisory remains open. |

The old graph is also the final retained production graph:

```text
sentinel-cli -> sentinel-tui -> ratatui 0.29.0 -> lru 0.12.5 / paste 1.0.15
sentinel-tui -> sentinel-mascot -> ratatui 0.29.0 -> lru / paste
sentinel-mascot -> ratatui-image 8.0.2 -> ratatui 0.29.0 -> lru / paste
```

The separately locked historical probe depends directly on the same old Ratatui/image/Crossterm versions and on the shared mascot. Both manifests and lockfiles were restored. The production lock retains 135 external package/version entries. No unsupported lockfile-only substitution or second direct patched lru was used.

## 2. Advisory assessment

| Advisory | Verified affected conditions | Path, disposition and evidence |
| --- | --- | --- |
| [RUSTSEC-2026-0002](https://rustsec.org/advisories/RUSTSEC-2026-0002.html) | Mutable LRU iteration invalidates internal aliases. Affects 0.12.5; patched `>=0.16.3`, unaffected `<0.9.0`. | All production paths above reach the affected package through Ratatui. **Deferred, still 0.12.5**. Candidate 0.18.5 was within the patched range but its upstream integration was not compatible. |
| [RUSTSEC-2026-0253](https://rustsec.org/advisories/RUSTSEC-2026-0253.html) | A panicking key destructor in `pop`, caught unwinding and later cache reuse/eviction can leave invalid list nodes. Affects 0.12.5; patched `>=0.18.2`. | Same dependency paths. **Deferred, still 0.12.5**. Candidate 0.18.5 passed range checks, but compatibility failed. |
| [RUSTSEC-2024-0436](https://rustsec.org/advisories/RUSTSEC-2024-0436.html) | `paste 1.0.15` is unmaintained; no patched version. This is a build-time maintenance advisory, not a demonstrated application exploit. | Same Ratatui paths. **Deferred, still 1.0.15**. Upstream 0.30.2 removes it, but that migration is rejected in its tested forms. |

The standalone probe has the same three open package findings. Historically compiled probe outputs also remain unchanged and are not patched by this investigation. Tests passing does not close any retained advisory.

### Reachability review

The review covered more than one call site. Reverse dependency trees identified Ratatui as the only lru consumer in the old production graph. Source-wide searches covered Sentinel Rust crates, Ratatui 0.29.0 and ratatui-image 8.0.2. Ratatui's only `LruCache` use is a private thread-local layout cache keyed by `(Rect, Layout)`, with segment/spacer values.

Reviewed construction, `get_or_insert`, capacity resize, eviction and destruction, plus the corresponding lru implementations. `get_or_insert` reaches internal replacement/detach/attach; resize reaches `pop_lru`/`remove_last`; destruction drains the map. These inspected paths do not invoke LRU `IterMut::next`/`next_back` or `LruCache::pop`. Ordinary text-vector `.iter_mut()` calls were distinguished from LRU mutable iteration. The layout key owns constraints and scalar geometry rather than application-supplied destructor callbacks. No `catch_unwind` call was found in the inspected application/Ratatui/image source.

**The published trigger conditions were not identified in the inspected application paths.** This is constrained source/graph evidence, not whole-program formal reachability proof, Miri proof or an “unreachable” exemption. Both memory-safety advisories remain affected-package findings and release blockers. No exploit was demonstrated, and no risk waiver is inferred from current usage.

## 3. Verified upstream options

Public registry metadata and source archives were downloaded to temporary storage, with archive SHA-256 checked against registry checksums. Nothing derived from the private local inventory was uploaded.

| Option | Result |
| --- | --- |
| Update only lru under Ratatui 0.29 | Unsupported: `^0.12` cannot select patched 0.18. Adding a direct 0.18 dependency leaves the affected transitive copy. |
| Image 8.1.1 / 9.0.0 | Inspected manifests still require Ratatui 0.29; cannot remove these findings. |
| Ratatui 0.30.2 / core 0.1.2 | Maintained published upstream solution for both package families: core accepts lru 0.18, resolves to 0.18.5; paste absent. Evaluated, then rejected for compatibility below. |
| Image 10.0.0 | First inspected stable image release accepting Ratatui 0.30 while keeping public Kitty image IDs used by Sentinel. Exact-pinned in the candidate to avoid an unrelated player rewrite. |
| Image 10.0.8 / latest stable 11.1.0 | Later API hides `StatefulKitty.unique_id`. An isolated 10.0.8 compile confirmed this breaks current encoding/owned-ID cleanup. Not selected for broad player migration. |
| Crossterm 0.28 with Ratatui 0.30.2 | Isolated resolution selected both 0.28 and 0.29 because the Ratatui backend keeps its default 0.29 feature. Candidate aligned direct event integration to 0.29. |
| `pastey` / another maintained macro alternative | RustSec lists alternatives, but Ratatui's package identity cannot be replaced by a simple lock update. A wrapper/vendor edit would add maintenance burden. |
| Narrow vendor/backport | Technically plausible: retain Ratatui APIs while reviewing/backporting both lru fixes and replacing paste through supported source changes. **Not implemented**: requires unsafe-code/panic-safety verification, notices, source provenance, patch lifecycle and compatibility evidence. No permanent private fork justified yet. |
| Defer with release blocking | **Selected final disposition** because no tested upstream candidate preserved required behavior. |

The evaluated candidate used Ratatui 0.30.2, core 0.1.2, ratatui-image 10.0.0, Crossterm 0.29.0 and lru 0.18.5; paste was absent. Its constraints supported the regenerated lockfiles. Explicit features retained layout caching, underline color and macros while avoiding new default calendar widgets. The mascot used workspace Ratatui/Crossterm constraints in that candidate; this refactor was also reverted.

Candidate lock expansion was material: 125 entries unchanged, 14 removed, 129 added, yielding 250 external entries; optional backend/platform dependencies account for much of the expansion. The active candidate macOS normal/build tree had 121 unique external packages, versus the prior audit's 106. [lock-delta.json](sec-p1-2/lock-delta.json) and [macos-build-packages.json](sec-p1-2/macos-build-packages.json) describe the **rejected candidate**, not the retained graph.

## 4. Compatibility experiments and scoped reversion

### Candidate A — upstream graph with narrow image API adaptations

The candidate compiled and passed all 122 Rust tests, formatting, Clippy, debug/release builds, 15 CW-04.5 player cases, three no-input comparisons and 18 external-signal cases. It also passed a terminal run without installed assets. Those successes did not cover the failing asset-enabled Sixel path.

With canonical assets available, the 120×40 Sixel test failed with a cursor-position timeout. Source inspection established a changed [upstream terminal contract](https://docs.rs/ratatui-core/0.1.2/ratatui_core/terminal/struct.Terminal.html#method.clear): Ratatui 0.30.2 `Terminal::clear()` now asks the backend for cursor position before clearing. Sentinel's existing Sixel full-redraw branch calls it while its input reader is active. The old 0.29 fullscreen clear does not read stdin. This is a real bounded-input compatibility regression, not an advisory-range problem.

All eight applied candidate manifest/lock/source files were restored from verified private originals. No unrelated work was reverted.

### Candidate B — isolated fullscreen redraw adaptation

A second candidate stayed outside the repository. Its single TUI adaptation replaced fullscreen `clear()` with the documented `resize(screen)` path, which clears the viewport and resets the previous buffer without a cursor query. It retained the current query suffix guard, 300 ms/4096-byte bounds, cell validation, cache limits, player timing, owned IDs and signal cleanup. Deprecation allowances were local to the cell-size constructor and its cleanup test; no broad warning suppression was used.

All 122 Rust tests passed. Asset-enabled Sixel worked at 60×20 and the earlier cursor query disappeared. At 120×40, however, the paste/input sequence failed to show its expected second line and cleanup then timed out. The failure persisted with a bounded one-second text-state wait while draining graphics. No cursor-position request was present in the captured bytes. The remaining cause is **not established**; the candidate is not accepted.

A fresh unchanged baseline release passed both Sixel sizes under the same fixture family, including the original short observation behavior with corrected graphics replay. This distinguishes the candidate failure from a baseline-wide inability to execute the test. Candidate B was never applied to production files; no compatibility workaround is retained.

### Fixture handling

[run-regressions.py](sec-p1-2/run-regressions.py) runs archived fixtures with a selected binary and a new external output directory. Original harnesses and results remain unchanged. Its compatibility adapter skips complete/partial Sixel DCS payloads in text-only VT replay (the old replay skipped APC but not DCS), rejects cursor-position requests, and uses bounded one-second text-state waits. Existing assertions remain. It does not answer cursor requests, fabricate acceptance values or label failed runs as passes.

Temporary binaries need assets at their supported installed-layout path. Initial runs without that staging failed asset loading; external `share/sentinel/core-warden` links to unchanged canonical assets corrected the fixture setup. No asset-discovery or artwork edit was made.

## 5. Validation results

| Check | Retained graph | Candidate observations |
| --- | --- | --- |
| Full Rust tests | PASS: 122 (14 CLI, 11 IPC, 22 mascot, 75 TUI), plus empty doc-test suites | Both candidate forms passed 122; insufficient to establish compatibility. |
| Formatting | PASS: workspace and retained probe | Candidate A passed. |
| Clippy all targets, warnings denied | PASS: workspace | Candidate A and candidate probe passed. |
| Debug and release builds | PASS: Rust 1.94.0 | Candidate A passed both; candidate B release passed. |
| Terminal compatibility | PASS: 29 asset-enabled cases, two initial sizes plus disconnected | Candidate A cursor timeout; candidate B 120×40 Sixel paste/cleanup failure. |
| Core Warden / CW-04.5 | PASS: 15 cases; 8 FPS/bounded cache/owned-ID and state regressions in mascot tests | Candidate A also passed these Kitty-focused fixture cases. |
| No-input/key/resize comparisons | PASS: 3 cases | Candidate A passed. |
| External signals | PASS: 18 cases including immediate/startup, overlay/resize, repeated SIGTERM and SIGHUP | Candidate A passed; does not clear the Sixel failure. |
| IPC generated contract | PASS: `python3 tools/ipc/generate.py --check` | No contract changes. |
| Local advisory comparison | **FAIL: three unresolved advisories per lockfile** | Rejected candidate range checks passed; source not retained. |
| Git checks / preservation | PASS: diff whitespace, unchanged index, existing-file digest comparison | Scoped candidate restored; no tracked removal. |

The retained baseline's **65 PTY cases** preserve raw evidence in temporary private directories. [validation.json](sec-p1-2/validation.json) contains sanitized summaries and raw-result digests. No physical terminal, fresh CPU/RSS benchmark or native Linux/Windows acceptance is claimed.

Commands used for the retained workspace (each build/test target external):

```sh
cargo +1.94.0 fmt --manifest-path cli/Cargo.toml --all --check
CARGO_TARGET_DIR=/tmp/sec-p1-2-baseline-check-target cargo +1.94.0 clippy --manifest-path cli/Cargo.toml --locked --offline --all-targets -- -D warnings
CARGO_TARGET_DIR=/tmp/sec-p1-2-baseline-check-target cargo +1.94.0 build --manifest-path cli/Cargo.toml --locked --offline
CARGO_TARGET_DIR=/tmp/sec-p1-2-baseline-target cargo +1.94.0 build --manifest-path cli/Cargo.toml --release --locked --offline
CARGO_TARGET_DIR=/tmp/sec-p1-2-baseline-target cargo +1.94.0 test --manifest-path cli/Cargo.toml --locked --offline
python3 tools/ipc/generate.py --check
```

The adapter accepts `--repo`, `--binary`, `--output` and `--suite` (`compatibility`, `warden`, `comparison`, `signals`). Choose a new output path for each run and stage assets at the binary's supported runtime path. Raw diagnostics may contain local paths; keep them outside the repository.

The public RustSec archive was downloaded and compared locally against every package/version in each lockfile. Requirements were evaluated with Rust **semver 1.0.28**, including patched/unaffected ranges and excluding withdrawn records. [advisory-results.json](sec-p1-2/advisory-results.json) explicitly separates the retained affected graph from the rejected advisory-clean candidate. Production: 23 package/advisory pairs examined, three unresolved. Probe: 21 pairs, three unresolved. Candidate production/probe: 43/42 pairs, none unresolved. This is a local range comparison, not `cargo-audit` certification or universal vulnerability clearance. No repository inventory was uploaded to an external security service.

All builds/tests used **Rust 1.94.0** on `aarch64-apple-darwin`, matching Sentinel's declared workspace floor. Candidate Ratatui/core/widgets declare 1.88; image 10.0.0 declares 1.86; lru 0.18.5 declares 1.85. Candidate all-platform metadata had no available MSRV declaration above 1.94 (254 packages / 182 declarations for production; 234 / 162 for probe). Missing declarations are explicit in [candidate production metadata](sec-p1-2/metadata-msrv.json) and [candidate probe metadata](sec-p1-2/probe-metadata-msrv.json); they are not assumed compatible. Native Linux/Windows and physical terminal checks were not performed here.

## 6. Preservation and evidence impact

Digest verification covered **7,878 existing tracked/nonignored files**. Every pre-phase file matches its original digest. Both production/probe Cargo manifests and lockfiles, mascot sources and TUI source are unchanged. Existing SEC-P1/SEC-P1.1 corrections, architecture, raw evidence and assets are preserved. The index is unchanged and all **7,862 tracked files** remain tracked. The checkout is intentionally dirty with inherited work and these new documentation/support records; it is not described as clean. See [change-ledger.json](sec-p1-2/change-ledger.json).

Fresh retained release SHA-256: `c92778d566e4bcfc4aa42c51dbed5445bad26035a1bfed0f3870011971de46b9`. This temporary binary is test evidence, not a published or acceptance-cleared artifact.

Originals and rejected candidate files are retained privately under `/tmp/sec-p1-2-originals/`, `/tmp/sec-p1-2-rejected-candidate/` and the isolated candidate checkout. Raw logs, diagnostics, screens, downloaded advisory data and test outputs stay outside Git. Move needed records to approved private durable storage before temporary storage expires.

Historical acceptance results, evidence hashes, probe manifests/lock/source, tracked target outputs, screenshots, archived binaries, PNGs, Blender masters and FA-3 assets retain their pre-phase bytes. A Cargo metadata command refreshed the tracked probe `.rustc_info.json`; it was restored to independently verified pre-phase bytes. Build/test targets were placed in temporary directories. No file was untracked or deleted; no history rewrite, commit, push, merge, tag or publication occurred. SEC-P1.1's untracking proposal still requires maintainer approval.

No dependency-license clearance was issued. Candidate metadata contained license declarations, but original-notice packaging and the SEC-P1 Objective-C/Qt/toolchain/artwork/source-distribution gates were not resolved. Candidate test success cannot transfer old release acceptance to new binaries or clear public distribution.

## 7. Remaining risks and exact next actions

1. Keep all three retained advisories open and public binary distribution blocked. Do not treat the constrained reachability review as a waiver.
2. Use the preserved candidate/Sixel failures to investigate the maintained upstream transition: cursor-query contract first, then the reproduced 120×40 paste/cleanup failure. Require asset-enabled Sixel, Kitty, fallback, input/model/session/settings, resize and external-signal coverage before applying another candidate.
3. If upstream compatibility cannot be established, design a bounded temporary backport proposal with exact upstream fixes, original notices and tests for aliasing/panic safety. Review its maintenance/removal plan before choosing a private fork. No such fork or backport was silently introduced here.
4. Preserve needed originals/raw evidence privately; the report and sanitized summaries record this phase without altering historical measurements.
5. Run native Fedora/Linux and physical terminal acceptance for any eventual fixed graph. Refresh release dependency notices and address the separate privacy/distribution gates before publication.
6. Stop here. No SEC-P1.3, release publication, untracking or history remediation begins automatically.
