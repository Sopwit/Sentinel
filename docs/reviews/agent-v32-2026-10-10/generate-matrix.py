from pathlib import Path
import json,collections
out=Path('docs/reviews/agent-v32-2026-10-10')
s='''# TUI V3.2 Agent test matrix

All live rows: `lm-studio / nvidia/nemotron-3-nano-4b`, localhost LM Studio, isolated portable daemon/profile and synthetic C++/CMake workspace. Real inference is separate from deterministic provider fixtures. Counts are observed public events; model turns are `agent.activity / ModelRequestStarted`, excluding any unreported classifier/repair requests. Tokens: unavailable. Time is harness wall time, not an equivalent competitor benchmark.

## Live tasks

| Build / task | Terminal outcome | Tools requested / results | Model turns | Seconds | Approval / change evidence |
|---|---|---:|---:|---:|---|
'''
for filename,prefix in [('natural-tasks.json','Baseline'),('post-fix-tasks.json','Updated'),('controlled-recovery.json','Updated controlled')]:
 p=out/filename
 if not p.exists():continue
 for c in json.loads(p.read_text())['cases']:
  run=c['terminal'].get('payload',{}).get('run_id');ev=c['events'];req=sum(e['name']=='tool.requested' for e in ev);res=sum(e['name']=='tool.result' for e in ev);turn=sum(e['name']=='agent.activity' and e['payload'].get('activity')=='ModelRequestStarted' for e in ev)
  changes='real target diff; unrelated preserved' if c['diff'] else 'no diff; unrelated preserved'
  s+=f"| {prefix} {c['label']} | {c['terminal']['name']} | {req} / {res} | {turn} | {c['duration_seconds']:.2f} | {len(c['decisions'])} decision(s); {changes} |\n"
s+='''
**EXISTING + REUSED / live PASS:** constrained Level 1 `@CMakeLists.txt` + `@README.md` → actual authorized read-file → real model continuation → accepted `run.completed`; fact/excerpt `set(CMAKE_CXX_STANDARD 20)`. Evidence: `context-ui.json`, raw context captures. Run `b7198407-8af4-43a7-a20e-bc0118ad2600`, session `7b214903-cc29-4460-83c8-dc2059e0e617`. This was an explicit read instruction, not a general natural-task benchmark. Function explanation/file location were not separately accepted live.

**Level 2 NOT ACCEPTED:** baseline doom-loop failure; updated six reads followed by evidence rejection for math.h. No fluent explanation was accepted. The public IPC lacks redacted arguments/results sufficient to diagnose the exact observation/resource mismatch.

**Level 3 IMPLEMENTED + PARTIALLY VERIFIED:** updated task got one exact-target edit-file approval and produced real `a-b` → `a+b` diff in math.h. Multiply, main.cpp, test_main.cpp and unrelated.txt remained intact. Provider continuation returned context-size exceeded; accepted final and subsequent verification are missing. Baseline made no changes. No mutation retry occurred.

**Level 4 NOT ACCEPTED:** baseline exceeded 300-second bound; cancellation requested. No target change/build/test evidence. `workspace.changes` error during cancellation is not interpreted as an empty authoritative review. Natural Level 4 was not rerun after the failed Level 3 continuation.

**Level 5 NOT ACCEPTED:** baseline missing-file/fallback task exceeded 300-second bound and was cancelled, not successfully recovered. An event arriving from a previously cancelled run can be present in the next case's collected events; terminal attribution must use run_id, not name alone.

## Recovery and UI acceptance

| Scenario | Status | Evidence / limitation |
|---|---|---|
| Isolated daemon restart / session continuity | EXISTING + REUSED, VERIFIED | recovery.json: same session, persisted grounded history and accepted ModelBinding |
| Provider interruption | EXISTING + REUSED, VERIFIED | isolated endpoint deliberately closed; authoritative run.failed / ConnectionFailed, no false success |
| Provider restoration | PARTIALLY VERIFIED | endpoint restored, discovery/model.select accepted; no successful new inference asserted |
| Active client interruption / reconnect | IMPLEMENTED + VERIFIED | real release TUI attachment at 80×24/120×40 shows task; newer output/draft/approval preservation also deterministic |
| Live running-tool cancellation | BLOCKED | controlled probe returned provider context-size error before tool execution; not counted as cancellation acceptance |
| Live permission-denial task | See controlled-recovery.json | only actual approval/terminal events count; a pre-tool provider failure does not exercise denial |
| Invalid arguments / unknown native calls | IMPLEMENTED + VERIFIED, deterministic only | rejected calls absent from native continuation, at most two planning attempts, no tool effects |
| Repeated reads | IMPLEMENTED + VERIFIED, deterministic only | sync/async, one planning repair, no repeated execution; subsequent repetition stops |
| Repeated mutation | VERIFIED, deterministic only | one write execution; no read-repair bypass or automatic replay |
| Tool timeout / process cancellation | EXISTING + REUSED, real process fixture | ProcessExecutor tests use disposable processes; separate from live-model Agent acceptance |
| Denial / pending approval / Agent cancellation | EXISTING + REUSED, deterministic | AgentLoop, AgentRuntime, daemon IPC and TUI regressions; no personal grants |
| Responsive / keys / focus | EXISTING + REUSED, deterministic + PTY | Rust size matrix 80×24,100×30,120×40,160×48; actual reconnect capture at80×24/120×40 |
| Physical terminal Shift+Enter / native hosts | NOT VALIDATED | PTY key routing is not physical Ghostty/Terminal/iTerm2/Linux/tmux/SSH acceptance |

## Regression/build commands

- `cargo fmt --all -- --check` in cli: PASS.
- `cargo clippy --workspace --all-targets -- -D warnings` in cli: PASS.
- `cargo test --workspace` in cli: **73 passed**, 0 failed (14 CLI, 11 IPC, 48 TUI).
- `cargo build --workspace --release` in cli: PASS.
- `cmake --preset tests`: PASS.
- `CCACHE_DISABLE=1 cmake --build --preset tests -j4`: PASS. Ccache disabled because configured external cache is outside writable roots; no project cache setting changed.
- Changed C++ ranges formatted with repository clang-format configuration; `git diff --check`: PASS.
- `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`: final result recorded in `ctest-verified.log`. The restricted sandbox attempt failed socket/process/test-storage access; final execution used approved access without weakening Sentinel sandbox policies.
- Focused LlmAgentRuntime: **29 passed**; AgentLoop **28 passed** (includes Qt initialization/cleanup and data rows). Complete suite includes IPC, observation policy, process executor, security and UI contracts.

No incorrect accepted success was observed in the recorded runs. Failed/incomplete requests and a partial source mutation are explicit. This finite sample does not prove all models never hallucinate. No equivalent Codex/OpenCode executable benchmark was performed. Final verdict: **IMPROVED — FURTHER FIXES REQUIRED**.
'''
Path('docs/development/TUI_V3_2_AGENT_TEST_MATRIX.md').write_text(s)
