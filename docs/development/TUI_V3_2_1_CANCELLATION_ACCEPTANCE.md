# V3.2.1 cancellation acceptance

## Authority and propagation

Rust's Ctrl+C action emits typed `run.cancel {run_id}`. DaemonIpcServer accepts only its actual running/approval run and routes to AgentRuntime (or Chat cancellation). AgentRuntime sets its cancellation token, publishes Cancelling and queues AgentLoop::cancelAsync; the loop invokes its current gateway/executor cancellation closure. ProcessExecutor terminates only its owned process/job. Unix enforced process groups get SIGTERM and a one-second grace before SIGKILL; Windows uses its owned job. Child containment remains platform/sandbox dependent. No arbitrary process was terminated.

A synchronous/atomic filesystem operation can finish before cancellation is observed; a client must wait for authoritative terminal state. It is not evidence of mid-read preemption. Tool success and accepted Agent completion remain separate.

## Actual live probes

All probes use private portable profiles and synthetic workspaces, `lm-studio / nvidia/nemotron-3-nano-4b` with reported loaded context8192.

- **LIVE E2E PASS, IPC:** F-process-cancel: model selected actual native run-command, one exact digest-scoped approval allowed `cmake -E sleep 20`, tool.running observed, run.cancel accepted and run.cancelled received. No file diff. Owned-PID cleanup/TUI key propagation is assessed separately, not inferred from this row.
- **LIVE E2E PASS, IPC:** G-approval-cancel: cancellation during actual pending approval produced run.cancelled without granting access. No command effects are counted.
- **LIVE E2E PASS, IPC boundary:** H-continuation-cancel: cancelled after an actual tool result, then run.cancelled. This establishes boundary cancellation, not private model streaming visibility. The metadata forwarding proxy does not propagate upstream disconnects during a blocked read, so this probe does not attest backend inference abort.
- **PARTIAL:** Direct-endpoint TUI Ctrl+C / owned-process tracking: see `cancellation-pty.json`, `owned-process-cleanup.json`, raw PTY captures and final report. The approved cmake command failed approximately 5 ms after tool.running; no descendant PID was observed. Ctrl+C cancelled subsequent planning. This is not evidence of interruption or cleanup of a live process. Only actually observed descendants count as cleanup evidence.
- Mid-read preemption, physically typed native-host Ctrl+C/Shift+Enter, Linux/Windows process containment: **NOT VALIDATED** in this macOS run. Existing process/IPC fixtures are separate integration evidence.

## Reproduced cancellation/session race

The bounded B run was cancelled; creating C immediately allowed a queued B cancellation message into C's new history. Raw C PTY/history captures reproduce this. AgentRuntime publishes its terminal event before the subsequent RuntimeStateChanged projection; the controller's previous active Agent session identity survived a conversation reset.

`cancelledProjectionCannotLeakIntoNewConversation` deterministically queues a new conversation from AgentCancelled before the late projection. It failed with the actual cancellation text before the fix. Conversation create/switch now refuse active Agent runs; idle conversation resets retire the old Agent projection identity. Authoritative run records/events are retained. The fix does not reorder AgentRuntime events or introduce another cancellation authority. The new regression passes in the final 122/122 C++ test run.

## Recovery boundaries

No mutation is automatically replayed. The benchmark's timed-out modification task made no observed edits; its timeout is not success. A real provider context error remains failure. Reconnect/approval/draft/stream identity guards retain existing deterministic coverage. Native invalid-argument repair, read-only retry, missing-file failure evidence and timeout behavior are fixtures unless a corresponding real-model row explicitly reports otherwise.

## Process acceptance limitation

`tool.running` means gateway execution has started, not that a child OS process has successfully launched. The initial IPC cancellation probe and direct TUI probe cannot establish child cleanup from that event alone. The public history projection omits the detailed process launch error. The exact cause of the cmake launch failure is therefore not established by saved public evidence.

Source inspection identifies relevant macOS constraints: StaticSandboxPolicy requires process-tree control for run-command; ProcessSandbox implements this with a stronger deny-fork policy because it lacks a kill-on-close tree primitive. Homebrew toolchain paths also are not broadly readable. These are possible compatibility constraints, not a proven diagnosis of this launch. No sandbox rule was weakened. ProcessExecutor cancellation tests use explicitly permitted unconfined synthetic fixtures and are INTEGRATION PASS, not live Agent gateway acceptance.

## Confirmed running-process cancellation

**LIVE E2E PASS, macOS PTY:** A separate direct-endpoint probe asked the real model to run exactly `/bin/sleep 20`, approved only that command digest, then attached the actual release TUI at 80×24. Before Ctrl+C, the owned daemon descendant `/bin/sleep` PID 72613 (parent 72488) was observed. Two seconds after Ctrl+C the descendant tree was empty and run `8f2f8344-d167-42ec-8242-3f213bbb2818` reached authoritative run.cancelled. No fixture diff and no subsequent tool execution were observed. This closes this single-process cancellation case, not arbitrary child-tree containment.

Evidence: cancellation-pty-sleep.json, owned-process-cleanup-sleep.json, cancel-sleep-before/after.ansi and the inspected VT replay PNG. This used the direct local endpoint, not the non-aborting metrics proxy. It does not claim physical Ghostty/macOS Terminal acceptance. The cancelled footer is correct; an older `/notices` cancellation-waiting text remains visible and the zero-completed-steps wording is ambiguous after a tool starts. Those existing presentation issues were not redesigned in this reliability phase.

## Additional bounded probes

A direct Chat streaming probe produced no public output.delta within 90 s and was cancelled by its observer. It does not establish mid-stream cancellation; native Agent token streaming is NOT VALIDATED. With only the isolated profile endpoint set to closed localhost port 1, immediate Agent cancellation returned run.cancelled and no tool event. This verifies unavailable-configuration cancellation at the IPC boundary, not that an in-flight network request had already failed. Restoring that profile endpoint yielded a daemon state snapshot, not proof of resumed successful inference. See recovery-live.json. Read-only tools completed in milliseconds; mid-read interruption is NOT VALIDATED.
