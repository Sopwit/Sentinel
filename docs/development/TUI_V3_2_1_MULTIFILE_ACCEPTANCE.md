# V3.2.1 multi-file acceptance

## Reproducible isolation

All live work used fresh 0700 portable daemon profiles and disposable C++20/CMake workspaces. The user's running daemon/profile and project source were not task targets. Model selection was explicit: LM Studio / nvidia/nemotron-3-nano-4b, loaded context 8192. No model was loaded/downloaded/reconfigured; no fallback occurred.

Fixture: calc.h declares add; calc.cpp initially computes a-b; test.cpp initially expects add(2,3)==4. CMake builds check and registers arithmetic with CTest. unrelated.txt contains USER_CHANGE_PRESERVE_821. README/config carry PORCELAIN-821 and GLACIER-821 markers. The model actually chose native tools; the harness did not script a planner/tool sequence. Approvals allow only exact canonical calc.cpp/test.cpp paths or SHA256 digests of the named verification commands. Other requests are denied.

## Initial bounded benchmark

| Task | Outcome | Evidence / limits |
|---|---|---|
| A: two-file context | LIVE E2E PASS | Two real reads, grounded final includes both scoped marker values. Evidence presentation is factual excerpts rather than fluent synthesis. 144.67 s. |
| B: interface/implementation/test reasoning | PARTIAL | Glob plus two reads; no accepted final within 180 s; observer cancels. |
| C: bounded function and test edit | PARTIAL | Grep and two reads; no edit or approval before 240 s bound. Diff empty. |
| D: authorized configure/build/test | FAIL | Context exhaustion before a tool ran; outer provider HTTP 400, nested engine code 500. No build/test result. |
| E: near-capacity multi-step context | PARTIAL | One directory listing; no accepted final within 180 s. |
| F: ongoing command cancellation | PARTIAL | Cancelled terminal state; process launch/owned PID not proven. See cancellation report. |

Every initial benchmark diff was empty; unrelated fixture preservation is observed but does not establish preservation across successful edits. No full multi-file coding/verification acceptance is claimed. Provider/tool failures did not become completed run states. Timed-out observers are recorded as observer-timeout even where a cancellation terminal later arrives.

## Direct-endpoint follow-up

The original forwarding proxy records request metadata but does not propagate upstream cancellation while reading. A fresh direct-endpoint benchmark removes that instrumentation limitation and uses longer bounds: B 420 s, C 600 s, D 360 s. Its results and actual diffs are in extended-live.json and the final report. Direct follow-ups do not provide request token telemetry. No uncertain write is blindly replayed; each benchmark is a separate disposable fixture.

## Remaining requirements

Acceptance still requires a natural task that applies both intended edits, preserves the unrelated marker, runs approved verification successfully and receives a grounded accepted final. A successful standalone unit test or an assistant assertion is not that evidence. No automatic Git operation, unsafe rollback or implicit context compaction was added.

The direct B follow-up exceeded 420 s after 11 tool results; no accepted reasoning final. Direct C failed after 284.87 s because the model repeated an unchanged call without an intervening observation. The existing duplicate-call guard rejected it. No write approval or file diff occurred; unrelated.txt stayed intact. This is safe failure, not successful multi-file autonomy. Changing the duplicate guard to force progress would weaken safety and was not done.

Direct D failed after 262.72 s: the model requested the exact configure command twice, each with a fresh exact-digest approval; both executions failed (182 ms and 68 ms). A later unlisted command digest was denied and the run failed with “User denied the required action.” Build and CTest commands were not successfully executed. No build directory was present in the final recursive workspace manifest. Four tracked source/marker diffs were empty; the manifest additionally records all eight fixture files. No accepted final or verified source change is claimed.

This exposes a remaining recovery boundary: model-initiated repeated process requests can recur across observations and receive new approvals. The probe allowed the same named command twice; that is not proof of safe resumability after uncertain partial effects. Durable mutation identity/outcome checks remain required. No blanket automatic retry logic was introduced in V3.2.1.
