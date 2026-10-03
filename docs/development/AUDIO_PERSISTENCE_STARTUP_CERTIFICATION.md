# SENTINEL — AUDIO / PERSISTENCE / STARTUP CERTIFICATION REPORT

Current host: macOS, 2026-10-03. **FINAL VERDICT: PARTIAL.** Persistence and startup exercised below passed, with six reproduced defects fixed. Positive real audio, live voice chains, and several explicitly limited cases remain unvalidated. Installed audio executables/models alone do not establish runtime success.

The shipping Qt desktop and production adapters were used. Disposable profiles were isolated with `CFFIXED_USER_HOME`, `HOME`, and `TMPDIR` under `/tmp/sentinel-startup-cert`. Paths were verified through Qt before persistence tests. No models were downloaded, OS permissions changed, or sandbox policy weakened. Controlled forced termination targeted disposable-profile processes only. Personal-Brain CLI was unavailable; the explicitly supplied workspace and repository instructions resolved the project. Earlier unrelated working-tree changes were preserved.

Evidence: `build/certification/audio-persistence-startup/` contains cycle/degraded JSON, final test logs and sampled stack. These are local build artifacts. Test helpers and deterministic regressions are in `tests/`. Runtime checks preceded the final full regression run; the original ten-cycle run preceded the final interrupted-message and asynchronous-startup fixes.

## 1. Audio Architecture

- **Microphone:** `AudioDeviceService`, Qt `QMediaDevices`/`QAudioSource`, `QMicrophonePermission`; device selection, bounded capture, mono 16 kHz signed 16-bit PCM normalization and VAD are implemented.
- **STT:** `WhisperSttRuntime` invokes whisper.cpp through `ProcessExecutor`. A configurable real adapter exists; it is not merely an interface.
- **TTS:** `PiperTtsRuntime` and `KokoroTtsRuntime` are implemented. Kokoro is exposed by engine selection but requires a compatible `kokoro-tts` CLI, ONNX model and voices asset. Legacy voice clients explicitly reject retired paths.
- **Playback:** `AudioPlaybackService` uses `QMediaPlayer`/`QAudioOutput`.
- **Voice Chat / Voice Agent:** `VoiceSessionService` connects final transcription to the respective bridge. Agent requests enter `AgentRuntime`; accepting an `AgentLoop` final answer remains the completion boundary. Implementation exists, but live chain success is not established here.
- **Lifecycle/temporary data:** session cancellation tokens and generations suppress stale callbacks; capture uses private temporary files and synthesized artifacts belong to the session. Production process execution retains strict sandbox/no-network/no-detached-child rules.

## 2. Real Audio

| Path | Current-host result |
| --- | --- |
| Microphone | Qt permission **Denied**; two input devices and one output device enumerated. Start returned false, capture remained false, typed `MicrophonePermissionDenied`. Authorized capture NOT VALIDATED. |
| Whisper | Existing Homebrew whisper.cpp 1.9.4, `ggml-base.bin`, prerecorded packaged `jfk.wav`. Production adapter returned `TranscriptionFailure` in approximately 146 ms. Sandbox diagnostic: `libwhisper.1.dylib` blocked. No usable transcript. The requested recorded Turkish phrase could not be captured. |
| Piper | Existing pipx Piper executable and `tr_TR-fahrettin-medium.onnx`; requested text `Sentinel ses testi başarılı.`. Production adapter returned `SynthesisFailure` in approximately 57 ms, no output file. Identical sandbox diagnostic returned exit 71 / exec operation not permitted. Python interpreter/venv dependency access is unresolved. |
| Kokoro | Existing ONNX/voices assets, compatible CLI absent. Production adapter returned typed `RuntimeUnavailable`, approximately 4 ms. |
| Voice Chat / Voice Agent | RUNTIME NOT VALIDATED: microphone denial and STT execution block the input chain. Prior real Agent certification is not reclassified as voice evidence. |
| Playback | Positive real synthesis/playback NOT VALIDATED. |

**No real positive audio path passed.** Audio lifecycle AUTOMATED CONTRACT PASS is separate from these runtime failures. The speech sandbox/dependency contract needs further resolution; this report does not claim those failures were fixed.

## 3. Audio Failure Paths

Native permission denial terminates capture safely. Deterministic owner tests cover invalid device selection, idle/idempotent stop and cancel, missing runtimes/models, unauthorized and empty input, stale STT success, empty successful STT result, outstanding TTS cancellation, and late artifact cleanup after failure/destruction. No physical device was disconnected and no permission was revoked by the test.

Recording and playback cancellation have idle/contract coverage; cancellation during actual authorized recording/playback is NOT VALIDATED. Unsupported nonempty audio is not independently certified as a native runtime case. STT/TTS late-result cancellation is deterministic and verified without success-by-sleep. Real audio child termination under active successful processing remains unvalidated because processing could not start successfully.

## 4. Persistence Ownership

| Data | Owner / current-host location within isolated home |
| --- | --- |
| Conversations, messages, summaries | `ConversationStore`, `Documents/Sentinel/conversations.sqlite3` |
| Legacy transcript | Separate `ChatHistoryStore`, `Documents/Sentinel/chat_history.sqlite3` |
| Memory | `SQLiteMemoryStore`, `Documents/Sentinel/memory.sqlite3` |
| Agent runs, events, steps, evidence | `SQLiteAgentRunStore`, Application Support `agent_runs.sqlite3` |
| Permission grants | `SQLitePermissionGrantStore`, Application Support `permission_grants.sqlite3` |
| Local RAG | Separate Application Support `local_rag.sqlite3` |
| Settings, workspace catalog/profiles, presets, selections, onboarding | AppConfig `settings.json`; credentials remain in their credential store |
| Skill enable preferences | AppConfig `skill_preferences.json`, source/name identity |
| Plugins/extensions | PluginManager runtime state; workspace extension overrides belong to settings/profiles. General PluginManager enable-state restart persistence was not established. |
| Recovery diagnostics | Application Support `recovery-status.json`, separate from owner data |

No cross-store migration, merging or new recovery architecture was introduced. Settings, memory, Chat, Agent and grants retain separate owners.

## 5. Normal Restart

- **Chat: PASS.** Actual local llama.cpp Chat response `PERSISTENCE_OK` persisted as Completed and restored through normal desktop restart. Completed rows remained intact without duplicate restoration rows. A later recovered-provider request returned actual `RECOVERY_OK`.
- **Cancelled Chat:** real Stop attempts lost the race to already completed generation. Native cancelled-message creation is NOT VALIDATED; cancelled state/content preservation is deterministic owner coverage.
- **Agent:** deterministic production-store round trips preserve Completed/Cancelled and turn interrupted active metadata into Interrupted. These fixtures are synthetic metadata, not a new real Agent execution. Real Agent metadata generation in this phase is NOT VALIDATED.
- **Workspace/settings: PASS** for research selection, representative theme/provider/model and persisted local endpoints. The endpoint defect below was fixed and regression tested.
- **Skills/plugins:** existing owner tests verify skill preference persistence and extension isolation. Actual desktop restart with a newly changed skill/plugin enable preference was NOT VALIDATED.
- Empty interrupted assistant content now displays a truthful interruption notice while raw stored content remains unchanged; partial cancelled content is retained.

## 6. Repeated Startup

**10/10 normal launch → rendered/restored main window → normal Quit cycles passed**, each exit 0. Every cycle had SQLite integrity checks with no errors and no remaining direct children. No fatal-warning accumulation or locked DB was observed. Eight short cycle lifetimes were approximately 1.6–1.7 seconds; these include UI inspection and Quit and are **not startup latency measurements**. Two cycles included longer inspection pauses.

Relaunch gaps were approximately 40–80 ms after checks, exercising singleton/DB release. Restored Chat was observed in all ten UI snapshots. No provider callback replay or plugin-host accumulation was observed in this exercised scope. Optional daemon behavior and active-plugin-host rapid restart were not separately certified.

## 7. Unclean Shutdown

**PASS in disposable profile.** A real desktop request reached a persisted assistant Sending row. Only that owned desktop was forcibly terminated. Previously completed messages remained intact; restart changed the incomplete assistant row to Interrupted, never Completed, with no automatic replay. The initially blank interrupted card was reproduced and fixed in its view-model/QML presentation.

The actual WAL was approximately 32,992 bytes and SHM was present after termination. Reopen/integrity succeeded. A deterministic child-process regression also holds production stores open before controlled kill and verifies Chat, memory and Agent metadata recovery.

## 8. SQLite

- WAL/checkpoint/reopen: PASS on disposable production stores with retained records and `integrity_check`.
- WAL + SHM present: PASS, including real unclean-desktop recovery. Presence itself was not treated as corruption.
- Arbitrary stale/mismatched auxiliary files and controlled WAL/SHM corruption: **NOT VALIDATED**. No unsupported destructive repair was attempted.
- Malformed primary conversation and memory DB: desktop starts deterministically; original malformed bytes are preserved and unrelated stores remain intact. These runs are startup-isolation PASS, **not successful database recovery**. Central recovery-status contained no condition for these malformed DBs; visible recovery diagnostic completeness remains unvalidated.
- Malformed settings: desktop fallback starts; original bytes preserved; recovery diagnostic reports `CorruptState`, `RecoveryRequired`, action `repair-settings`.

## 9. Persistence Failures

Read-only settings file/directory startup: PASS. A live settings mutation was not attempted in that profile, so it does not establish native write-failure UX. Existing deterministic settings recovery tests verify failed writes report failure, preserve disk and avoid false persisted success.

Production owner concurrency regression performs Chat persistence, a separate memory worker and serialized Agent metadata writes. PASS: no deadlock or leaked busy failure. This is a bounded owner-concurrency check, not a live provider-under-load benchmark.

## 10. Degraded Startup

**12 disposable desktop profiles exited normally with startup/UI observed:** llama.cpp down, cloud offline, Ollama down, LM Studio down, missing speech runtime, configured unavailable MCP, broken plugin fixture, malformed settings, malformed conversation DB, malformed memory DB, missing stores/settings/cache, read-only settings.

Local selected-provider failure was additionally exercised by Send: assistant Failed with `ProviderUnavailable` / discovery `ConnectionFailed`, no silent fallback. Starting the existing local server later refreshed discovery and produced real `RECOVERY_OK`; selection stayed unchanged.

Cloud Offline-policy startup is PASS; this does not certify a successful cloud request or a physically disconnected network. Missing optional runtime does not prevent the general desktop starting.

MCP unavailable and broken-plugin **fixture-presence startup isolation** passed. Their activated failure UI, successful unaffected Chat in those exact profiles, and an actually started broken plugin host were not independently observed. Existing MCP/plugin owner suites remain passing; fixture presence alone is not plugin execution certification.

## 11. Startup Responsiveness

Slow loopback Ollama discovery reproduced a synchronous startup readiness getter entering network `QEventLoop` on the Qt main thread. Before fix, inspected main window was available at approximately 4.28 seconds. After fix, stack sampling showed network wait in the poll worker; main thread sampled QML/font/image/plugin loading rather than synchronous readiness discovery. First inspected rendered window was approximately 3.72 seconds in a cold/instrumented run. This is not a formal benchmark or proof of a warm-start latency improvement.

A deterministic zero-delay event-loop sentinel proves the readiness getter no longer enters the nested transport loop. Local provider load was approximately 1.05 seconds warm and 9.26 seconds on initial load, measured separately from desktop startup. Exact discovery-complete/product-usable timestamps were not collected for every profile.

Final supplementary UI automation timed out after the bounded slow-run process ended. That extra inspection is not counted as normal-cycle evidence. Narrow responsive layout and comprehensive keyboard-focus verification remain **NOT VALIDATED**; interruption notice was visually observed before that timeout. Certification-owned processes were cleaned up.

## 12. Defects Fixed

| Reproduced defect | Owning-layer correction / regression |
| --- | --- |
| Voice failure left pending synthesis uncancelled | Failure cancels tokens/bridges/resources before publishing failure; gated late-synthesis test |
| Session destruction left late synthesized file orphaned | Destructor cancellation and worker-shared artifact cleanup guard; destruction/late artifact regression |
| Empty STT success became Completed | Reject whitespace-only transcript with typed TranscriptionFailure; deterministic regression |
| Persisted local endpoint showed in settings but controller used default | Desktop shell synchronizes persisted endpoints before provider/model selection; disk-reopen regression |
| Interrupted empty assistant rendered blank phantom card | QML-safe state notice role, EN/TR text and QML presentation; raw/partial content regression and visual retest |
| Slow Ollama readiness blocked startup main thread | Concrete HTTP runtime readiness initialized as pending; poll queued asynchronously; loopback/event-loop regression and stack retest |

Capture-stop failure handling also preserves the failure instead of overwriting it with Transcribing. No new engine, provider, daemon, IPC, UI redesign or recovery architecture was added.

## 13. Regression

Validation commands:

```sh
cmake --preset tests
CCACHE_DISABLE=1 cmake --build --preset tests -j4
QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure
CCACHE_DISABLE=1 cmake --build --preset tests --target sentinel-desktop_qmllint
git diff --check
```

`CCACHE_DISABLE` avoids an inaccessible external cache path; it is a test environment setting. Registered process/security tests ran with normal host permissions when required. QML lint exits 0 with existing warnings; it is not warning-free.

Final full run: **108/108 registered suites PASS**, 0 failed, 88.55 seconds. ApplicationController **127/0/0**; DesktopShell **74/0/0**; UnifiedAudioLifecycle **11/0/0**; PersistenceStartup **7/0/0**; ChatMessageRecovery **4/0/0**. Configure/build, QML lint and `git diff --check`: PASS. Audio/Persistence/Startup/Chat/Agent/MCP/Plugins/Skills owner suites: PASS within their automated contracts.

Six existing environment-dependent case skips, individually justified (no new certification-test skips):

1. `AgentRuntimeTest::asyncRunCommandStreamsAndContinues`: shell fixture requires fork; strict macOS process plan denies child creation.
2. `AgentRuntimeTest::asyncRunCommandCancellationAndTimeout`: same fork-dependent shell fixture; native host cancellation has separate integration coverage.
3. `AgentRuntimeTest::asyncDockerUsesProcessExecutorAndPreservesRestrictions`: dummy Docker shell fixture requires child creation denied by the strict macOS plan.
4. `AgentRuntimeTest::shutdownStopsActiveCommand`: fork-dependent shell fixture; native host shutdown has separate integration coverage.
5. `RealToolExecutorToolsTest::runCommandDockerSandboxReportsMissingDocker`: Docker is installed, so the host's missing-Docker branch cannot run.
6. `RealToolExecutorToolsTest::browserToolsReportMissingNodeGracefully`: npx is installed, so the host's missing-Node branch cannot run.

These skips do not establish runtime success for their skipped branches. New registered regressions cover unified audio lifecycle, persistence/startup and recovered Chat presentation. Existing Voice/local inference/conversation/memory/Agent/settings/workspace/recovery/retention/Chat/Tool/MCP/Plugin/Skill/security suites are included in that full run. Native runtime successes are limited to the evidence above.

## 14. Remaining Blocks

1. Native microphone is denied: authorized capture/device-loss and live Voice Chat/Agent cannot pass.
2. Existing Whisper/Piper dependencies are blocked by their strict process sandbox; no positive real STT/TTS result. Kokoro compatible CLI is missing.
3. Active real playback cancellation, native cancelled Chat generation and new real Agent metadata were not established.
4. WAL/SHM corruption and arbitrary stale auxiliaries remain NOT VALIDATED.
5. Activated unavailable MCP/broken-plugin startup diagnostics, desktop skill/plugin preference mutation/restart, native read-only write UX and comprehensive responsive/keyboard checks remain unvalidated.
6. Primary corrupt DB startup preserves data but lacks verified central/visible recovery diagnostics.

The minimum positive-real-audio gate is unmet. **FINAL VERDICT: PARTIAL**, with fixed deterministic defects and passing exercised persistence/startup scope; no claim of full audio certification.
