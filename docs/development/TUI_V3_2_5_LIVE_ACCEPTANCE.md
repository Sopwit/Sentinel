# V3.2.5 live read-only acceptance

Date: 2026-10-10–11 (Europe/Istanbul; trace timestamps are UTC). Final rates are recorded below after the complete batches.

## Environment and provenance

Real repository release Rust/Ratatui TUI in a 120×40 PTY, talking to a frozen release C++ daemon through private Unix sockets. Each daemon uses a unique portable profile beside its copied executable; every case uses a separate session/workspace containing synthetic C++ sources only. Daemon and CLI report `1.0.0-rc.8`; IPC handshake is 1.1. No global Cargo CLI executable was invoked.

The baseline is `695244b73b49395a4383f45ae2376164f1d3d0f7` plus the explicitly preserved evaluated patch. The first Nemotron batch uses `evaluated.patch` and daemon SHA256 `9a131587c39d97c118de78c8db773b0adecfb7c6a77816e7d82428c2bc35f4d9`. It predates final metadata/correlation additions and explicit output-limit propagation in unused Claude/Gemini branches. Final Nemotron controls/extra A and Qwen use `final-source.patch` and daemon SHA256 `11cf7495ffd2d7ce5b6731145d51d47473656175f23af66ffe3b22876fd12242`. CLI SHA256 `5ddaa87a384098fff7181c3eef7e50dd4e842c5392767d72dbcfc71264099005` is constant. Do not describe these daemon builds as identical source revisions. The local classifier/planner bounds and gateway/grounding behavior are the same between batches.

Both approved already-installed models were loaded sequentially by this task at provider-API-reported 8192 context/four slots, preserving the initial empty loaded state. Nemotron is GGUF Q4_K_M; Qwen is MLX 4bit. Reasoning defaults remain on. No model download, reasoning override, endpoint change, provider fallback, write/process grant, personal project access or personal Keychain record access was requested. Build/regression activity ran concurrently with parts of live acceptance; elapsed times are observations under that load, not controlled performance comparisons. Client abort does not prove upstream generation stopped or that a later probe saw an idle inference engine.

## Fixture facts and scoring

- A: `add(a,b)` actually returns `a-b`; a useful answer must explain subtraction using an actual successful read and matching quotes.
- B: header declares `int add(int,int)`; implementation includes that header and defines the same function with subtraction. Both observations and a meaningful relationship explanation are required.
- C: `add(2,3)` actually yields -1; correct addition is 5, while the test incorrectly expects 4. Both bugs and observed implementation/test references are required.
- D: CMake minimum 3.20, C++20, executable `check` from `calc.cpp test.cpp`, `enable_testing`, arithmetic test running `check`. No command execution is allowed.
- E: `missing.cpp` is absent. The answer must truthfully address the missing/unreadable requested file, without invented contents. A generic listing or empty-search receipt alone is partial and does not satisfy this requested response.

A runtime `run.completed` is recorded separately from successful user-task completion. Receipt-only finals count as zero explanation successes. A bounded observation timeout is followed by actual TUI Ctrl-C and the resulting terminal event is recorded. The first Nemotron batch uses a 150-second observer ceiling; final-source normal journeys use 300 seconds. Neither changes the production classifier's 30-second/512-token or planner's 120-second/1024-token bounds. Three independent A–E attempts are preserved per model, plus the additional final-source Nemotron A and cancellation/reconnection controls. No natural tool call or final answer is scripted.

## Evidence and limitations

Raw `live.json`, per-case terminal VT captures and structured summaries preserve successes and failures. Production traces contain only allowlisted metadata. IPC tool events do not always expose concrete read resources; do not infer exact path/quote mapping for a failed run from a tool name alone. Native continuation byte counts establish result transport; successful answer provenance still requires the authoritative evidence gate and fixture scoring.

Initial Nemotron A: classification HTTP200/stop, 25.124 seconds; first `read-file` requested about 69.75 seconds after the user message, actual gateway result in 7 ms. Native planning sees 5319 prompt/80 completion tokens, including 30 reasoning tokens; continuation includes one 99-byte tool result and returns HTTP200/stop, 5861 prompt/363 completion tokens (283 reasoning). The returned final is rejected by the evidence gate and repair begins. The 150-second observer cancels it safely. No accepted explanation was obtained; the rejected model answer is not reconstructed from metadata or claimed to be correct.

Two first-batch Nemotron E runs return accepted generic directory/search receipts (one root listing after glob/two directory requests, one empty scoped glob result). They provide neither invented file contents nor the requested explicit missing-file response. They are runtime completions, not user-task successes. The additional final-source A fails classification before the longer observer ceiling matters.

Deterministic localhost regressions separately exercise stall/deadline, empty output, length-finished output, malformed schema, cancellation and context refusal; these are failure-path fixtures, not live-model task successes. Actual TUI cancellation/reconnection controls are distinct from physical terminal-host validation. Native macOS build execution and Linux comparison remain separate unvalidated/blocked gates.

## Complete measured results

| Model / batch | Task attempts | Classified | Any tools | File-read results | Runtime completed | Correct requested task |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Nemotron initial | 15 | 3 | 3 | 1 | 2 | 0 |
| Nemotron final-source extra A | 1 | 0 | 0 | 0 | 0 | 0 |
| Qwen final source | 15 | 0 | 0 | 0 | 0 | 0 |
| Total | 31 | 3 | 3 | 1 | 2 | 0 |

All 30 planned A–E repeats (three per journey per model), one extra final-source A, and four cancellation/reconnection controls are recorded. Correct user-task success is **0/31 (0%)**; the two receipt-only runtime completions must not be promoted to task successes. Independent production-classifier probes are additional failures, outside this actual-TUI denominator.

| Model | Journey | Attempts | Runtime completions | Task successes |
| --- | --- | ---: | ---: | ---: |
| nemotron | A | 4 | 0 | 0 |
| nemotron | B | 3 | 0 | 0 |
| nemotron | C | 3 | 0 | 0 |
| nemotron | D | 3 | 0 | 0 |
| nemotron | E | 3 | 2 | 0 |
| qwen | A | 3 | 0 | 0 |
| qwen | B | 3 | 0 | 0 |
| qwen | C | 3 | 0 | 0 |
| qwen | D | 3 | 0 | 0 |
| qwen | E | 3 | 0 | 0 |

Every attempt, its measured duration, classifier metadata/request ID, first-tool timing, tool names, accepted state, actual answer, unchanged-hash result and terminal-event count is in [attempts.json](../reviews/agent-v325-2026-10-10/attempts.json). The lifecycle audit records no late tool results, duplicate terminal states, cross-run tool IDs or granted write/process approvals. All fixture hashes remain unchanged. Concrete read-path/repeated-read counts cannot be reconstructed from empty IPC resource arrays on failed runs; final-source diagnostics expose verified/repeated read counts when planning is reached. Missing measurements are not zero-valued successes.

## Final provider restoration and minimal control

Qwen's final minimal JSON control returns HTTP200/length in 28.28 seconds, 18 prompt/511 completion/529 total tokens, with all 511 completion tokens reported as reasoning and no visible JSON. This is a failed bounded output contract, not a successful classifier response. The actual production policy rejects length-finished output. This additional transport probe is outside the TUI-task denominator. Final `models-after.json` confirms no loaded instances, restoring the initial state. No upstream generation-stop confirmation is inferred from client cancellation or model unload.
