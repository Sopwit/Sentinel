# Phase 13 Checkpoint

Phase 13.9 completes the Voice/Piper checkpoint and readiness review for the current Desktop Alpha
voice architecture.

## Completed Phase 13 Scope

- Metadata-only local voice runtime environment ownership for future Piper and Whisper binaries,
  models, permissions, and safety reporting.
- A Piper text-to-speech provider boundary behind `ITextToSpeechProvider`.
- Default-disabled Piper configuration and deterministic refusal for missing or invalid Piper
  binary/model metadata.
- Controlled local Piper file-output synthesis behind `IPiperTtsClient` and provider gates.
- QML-safe controller and view-model exposure for voice runtime, safety, Piper readiness, and
  Piper file-output status summaries.
- Read-only Settings visibility for voice/Piper readiness with no activation controls.
- Test coverage using null/static/fake providers and clients, with no real Piper, Whisper,
  microphone, playback device, downloads, or cloud service required.

## Current Piper And Voice Architecture

- `ITextToSpeechProvider` owns the TTS provider boundary.
- `PiperTextToSpeechProvider` is the current Piper adapter behind that boundary.
- `IPiperTtsClient` owns the low-level Piper client boundary.
- `NullPiperTtsClient` is deterministic and never writes files, plays audio, downloads assets, or
  launches Piper.
- Historical note: a controlled local file-output client existed in Phase 13. Phase 18.22-18.24
  supersedes current behavior with a non-executing Piper synthesis boundary.
- `ISpeechToTextProvider` remains backed by the null STT provider; no Whisper adapter exists yet.
- `IVoiceRuntimeEnvironment` reports binary/model/permission/safety metadata for Piper and
  Whisper readiness without probing broadly or enabling voice runtime authority.
- `ApplicationController` owns the injected/default providers and exposes only safe status,
  summary, and readiness values.
- `DesktopShellViewModel` forwards strings, string lists, and booleans only. QML does not receive
  provider, client, process, path-owner, model, session, or safety objects.

## Current TTS Path

The current TTS path is:

```text
text/config metadata -> Piper synthesis readiness -> deterministic refusal/fallback metadata
```

Current desktop builds do not reach a Piper process client. Future controlled synthesis and future
playback/audio-device support require separate explicit phases.

The default app configuration does not pass these gates. By default, Piper remains disabled/not
configured and the provider refuses before the client boundary.

## Safety Boundary Findings

- Piper is disabled by default.
- Piper file output is available only behind explicit configuration, request, permission, safety,
  binary, model, and controlled-output-path gates.
- No audio playback exists.
- No microphone access exists.
- No Whisper execution exists.
- No model or binary downloads exist.
- No cloud voice provider, cloud call, credential store, or API-key path exists.
- No autonomous voice loop, wake-word flow, background recording, or speak/play UI exists.
- Settings remains read-only for voice/Piper readiness and does not expose setup, path picker,
  model picker, speak, play, record, or activation controls.

## Remaining Limitations

- Piper binary/model configuration is not user-configurable through the UI.
- Piper file output has no user-facing synthesis action in QML.
- Generated audio is not played back.
- There is no cleanup UX, history, or output-file management surface for synthesized files.
- Whisper/STT remains a null-provider refusal path.
- Voice runtime metadata is not connected to a conversation loop, local inference streaming, agent
  runtime, tools, plugins, or memory recall.

## Future Next Steps

- Add Piper path/model configuration UX after defining user-controlled path ownership, validation,
  persistence, and failure summaries.
- Add a controlled playback phase after defining playback permission, device ownership,
  cancellation, cleanup, and no-autoplay rules.
- Add a Whisper STT adapter phase after defining microphone permission, capture lifecycle, local
  binary/model ownership, transcription privacy, cancellation, timeout, and error behavior.
- Add a voice conversation loop phase only after TTS, STT, playback, capture, inference routing,
  user activation, cancellation, and safety policy are explicitly integrated.

## Phase 14 Readiness Criteria

- Keep Piper disabled by default unless a later phase explicitly enables configuration UX.
- Keep file-output execution behind the existing provider/client gates.
- Keep playback, microphone access, Whisper execution, downloads, cloud/API-key behavior, and
  autonomous voice loops out of scope until separately approved.
- Preserve injectable providers/clients so tests do not require local voice binaries, models, or
  audio devices.
- Keep QML exposure read-only and QML-safe until a later phase explicitly adds voice controls.

## Phase 14 Readiness Verdict

Phase 14 is ready to begin as a planning or configuration-readiness phase. It is not ready for
autonomous voice operation, microphone capture, playback, Whisper execution, downloads, cloud
voice providers, or broad runtime automation without a new explicit phase scope and matching
safety gates.
