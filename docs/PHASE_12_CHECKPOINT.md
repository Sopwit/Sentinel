# Phase 12 Checkpoint

Phase 12.7-12.9 completes the voice checkpoint and local voice integration planning pass for the
current Desktop Alpha voice architecture.

## Completed Phase 12 Scope

- Disabled-by-default voice provider metadata for future text-to-speech and speech-to-text work.
- `ITextToSpeechProvider` and `ISpeechToTextProvider` boundaries.
- Deterministic `NullTextToSpeechProvider` and `NullSpeechToTextProvider` implementations that
  refuse safely.
- `VoiceSession`, `VoicePipeline`, and `IVoiceRuntimeCoordinator` metadata for future runtime
  ownership.
- `StaticVoiceRuntimeCoordinator` with deterministic completed, blocked, and error pipeline
  traces.
- `ApplicationController` ownership of TTS/STT providers and the voice runtime coordinator.
- `DesktopShellViewModel` exposure through QML-safe strings, string lists, and booleans.
- Read-only Settings voice readiness, runtime, session, and pipeline metadata.
- Test coverage for null provider refusal, deterministic pipeline metadata, blocked/error paths,
  controller exposure, view-model exposure, and disabled runtime posture.

## Current Voice Architecture

- `ITextToSpeechProvider` owns the future text-to-speech provider boundary. Piper may only enter
  through this interface in a later explicit phase.
- `ISpeechToTextProvider` owns the future speech-to-text provider boundary. Whisper may only enter
  through this interface in a later explicit phase.
- `NullTextToSpeechProvider` and `NullSpeechToTextProvider` are the only current providers and do
  not synthesize, play, record, read, or transcribe audio.
- `VoiceSession` records deterministic session identity, state, runtime summary, and safe summary
  text.
- `VoicePipeline` metadata records idle, preparing, awaiting-input, transcribing-placeholder,
  inference-placeholder, synthesis-placeholder, completed, blocked, and error stages.
- `StaticVoiceRuntimeCoordinator` owns the current metadata-only runtime summary and pipeline
  evaluation.
- `ApplicationController` constructs null/default voice providers and the static coordinator when
  no injected test doubles are supplied.
- `DesktopShellViewModel` forwards only QML-safe voice metadata; QML does not receive providers,
  coordinator objects, requests, responses, sessions, or traces as raw core objects.

## Known Limitations

- No microphone access exists.
- No audio playback exists.
- No Piper integration exists.
- No Whisper integration exists.
- No subprocess, process, or local binary ownership exists for voice.
- No voice model path, model catalog, install, download, or deletion workflow exists.
- No capture/playback lifecycle, cancellation, device selection, or permissions UX exists.
- No voice activation, voice buttons, autonomous voice loop, or wake-word behavior exists.
- Voice metadata is not connected to chat routing, local inference, streaming, tools, plugins, or
  memory recall.

## Safety Guardrails

- Voice runtime mode remains disabled.
- Null TTS/STT providers return safe refusals and report unavailable/disabled metadata.
- Runtime summaries explicitly report runtime unavailable, TTS unavailable, STT unavailable,
  microphone disabled, playback disabled, local-only policy active, and process execution
  disabled.
- The static pipeline emits metadata traces only and does not touch audio devices, filesystems,
  system services, networks, subprocesses, providers, models, tools, plugins, or API keys.
- Settings shows read-only metadata only and provides no record, speak, setup, model-management,
  activation, or playback controls.
- QML receives no raw provider, session, pipeline, runtime coordinator, path, credential, device,
  or process objects.

## Future Piper Integration Plan

- Add a concrete TTS provider only behind `ITextToSpeechProvider`.
- Define local binary ownership before execution: discovery/configuration, version metadata,
  allowed path handling, and no implicit downloads.
- Define local voice model ownership before execution: configured model path metadata,
  compatibility checks, and explicit user-controlled installation or selection in a later phase.
- Define playback ownership separately from synthesis: output file/buffer lifecycle, audio device
  selection, cancellation, and cleanup.
- Gate Piper execution behind runtime permission and safety checks, with deterministic error and
  refusal summaries.
- Keep provider construction injectable so tests never require Piper, audio devices, or model
  files.

## Future Whisper Integration Plan

- Add a concrete STT provider only behind `ISpeechToTextProvider`.
- Define microphone permission and capture lifecycle before recording: permission state,
  start/stop ownership, cancellation, and no background capture.
- Define local Whisper binary/model ownership before execution: configured binary/model metadata,
  compatibility checks, and no implicit downloads.
- Keep transcription local-first and explicit, with privacy-safe summaries and no cloud fallback
  unless a later provider phase authorizes it.
- Gate transcription behind runtime permission and safety checks, with deterministic blocked,
  refused, timeout, cancellation, and error paths.
- Keep provider construction injectable so tests never require Whisper, microphones, or model
  files.

## Phase 13 Readiness Criteria

- Preserve the current disabled/no-audio behavior until Phase 13 explicitly scopes a voice
  capability.
- Keep Piper and Whisper behind their provider interfaces and the voice runtime coordinator.
- Define permission, safety, lifecycle, cancellation, and error semantics before any audio I/O or
  subprocess execution is added.
- Keep QML read-only and QML-safe unless a later phase explicitly adds voice controls.
- Keep local voice integration local-first, injectable, deterministic in tests, and separate from
  chat providers, local inference clients, tools, plugins, memory, and model management.
- Re-run the full test preset, clang-format dry-run, and `git diff --check` before starting any
  executable voice integration.
