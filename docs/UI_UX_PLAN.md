# UI/UX Plan

Phase 5 establishes a small design-system and interaction foundation for Sentinel Desktop. It does
not implement advanced motion, assistant visuals, model management, provider integration, or
execution features.

Phase 5.4 translated the useful UI direction from the former `lovable-tasarim` design reference
into native Qt/QML. The React/Vite/Tailwind reference has been removed from the production
repository after the native shell adopted the useful visual identity pieces.

Phase 5.4.5 is a stabilization checkpoint. It should reduce obvious risk in the existing QML and
docs without adding new product features, UI systems, runtime behavior, or web dependencies.

Phase 5.5 reconstructed the visual shell around the translated Sentinel identity:
cinematic AI operating environment, presence-first workspace, floating translucent surfaces,
negative space, partial bracket grammar, cyan/teal glow hierarchy, and a bottom command dock.

## Direction

- Native Qt/QML desktop experience optimized first for Linux/Fedora KDE Plasma.
- Cross-platform-safe visual language that remains viable on Windows and macOS.
- Cinematic operating environment rather than a developer dashboard or KDE-style utility shell.
- Low-density but readable information surfaces for chat, local memory, runtime metadata, and
  settings.
- Local-first posture: UI should make unavailable networking/provider capability clear without
  implying hidden cloud behavior.
- Workspace-oriented layout: central Sentinel presence scene, floating right interaction surface,
  bottom command dock, bottom-right settings command, and compact top system status.
- Cinematic AI operating environment: presence-first, negative-space driven, atmospheric,
  translucent, and emotionally calmer than a developer dashboard.

## Design System Foundation

The initial QML design tokens live in `ui/qml/theme/SentinelTheme.qml`.

Token categories:

- palette: background, panel, surface, text, accent, success, and error colors
- spacing: small fixed layout spacing values
- radius: restrained panel and control rounding
- typography: stable pixel sizes for current desktop shell text
- motion: standard durations and easing values for low-cost UI transitions
- layout: compact and wide breakpoints, dock sizing, and small spacing helpers
- mode visuals: accent, panel color, glow scale, and status text helpers for Companion, Focus,
  Mission, System, Minimal, and Tactical modes

Guidelines:

- Prefer shared tokens for repeated colors, spacing, radii, and text sizes.
- Prefer small shared components for repeated controls such as command buttons, text fields, and
  read-only status rows.
- Keep component styling close to existing QML patterns.
- Preserve the translated Sentinel composition before adding generic utility-app dashboard
  conventions.
- Do not move business logic into QML.
- Avoid broad visual rewrites during foundation work.

## Component Consistency Guidelines

- `SentinelButton` owns command button height, hover/focus colors, and disabled opacity.
- `SentinelTextField` owns input text color, focus border, and control height.
- `InfoRow` owns read-only label/value status row typography and compact stacking.
- `MetricCard` and nested panels should use shared card padding.
- Section headings should wrap cleanly and keep page subtitles muted.
- `ShellPanel` owns glass-like panel borders and lightweight corner brackets.
- `SentinelDock` owns bottom navigation ordered Runtime/Memory, Home, Agents through the existing
  view model. Settings remains a bottom-right command button.
- `SentinelOrb` owns the lightweight central presence geometry.
- `SentinelTelemetry` owns small floating readouts for safe view-model metadata.
- `WorkspacePresence` composes the central ambient presence scene and reads only safe view-model
  state.
- `Atmosphere` owns the low-cost ambient shell background.
- Mode-aware helpers in `SentinelTheme.qml` are presentation helpers only; they should not become
  routing, provider, execution, or workflow logic.

## Adaptive Layout Guidelines

The shell should support three practical desktop width categories:

- Compact: narrow windows should reduce margins, use compact dock sizing, and stack dense
  forms or dashboard panels.
- Normal: default desktop windows should keep dock/settings access visible and preserve current page
  structure.
- Wide: wider windows can place dashboard overview and chat side by side.

Guidelines:

- Prefer wrapping grids and smaller margins over hidden controls.
- Keep dock and settings access available at compact widths, but allow labels to elide before
  layout breaks.
- Keep status surfaces readable without exposing paths, platform details, or raw internals.
- Avoid expensive resize behavior, blur-heavy layers, and device-specific calculations.
- Linux/Fedora KDE and macOS should remain usable with ordinary resizable windows and native Qt
  controls.

## Motion Guidelines

Motion should be quiet and functional:

- Use `durationFast` for hover/focus feedback.
- Use `durationNormal` for page/content state changes.
- Reserve `durationSlow` for later, rare emphasis transitions.
- Favor color, border-color, and opacity transitions.
- Avoid translating large layouts during routine navigation.
- Avoid heavy continuous animation on idle dashboards.
- Use only lightweight QML opacity, scale, and rotation animations for Phase 5.4 presence cues.
- Do not implement advanced particle systems or an assistant face yet.
- Preserve acceptable behavior on Fedora KDE Plasma, macOS, and lower-power devices.
- Avoid blur-heavy layers, shader-heavy rendering, or custom OpenGL/Vulkan render paths.

## Phase 5.4.5 Risk Notes

- Keep `SentinelTheme.qml` focused on durable visual tokens and small presentation helpers.
- Avoid adding additional always-running animations until visual QA confirms current workspace
  motion remains cheap and readable.
- Treat compact/normal/wide layout checks as required because automated visual driving is not yet
  implemented.
- Run `qmllint` when available; when unavailable, rely on QML cache compilation and startup smoke
  checks as fallback verification.

## Phase 5.5 Visual Identity Notes

- The translated native QML shell is now the visual source of truth for composition and
  atmosphere.
- The Dashboard/Core page should prioritize the central presence scene over metric-card density.
- Navigation should feel dock-led and cinematic, with settings kept as a separate bottom-right
  command and local status surfaced in the header/dashboard.
- Right-side interaction should read as a floating AI bridge surface, not an enterprise chat card.
- Continuous motion remains limited to lightweight opacity, scale, and rotation animations.

## Interaction Guidelines

- Hover states should clarify click targets without visual noise.
- Focus states should be visible for keyboard users.
- Selected navigation state should remain stronger than hover state.
- Page transitions should be subtle and should not delay interaction.
- Buttons should not imply execution capability when the underlying feature is metadata-only.

## Future Assistant Visuals

Future assistant visuals may include subtle status presence, listening/thinking indicators, and
model/runtime readiness states. These visuals should consume metadata from view models and must not
start provider calls, model execution, tool execution, or system actions.

## Future Model Management UI

Phase 6.2 adds text-only provider catalog visibility. Later model-management UI may organize
installed, downloadable, recommended, local, and cloud models from catalog metadata, but setup,
credentials, downloads, and execution must remain behind explicit future C++ boundaries rather than
QML logic.

Phase 6.3 adds text-only task planning visibility. Later planning UI may show capability graphs and
task steps, but those views should remain read-only until a later explicit execution phase adds
separate approval and execution controls.

Phase 6.4 adds text-only agent registry visibility. Agent panels may show static names, roles,
state summaries, and preferred-agent metadata, but must not expose autonomous toggles, execution
buttons, setup controls, tool controls, provider calls, or background-worker controls.

Phase 6.5 adds text-only memory taxonomy visibility. Memory surfaces may show static category
summaries, retention/privacy labels, and planner affinity metadata, but must not expose semantic
search, vector graph execution, autonomous memory writes, embeddings, provider calls, or tool
controls.

Phase 6.6 adds text-only orchestration snapshot visibility. Dashboard surfaces may show snapshot
health, summary, and compact signals, but must not expose execution controls, autonomous toggles,
provider setup, semantic search, vector graph execution, downloads, or background-worker controls.

Phase 6.7 adds text-only orchestration readiness visibility. Dashboard and Settings may show
diagnostic status, summary, and ordered diagnostic lines, but must not add setup buttons, provider
configuration, probing actions, execution controls, autonomous toggles, filesystem scans, downloads,
or background-worker controls.

Phase 6.8 adds text-only conversation/session context visibility. Dashboard and Settings may show
session id/status, interaction mode, attention state, and context-window summary, but must not add
chat-thread management, execution controls, provider/model setup, streaming controls, filesystem
scans, semantic search, downloads, or autonomous worker controls.

Phase 6.9 adds text-only conversation state graph visibility. Dashboard and Settings may show the
current state, last transition status, and last transition summary, but must not add approval
buttons, execution controls, provider/model setup, streaming controls, filesystem scans, semantic
search, downloads, or autonomous worker controls.

Phase 6.10 is a pre-runtime checkpoint. It should not add UI redesigns or controls. Any Phase 7.0
runtime-boundary UI should start as read-only metadata unless a later explicit scope adds approved
runtime behavior behind C++ boundaries.

Phase 7.0 adds read-only local runtime boundary visibility. Settings may show runtime status,
health, capabilities, and refusal summary, but must not add runtime setup, provider calls, model
execution, downloads, streaming controls, process launch, filesystem scans, tool/plugin controls,
or autonomous worker controls.

Phase 7.1 adds read-only local runtime session visibility. Settings may show session count, status,
health, allocation summary, and reservation summary, but must not add session controls, model
allocation actions, process launch, provider calls, downloads, streaming controls, scans,
tool/plugin controls, or autonomous worker controls.

Phase 7.2 adds read-only runtime capability negotiation visibility. Settings may show capability
counts, enabled/disabled capability summaries, negotiation summary, and local-only enforcement
metadata, but must not add toggles, activation buttons, setup flows, downloads, provider/model
execution controls, filesystem/process controls, tool/plugin controls, or autonomous worker
controls.

Phase 14.7 through Phase 15.0 activates controlled local Ollama chat while keeping UI changes
minimal. Dashboard and Chat may show runtime availability, selected model, inference state, and
streaming state. Settings may allow selecting discovered local Ollama models and toggling explicit
local chat inference/streaming. These controls must remain narrow: no cloud setup, API keys,
model downloads/pulls/deletes, tool controls, shell controls, filesystem-wide actions, voice
record/play controls, or autonomous-agent controls.

Phase 15.1 through Phase 15.3 refines Voice Configuration without broad redesign. Settings may use
shorter labels, explicit help text, an Apply Paths action, compact Ready/Blocked/Missing badges,
and exact validation rows for Piper/Whisper paths. These controls remain configuration-only: no
speak, play, record, microphone, playback, downloads, Piper execution, Whisper execution, broad
filesystem scan, cloud/API-key setup, or autonomous voice loop.

Phase 15.4 through Phase 15.6 adds the narrow Piper file-output execution UI. Settings may expose
a disabled-by-default opt-in, an explicit Generate TTS File action, clear execution status, and a
generated file path summary. The UI must not expose playback, microphone recording, Whisper
execution, arbitrary output paths, downloads, cloud/API-key setup, broad filesystem scans, or an
autonomous voice loop.

Phase 15.7 stabilizes local Ollama reliability without a broad redesign. Chat may disable the
input and Send button while local inference is active and may show concise inference failure
summaries near runtime status. Error copy should distinguish stopped/unreachable Ollama, missing
or invalid local model, timeout, malformed response, interrupted stream, permission/safety block,
and duplicate busy request without exposing raw internals.

Phase 15.9 adds concise conversation runtime visibility. Chat may show current session state,
active route, and active request id while a request is known. The surface should stay compact and
avoid raw traces, worker details, provider objects, database paths, or debug dumps. Clear Chat
should visually return the transcript/runtime status to a clean single-system-message state.

Phase 15.10 improves persistent conversation UX without redesigning Chat or Settings. Chat and
Settings may show compact persisted/runtime-only status, message count summary, and last
save/restore status. Clear Chat confirmation should be explicit that runtime transcript,
persisted local chat history when available, active request metadata, and live streaming text are
reset while settings and memory remain separate. No transcript browser, thread list, export/import,
search, pruning, encryption, or advanced history-management UI is added.

Phase 15.11 through Phase 15.13 add compact transcript QA visibility. Chat may show current
in-memory search status and disabled export readiness, plus a small search field scoped to the
current transcript only. Settings may show read-only search/export summaries. The UI must not show
a file picker, enabled export button, transcript browser, thread list, semantic/vector search
controls, indexing controls, import controls, pruning, encryption, tools/plugins, or filesystem
actions.

Phase 15.14 through Phase 15.16 add narrow current-transcript export actions. Chat and Settings
may show Export Markdown and Export JSON buttons plus the last export status and safe filename.
The UI must not expose raw filesystem paths, a file picker, custom output locations, import,
thread browsing, multi-conversation export, cloud sync, external process controls, or broad
filesystem actions.

Phase 15.17 through Phase 15.19 add a compact current-transcript browser-readiness section.
Chat/Settings may show browser status plus one current transcript entry summary (title, message
count, persistence, last updated/saved summary, and search/export availability summaries). This is
single-transcript foundation only: no full browser page, no sidebar redesign, no thread controls,
and no multi-conversation storage UI.

Phase 15.20 through Phase 15.22 add a compact read-only multi-conversation readiness status.

Phase 16.0 through Phase 16.6 add compact Memory Candidates visibility and review controls to the
Memory page. The UI may show total, pending, approved, rejected, and archived counts, last review
result metadata, candidate summary rows, and Approve/Reject/Reset actions. Approved must be
presented as reviewed metadata, not committed long-term memory. It must not add automatic capture
toggles, semantic search, vector/embedding controls, provider/model calls, filesystem controls,
cloud sync, tool/plugin controls, broad redesign, or autonomous memory actions.

Phase 16.7 through Phase 16.9 add compact Commit Readiness visibility to the existing Memory
Candidates section. The UI may show readiness status, readiness checks, planned candidate count,
target summary, per-candidate plan summaries, and last commit request result. It must make clear
that Approved is not Committed and that commit is future-gated. It must not add an enabled commit
or store action, automatic key-value memory writes, semantic search, vector/embedding controls,
provider/model calls, filesystem controls, cloud sync, tool/plugin controls, broad redesign, or
autonomous memory actions.
Settings may show current storage mode (`Single Transcript`), future mode (`Multi Conversation`),
migration readiness (`Not Started`), migration status summary (`Not Started / Planned`), and a
schema status summary. This remains planning-only metadata: no schema migration, no
multi-conversation persistence, and no new browser/thread controls.

Phase 15.23 through Phase 15.25 add read-only multi-conversation store readiness. Settings may show
conversation-store status, conversation count, and an active-conversation summary. The active chat
surface remains the existing single transcript; no thread sidebar, import UI, cloud sync, or broad
layout redesign is introduced.

Phase 15.26 through Phase 15.29 add a compact Conversation Browser surface inside Chat. The
browser may show conversation title, last updated summary, message count, and archived state, with
small create, switch, rename, archive, and unarchive actions. It must preserve the current
Sentinel bridge composition and bottom dock, avoid a chat-app/sidebar redesign, and avoid delete,
cloud sync, import/export workflow expansion, embeddings/vector search, semantic memory, tool
execution, or runtime safety changes. Switching should feel like loading a local transcript: live
preview disappears, active request metadata resets, and the selected messages replace the visible
thread without duplicate rows.

Phase 15.30 through Phase 15.32 polish that compact browser without changing its shape. Chat may
make the current conversation more obvious, visually mute archived rows, show an empty-state hint
when no user-created conversations exist, and provide compact rename success/refusal feedback.
Archived active conversations should show a direct hint and disable message sending until
unarchived. Settings may show active/archived counts plus archive-first delete readiness. Delete
controls remain status-only: no destructive delete button, confirmation flow, filesystem action,
cloud sync, import, broad redesign, model/voice/tool/plugin behavior, or runtime-policy change is
introduced.

Phase 15.33 through Phase 15.35 is a runtime QA/checkpoint pass for the same browser. It may update
documentation and focused tests for browser/session-switch/delete-readiness behavior, but should
not change the compact browser shape or add new user workflows. Archived active conversation
blocking, delete-readiness status, and stale-response safety remain expressed through existing
QML-safe view-model metadata.

## Current Separation

Current UI work remains presentation and metadata visibility only:

- no cloud provider integrations
- no non-loopback networking/API keys
- no model downloads/pulls/deletes
- no model execution outside explicit local Ollama chat inference
- no plugin loading
- no real tool execution
- no approval UX that triggers actions
- no sandbox runtime
- no filesystem/system actions beyond controlled app-owned transcript export and Piper file output
- no particle systems or assistant-face rendering

Manual visual QA expectations are tracked in `docs/UI_QA_CHECKLIST.md`.
