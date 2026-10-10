# V3.2.3 evidence

These are disposable-fixture observations on macOS 27.0.1 arm64, starting from fbfc41d8. They are not a successful coding-Agent demonstration. See the [acceptance report](../../development/TUI_V3_2_3_AGENT_ACCEPTANCE.md) and [execution design](../../development/TUI_V3_2_3_EXECUTION_ENVIRONMENT.md).

## Evidence classes

- `cmake-before.log`, `cmake-absolute-before.log`, `fork-before.log`: repeated real gateway/executor/sandbox integration probes before production changes. The helper uses explicit test-fixture permission, not model autonomy. Its source is in `reproduce/process-probe.cpp`; no new production execution path was added.
- `nemotron-before/`: real pre-change release TUI A run, forwarded unchanged to the original local server. Doom-loop failure rather than the historical empty-response failure.
- `nemotron/`, `qwen/`: live release TUI A–E runs, each in private portable profiles and synthetic workspaces. `live.json` retains public IPC states, hashes, actual synthetic file contents/diffs and exact TUI approval decisions. A captured terminal event is required; an observer timeout is never a pass.
- `*-host-baseline.json`: host configure/build/CTest evidence establishing D's failing fixture. These commands are **not Agent build/test verification**.
- `provider-models-before.json`, `*-loaded.json`: actual LM Studio catalog/loaded-instance snapshots. Published max_context_length is not the loaded limit. Both compared instances explicitly report 8192 context and four slots. Model formats/engines differ.
- `live-relay/provider-proxy.jsonl`: allowlisted metadata from unchanged forwarded request/response bytes. Raw prompts, response text, reasoning and credentials are never persisted by the relay. Numeric reasoning-token/byte counts are not reasoning text. Requests without a completed HTTP response are absent from this response-only log; absence is not proof of no request.
- `*-before.log`: real failing regressions before fixing output-limit classification, path guidance and successful HTTP status loss. `llm-final-tests.log` / `tool-schema-final-tests.log` validate the fixes through local HTTP fixtures and actual registry contracts. Deterministic response fixtures are not real model acceptance.
- `cpp-*`, `rust-*`: separate source build/test gates. `cpp-format.log` is empty when changed C++ ranges are formatted. Rust source is unchanged; all existing 73 tests were exercised.
- `*.ansi`: actual 120×40 PTY byte streams from the release TUI. `*.ansi.png` / `*.ansi.txt` are bounded VT replays, not native terminal screenshots. Replays can approximate glyph width/terminal rendering. Physical hosts remain NOT VALIDATED.

## Limits and provenance

Both comparison profiles copy the same frozen instrumented daemon. The subsequent HTTP-status and directory-guidance fixes were not retrospectively credited to that binary. Earlier successful HTTP responses appear as HTTP0 in its native diagnostics; the relay records the actual HTTP200. Its generic transport_failure outcome may carry a Cancelled category; use the category and terminal event together. The final source distinguishes cancellation/timeouts/rejections explicitly.

The diagnostic relay retains upstream non-streaming work until it returns or its 180-second timeout. It cannot be assumed to propagate a downstream cancellation to the inference engine. Cancelled-client broken-pipe/traceback noise in the observer log is a harness limitation, not a Sentinel tool failure. Provider causal conclusions require direct-endpoint checks; elapsed times under concurrent developer builds are not controlled performance measurements.

The native request estimate is labelled utf8_bytes/3+256. A local estimated-context refusal has transport_attempted=false and no actual usage for the unsent request. It must not be presented as a measured provider context rejection. The historical V3.2.2 empty reply still has no finish reason; new synthetic length fixtures do not explain that historical event.

## Reproduction

Build the normal release daemon and Rust client. With a local LM Studio server, both models already installed, no loaded model, and explicit approval for Qwen, run from the repository root:

```sh
V323_APPROVE_QWEN=1 sh docs/reviews/agent-v323-2026-10-10/reproduce/run.sh
```

This copies one current daemon binary for both models, loads them sequentially at 8192/four slots, creates new temporary profiles/fixtures, drives actual TUI approvals only for the named synthetic scope, and unloads only task-owned model IDs. It refuses to alter an initially loaded user model. The published harness uses the current binary, so a rerun is new evidence, not a byte-identical recreation of the earlier frozen build. The script is syntax-checked; the observed live run used the original equivalent scripts retained through these sources, with the frozen binary source path subsequently made configurable for reproducibility.

F is deliberately not simulated: no safely running macOS build exists to cancel under current confinement. The probe is not a workaround for that blocked acceptance journey. Linux comparison requires an actual Linux runner.


Direct transport controls (`direct-*-probe.json`) use the same synthetic two-word request, output bound 128, direct local HTTP and a fresh explicitly loaded 8192/four-slot instance for each model. Both returned actual HTTP200/finish_reason=stop. They are not Agent runs, have no tool authority, and do not establish coding capability. Reproduce separately after unloading task-owned models:

```sh
V323_APPROVE_QWEN=1 sh docs/reviews/agent-v323-2026-10-10/reproduce/direct_controls.sh
```

`cpp-final-ctest.log` is NOT fully green: 121/122 pass, with test_upgrade explicitly terminated while waiting in the user's macOS Keychain. `upgrade-keychain-stack.log` contains only diagnostic stack frames, not secret data. `upgrade-safe-subtests.log` covers its two non-Keychain subtests. No Keychain access approval or data/permission change was made. Final schema target rebuilds include an explicit algorithm header only; its nine cases pass again.

A final-source direct TUI control was added after the transport checks, without the relay and after developer builds finished. `nemotron-direct/` runs A; `qwen-direct/` runs A, then B–E only if A yields an actual tool result. This adaptive gate does not fabricate a successful tool sequence or mark unrun cases as passes. The same final release daemon is copied into both private profiles. Reproduce that separately with:

```sh
V323_APPROVE_QWEN=1 V323_DIRECT_FLOW=1 sh docs/reviews/agent-v323-2026-10-10/reproduce/run.sh
```

Nemotron's direct A receives AgentLoop run.completed after six reads and no edits, but its visible final is a file receipt/excerpts, not the requested explanation. It is a partial user-journey result. The native C++ metadata now shows real HTTP200, providing live confirmation of final status propagation. Model sampling, late guidance, absence of relay and background load differ from the earlier observation; do not attribute changed completion solely to a code fix.


Qwen's final-source direct A also reaches the 240-second observer bound and actual run.cancelled with zero tools. Its direct B–E gate is not entered. qwen-direct-classification-stack.log isolates the owned daemon's wait in plain intent classification, before native planning. This direct stall is not solely a relay effect; its underlying provider latency/finish reason is unknown. The initial matrix and the two final-source controls are distinct entries in acceptance-summary.json.

The original before capture, final Nemotron receipt and final Qwen cancellation replays were visually inspected. Header/composer structure remains; no physical terminal acceptance or visual redesign is claimed. cleanup.json records no owned daemon/relay and no loaded task model after restoration. linux-runner-status.log independently confirms the missing Docker daemon socket.
