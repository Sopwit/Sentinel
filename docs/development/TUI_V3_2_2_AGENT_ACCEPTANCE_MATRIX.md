# V3.2.2 controlled Agent acceptance matrix

Each task uses a fresh synthetic workspace. The model chooses tools; observer timeouts are not completed tasks. Full acceptance requires the entire edit → review → build → test → accepted final chain. Provider errors, tool errors, approval decisions and final run states are separate evidence.

Results are derived from `live.json`, matching `run_id` rather than collecting unrelated terminal events. Original and changed contents plus full file manifests are included. Command approvals are single-use exact digests; an unlisted or repeated command is denied.

F (cancel during an authorized build) is BLOCKED by observed CMake execution/fork denial in the gateway. The V3.2.1 live /bin/sleep cancellation is historical single-process evidence; it is not relabelled as build cancellation. No process-tree, native terminal or Windows/Linux acceptance is claimed.

## Observed results

| Journey | Status | Actual evidence / limit |
|---|---|---|
| A: inspect/explain | IMPLEMENTED + PARTIALLY VERIFIED; acceptance failed | Three real read-file calls completed. After 158s the provider returned no content/tool calls; run.failed (MalformedResponse). No accepted explanation or mutations. |
| B: two-file edit/review | IMPLEMENTED + PARTIALLY VERIFIED; acceptance failed | Actual TUI approved two exact file scopes. calc.cpp changed subtraction to addition; test.cpp edit failed. Ten tool results; observer stopped at 301s and TUI Ctrl+C produced run.cancelled. Partial work retained. |
| C: complete coding chain | BLOCKED / not accepted | Five tool results, no edits or command approvals; context exhaustion after 141s. No Agent build/test/diff verification. Independent gateway diagnostics additionally block CMake under macOS confinement. |
| D: recover failing test | IMPLEMENTED + PARTIALLY VERIFIED; acceptance failed | Separate host baseline genuinely failed the synthetic arithmetic test. One scoped approved calc.cpp edit succeeded; test.cpp unchanged. Eight tool results; context exhaustion after 227s. No recovered passing test or accepted final. |
| E: context pressure | IMPLEMENTED + PARTIALLY VERIFIED; acceptance failed | Seven tool results, repeated invalid discovery/read operations, no changes. At 241s observer timeout → actual TUI Ctrl+C → run.cancelled. No claim that pressure was handled successfully. |
| F: cancel authorized build | BLOCKED | CMake executable access and child creation denied by actual gateway sandbox. No running build existed to cancel. Historical single-process cancellation is insufficient. |

All five fixtures preserved unrelated.txt byte-for-byte. No run.completed or accepted final occurred. Observer bounds are harness limits, not authoritative success/failure timeouts. A terminal failed state, a successful tool call and a completed Agent task are distinct.

| Supporting capability | Status | Boundary |
|---|---|---|
| Exact once approval | EXISTING + REUSED, live exercised | B/D approvals via release TUI y/n; no broad grant or destructive command accepted. |
| Cancellation after bounded observation | EXISTING + REUSED, live exercised | B/E reached run.cancelled. This does not test cancellation during a compiler child or prove arbitrary descendant cleanup. |
| Native observation tail | IMPLEMENTED + VERIFIED | Deterministic before-failure / after-pass regression; original record retained. |
| Planning process observation tail | IMPLEMENTED + VERIFIED | Same; status/exit and failure tail survive bounded context. |
| UTF-8 byte-limited preview | IMPLEMENTED + VERIFIED | Regression proves byte overflow before fix; strict byte bound, valid UTF-8 and failure tail after fix. |
| Detailed tool inspection | EXISTING + REUSED, limited | Real timeline/approval captured; raw arguments/results are not fully projected by current IPC. No new live /details acceptance claim. |
| Physical terminal / other platforms | NOT VALIDATED | PTY at 120×40 and VT replay only. No Ghostty, Terminal, iTerm2, Linux/Windows, tmux/SSH acceptance. |

Public model-request started/completed event counts: A 4/4; B 11/10; C 6/6; D 9/9; E 8/7. These are event counts, not token accounting; cancellation leaves a request without a completed event. E did not successfully accumulate the requested file evidence, so it does not certify a near-limit context workload. C/D provide actual context-exhaustion evidence.
