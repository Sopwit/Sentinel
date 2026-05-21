# Phase 18 Agent And Voice Runtime Checkpoint

Phase 18.31-18.33 completes the agent/tool/voice runtime checkpoint for the current Desktop Alpha
architecture.

## Completed Phase 18 Scope

- Added agent task runtime, deterministic task queue, lifecycle metadata, and planning-session
  arbitration/refusal metadata.
- Added deterministic capability-registry metadata and tool contract/permission/sandbox metadata.
- Added voice runtime permission/path readiness metadata for future Piper and Whisper activation.
- Added Whisper STT and Piper TTS client boundaries that refuse before execution.
- Added voice pipeline session orchestration over Whisper readiness, local chat inference
  readiness, and Piper readiness.
- Added controlled audio-file session validation/readiness metadata for future offline STT.
- Exposed compact QML-safe statuses, counts, summaries, readiness lists, trace lists, fallback
  summaries, and safety summaries through `ApplicationController` and `DesktopShellViewModel`.

## Agent Runtime Boundary

- `IAgentTaskRuntime` owns task orchestration metadata only.
- `StaticAgentTaskRuntime` creates deterministic local tasks and queue/lifecycle state.
- Planning sessions derive bounded candidates from the queue and record arbitration, refusal, and
  fallback summaries only.
- Agent capabilities are local-only metadata. Future filesystem, shell, and plugin capabilities
  remain disabled or refused.
- Agent runtime paths do not start workers, schedules, approval flows, providers, models, tools,
  plugins, filesystem actions, shell commands, subprocesses, cloud/API calls, or autonomous loops.

## Tool Contract Boundary

- `ToolContractRegistry` is permission and sandbox metadata only.
- Enabled contracts are local-only/read-only metadata contracts for future conversation summary,
  memory inspection, retrieval preparation, semantic supplement preparation, voice response
  preparation, and export preparation.
- Future filesystem access, subprocess execution, plugin runtime, and export action contracts
  remain disabled or refused.
- Sandbox labels summarize future requirements only. No sandbox runtime, permission grant,
  approval workflow, executor, plugin host, filesystem adapter, subprocess boundary, or export
  action exists in this phase.

## Voice Runtime Boundary

- Voice runtime readiness records describe Piper/Whisper configured, missing, refused, sandbox,
  permission, and safety metadata only.
- Whisper transcription and Piper synthesis clients are non-executing boundaries. Null clients
  refuse, and local skeleton clients validate metadata before refusing.
- Voice pipeline session orchestration composes existing readiness values only. It does not call
  Whisper, Piper, local inference workers, chat send, transcript injection, playback, or
  subprocess execution.
- Audio-file sessions validate descriptor metadata only. They do not load files, decode waveforms,
  transcribe, play audio, scan the filesystem, or ingest automatically.

## No-Execution Guarantees

The Phase 18 audit found no added microphone capture, playback, subprocess execution, filesystem
scanning, cloud/API calls, tools/plugins, autonomous loops, real STT inference, or real TTS
inference.

Allowed behavior remains limited to metadata construction, exact configured-path validation for
voice settings/readiness, and existing unrelated app-owned features from earlier phases.

## `executionAttempted` Invariant

`executionAttempted` remains false across:

- agent task results, queue results, planning results, capability summaries, and registry
  summaries
- tool contract summaries, safety reports, and registry summaries
- voice runtime readiness and safety reports
- Whisper transcription sessions, readiness, traces, safety reports, and results
- Piper synthesis sessions, readiness, traces, safety reports, and results
- voice pipeline session results, traces, and safety reports
- audio-file session readiness, traces, summaries, safety reports, and results

Focused tests already cover these paths, so no redundant QA tests were added during this
checkpoint.

## Future Controlled Execution Requirements

Any future controlled execution phase must be explicitly scoped and must add:

- a concrete executor/client boundary for the exact operation being enabled
- permission and safety policy gates that default deny
- sandbox/lifecycle ownership for files, processes, devices, and cleanup
- user-visible controls that are narrow, reversible where possible, and clearly non-autonomous
- request/session ids and stale-result protection for asynchronous work
- regression tests proving refusal, fallback, cancellation/staleness, QML non-exposure, and
  no-authority expansion outside the scoped operation

## Future STT/TTS Activation Requirements

Future STT/TTS activation must separately define:

- explicit audio-file selection or microphone permission flow
- bounded file reads or audio capture lifecycle
- Whisper/Piper subprocess permission, timeout, cancellation, and cleanup behavior
- transcript/audio ownership, privacy, and prompt/chat injection rules
- playback/audio-device permission and lifecycle if playback is in scope
- tests proving disabled defaults, unsafe path refusal, no cloud fallback, no automatic chat send,
  no transcript injection unless explicitly authorized, and no raw path/payload exposure to QML

## Future Tool Runtime Requirements

Future tool runtime activation must separately define:

- tool executor ownership behind `IToolExecutor` or a similarly explicit boundary
- approval workflow and permission persistence rules
- sandbox implementation and denied-capability behavior
- filesystem/subprocess/plugin/export scopes as separate permissions
- audit trail and user-visible result summaries
- tests proving refused unsafe scopes, no hidden plugin loading, no autonomous loops, and no raw
  tool payloads exposed through QML

## Known Limitations

- Agent tasks are not scheduled or executed.
- Planning sessions are not approval workflows or autonomous agents.
- Tool contracts are not permission grants or tool runtimes.
- Voice runtime readiness is not a Piper/Whisper launcher or model loader.
- Whisper and Piper clients do not perform real inference.
- Audio-file sessions do not read files, decode audio, or create transcripts.
- QML surfaces remain status/readiness-only and have no upload, record, speak, play, execute,
  approve, plugin, filesystem, shell, or autonomous controls.

## Phase 19 Readiness Criteria

Phase 19 may proceed only if it preserves the checkpoint guarantees above or explicitly scopes and
tests a controlled authority change. Readiness requires:

- no hidden execution paths introduced by agent, tool, or voice metadata
- `executionAttempted = false` preserved for all metadata-only paths
- QML exposure limited to safe strings, counts, booleans, and string lists
- no raw paths, transcripts, prompts, audio payloads, provider/client handles, tool payloads, or
  runtime objects exposed to QML
- any real execution proposal split by domain: tool runtime, audio-file STT, live microphone STT,
  TTS synthesis, playback, filesystem action, subprocess action, or plugin runtime
- focused tests for every newly authorized operation and every refusal/fallback path

## Readiness Verdict

Phase 18 is ready to close. The agent/tool/voice architecture is coherent, metadata-only, and
bounded. Phase 19 can build on this foundation, but real tool execution, STT/TTS inference,
microphone capture, playback, subprocesses, filesystem scanning, cloud/API calls, plugins, and
autonomous loops remain unauthorized until separately scoped.
