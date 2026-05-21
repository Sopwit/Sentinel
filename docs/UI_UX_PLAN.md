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

Phase 18.0 through Phase 18.9 add compact agent task runtime, queue, lifecycle, planning-session,
and arbitration readiness visibility to Agents. The surface may show task runtime status, queue
count, planned/active/blocked/completed/refused counts, latest task summary, latest lifecycle
summary, runtime boundary summary, ordered trace summaries, planning status, planning/refusal
counts, arbitration summaries, refusal summaries, and deterministic fallback summary. It must
remain read-only and must not add execution buttons, approval controls, tool/plugin UI,
filesystem or shell controls, autonomous toggles, cloud setup, or a broad redesign.

Phase 18.10 through Phase 18.12 add compact capability-registry visibility to Agents. The surface
may show capability registry status, enabled/disabled/restricted counts, capability summaries,
readiness summaries, and safety summaries. Disabled and refused future runtime capabilities should
be visibly distinct as restricted metadata. The UI must not add enable buttons, execute buttons,
approval workflows, tool/plugin controls, filesystem or shell controls, autonomous toggles, cloud
setup, or a broad redesign.

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

Phase 16.10 through Phase 16.12 add a narrow explicit Commit action to the existing Memory
Candidates section. Commit is visible only for Approved candidates, and Approve remains labeled as
a review decision while Commit is labeled as storing to local memory. The UI may show committed
count, committed status, committed key/summary, readiness, duplicate-refusal result, and last
commit result through QML-safe strings/lists/counts. It must not add automatic commit on approval,
bulk commit, overwrite controls, semantic search, vector/embedding controls, provider/model calls,
cloud sync, tool/plugin controls, broad redesign, or filesystem/system actions beyond the existing
key-value memory store.

Phase 16.13 through Phase 16.15 add a compact “Local Memory Recall” surface to the Memory page.
The UI may show recall policy/status, committed memory entry count, result count, a literal
key/value query field, and compact matching committed entries. It must label recall as local and
read-only. It must not inject results into Chat automatically, imply semantic recall, add
vector/embedding controls, provider/model calls, broad redesign, cloud sync, tool/plugin controls,
or filesystem/system actions.

Phase 16.16 through Phase 16.18 add a compact “Context Assembly” readiness section to Memory or
Settings. The UI may show conversation context availability, committed memory availability,
runtime metadata availability, orchestration metadata availability, source counts, candidate block
counts, simple size estimates, and readiness checks. It must describe the feature as planning
metadata only. It must not add prompt assembly controls, automatic context attachment, provider or
model calls, semantic ranking, embeddings/vector controls, cloud/API-key setup, tools/plugins,
filesystem/system actions, or broad redesign.

Phase 16.19 through Phase 16.21 add a guarded “Use local memory/context in chat” Settings toggle
and compact Chat context status. The UI may show whether injection is enabled, the last injection
status, injected block count, source summary, and size summary. It must not show the raw assembled
prompt or private context payload. It must not add semantic/vector search controls, cloud/API-key
setup, tools/plugins, filesystem/system actions, broad redesign, or voice/runtime controls.

Phase 16.22 through Phase 16.24 add compact conversation-window readiness and truncation status.
Chat may show window status, included message count, and truncated count near the existing context
status. Settings may show budget and omitted/truncated summaries. The UI must stay summary-only:
no raw prompt, no private context payload, no transcript dump, no semantic/vector controls, no
model/provider setup expansion, no tools/plugins, no filesystem/system actions, and no broad
redesign.

Phase 16.25 through Phase 16.27 add compact deterministic conversation-summary readiness status.
Chat may show summary status, block count, and truncated block count near context/window status.
Settings may show summary budget, summarized-message count, omitted-message count, and block
counts. The UI must keep this as metadata/status only: no raw prompt, no full summary payload
display, no semantic summarization controls, no embeddings/vector controls, no provider/model
setup expansion, no tools/plugins, no filesystem/system actions, and no broad redesign.

Phase 16.28 through Phase 16.30 add compact deterministic retrieval-planning status. Chat may show
retrieval status, selected source count, and excluded count near existing context/window/summary
status. Memory and Settings may show retrieval readiness, budget summary, selected/excluded source
counts, selected candidate count, and truncated count. The UI must remain summary-only: no debug
console, no raw prompt, no private assembled payload display, no semantic/vector controls, no
embedding controls, no provider/model setup expansion, no tools/plugins, no filesystem/system
actions, and no broad redesign.

Phase 16.31 through Phase 16.33 add compact semantic/vector readiness status. Memory and Settings
may show semantic retrieval disabled state, embedding provider readiness, vector index readiness,
indexed item count, summaries, and checks. The copy must make clear that semantic retrieval is not
active. The UI must not add semantic search controls, embedding/vector debug UI, raw vector
display, vector score display, provider setup, vector database setup, prompt payload display,
filesystem/system actions, tools/plugins, or broad redesign.

Phase 16.34 through Phase 16.36 add compact semantic candidate orchestration and hybrid retrieval
readiness status. Memory and Settings may show candidate status, candidate counts, budget summary,
arbitration summary, participation summaries, and hybrid deterministic+semantic readiness. The
copy must make clear that semantic retrieval is not active and deterministic retrieval remains
authoritative. The UI must not add semantic search controls, rank controls, embedding/vector debug
UI, raw vector or score display, raw candidate payload display, prompt payload display, provider
setup, vector database setup, filesystem/system actions, tools/plugins, debug console UI, or broad
redesign.

Phase 16.37 through Phase 16.39 is a readiness checkpoint. UI work is limited to keeping existing
Memory, Chat, and Settings surfaces truthful: deterministic retrieval authoritative, semantic
retrieval disabled, prompt context injection opt-in, and semantic/vector exposure summary-only. It
must not add activation toggles, search/rank controls, vector database setup, provider setup, raw
candidate payloads, raw vectors, vector scores, prompt payload display, filesystem/system actions,
tools/plugins, debug console UI, or broad redesign.

Phase 16.40 through Phase 16.42 polishes runtime UX and context visibility without changing
runtime behavior. Memory may show a clearer context pipeline across committed memory, literal
recall, context assembly, retrieval planning, and semantic readiness, with memory candidates
visually separated from committed key-value memory. Chat may show compact chips/status rows for
context injection on/off, selected retrieval source count, conversation-window status, summary
status, and semantic disabled state. Settings may keep advanced semantic/hybrid readiness details
available but visually secondary. This remains UI visibility polish only: no semantic activation,
embeddings/vector database setup, provider/model calls, cloud/API keys, tools/plugins,
filesystem/system actions, raw prompt payloads, raw vectors, vector scores, debug console UI, or
broad redesign.

Phase 16.43 through Phase 16.45 polishes the runtime surface into a restrained futuristic local AI
operating layer. Home may use layered radial glow, a smoother orbital core, compact runtime
badges, idle breathing motion, and subtle state emphasis for deterministic retrieval authority,
context assembly availability, semantic retrieval disabled policy, local runtime availability,
and streaming activity. Chat and Memory may use animated chips, soft edge lighting, and improved
translucent hierarchy to separate the chat panel, runtime surface, memory/retrieval panels, and
dock. This is presentation-only: no retrieval authority change, no semantic activation, no
embeddings/vector database setup, no provider/model calls, no cloud/API keys, no filesystem or
system actions, no tools/plugins, no microphone/playback changes, and no broad navigation
redesign.

Motion philosophy for this phase is cinematic but quiet. Prefer declarative opacity, scale,
rotation, border/color transitions, and slow pulse animations. Avoid aggressive animation, layout
translation, resize-driven effects, custom shaders, blur-heavy surfaces, expensive timers, and
high particle counts. Status components should reserve stable dimensions so hover, active, and
selection states do not resize surrounding layouts. The surface should remain smooth on Fedora KDE
Plasma and macOS Apple Silicon.

Phase 17.0 through Phase 17.3 add compact Semantic Provider readiness visibility. Memory and
Settings may show selected semantic provider, provider mode, readiness, health, capability
summaries, activation readiness, disabled-by-default state, and required activation steps. This is
status/readiness only: no activation button, provider setup flow, embedding/vector debug UI, raw
vectors, scores, raw prompt payloads, config paths, cloud/API-key setup, downloads, filesystem
scans, tools/plugins, system actions, semantic prompt injection, or broad redesign.

Phase 17.4 through Phase 17.6 add compact semantic arbitration simulation and embedding runtime
planning visibility. Memory and Settings may show simulated semantic readiness, arbitration
readiness, future hybrid retrieval summaries, embedding runtime readiness, estimated runtime
budget, local-only requirements, and disabled constraints. The UI must stay summary-only: no raw
vectors, raw score payload dumps, rank controls, provider setup, vector database setup, filesystem
indexing controls, Ollama embedding controls, prompt payload display, tools/plugins, autonomous
actions, or broad redesign.

Phase 17.7 through Phase 17.9 add compact isolated embedding runtime readiness visibility. Memory
and Settings may show isolated runtime readiness, last isolated embedding test status, provider
health, local-only bounded state, timeout/busy/stale/refusal summaries, and safety checks. The UI
must stay read-only and summary-only: no raw vectors, no debug payload dumps, no rank controls, no
semantic search controls, no activation button, no provider setup, no filesystem indexing
controls, no vector database controls, no prompt payload display, no cloud/API-key setup, no
downloads, no tools/plugins, no autonomous actions, and no broad redesign.

Phase 17.10 through Phase 17.12 add compact local vector persistence readiness visibility. Memory
and Settings may show vector persistence readiness, index lifecycle status, bounded/local-only
state, disabled-by-default state, indexed item count, and safety checks. The UI must stay
summary-only and must not show raw vectors, filesystem paths, index handles, debug payload dumps,
semantic search controls, activation controls, automatic indexing controls, filesystem scanning
controls, cloud/API setup, downloads, tools/plugins, autonomous actions, or a broad redesign.

Phase 17.13 through Phase 17.15 add compact bounded semantic search visibility. Memory and Settings
may show semantic search readiness, local-only runtime state, candidate count, bounded search
summary, hybrid arbitration summary, and safety checks. Semantic search remains
non-authoritative: the UI must not expose semantic prompt-injection controls, ranking override
controls, filesystem indexing controls, background ingestion controls, cloud/API provider setup,
provider downloads, raw vectors, prompt payloads, debug payload dumps, tools/plugins, autonomous
actions, or a broad redesign.

Phase 17.16 through Phase 17.18 add compact hybrid bridge visibility. Memory and Settings may show
bridge readiness/status, deterministic-vs-semantic participation, bounded candidate counts,
arbitration summaries, fallback status, and local-only/non-authoritative checks. The UI must not
show raw vectors, raw prompt payloads, provider handles, filesystem paths, debug dumps, semantic
prompt authority controls, ranking override controls, filesystem indexing controls, cloud/API
setup, tools/plugins, autonomous actions, or a broad redesign.

Phase 17.19 through Phase 17.21 add compact semantic acceptance visibility. Memory and Settings
may show acceptance readiness/status, approved supplement counts, deterministic-vs-semantic
participation, fallback state, arbitration summaries, bounded supplement budget, and
local-only/non-authoritative checks. Accepted semantic candidates must be presented as bounded
supplements, not prompt authority. The UI must not show raw vectors, raw prompt payloads, provider
handles, filesystem paths, debug dumps, semantic prompt authority controls, ranking override
controls, filesystem indexing controls, cloud/API setup, tools/plugins, autonomous actions, or a
broad redesign.

Phase 17.22 through Phase 17.24 add compact semantic supplement assembly readiness visibility.
Memory and Settings may show assembly readiness/status, supplement metadata block count, budget
summary, safety report summary, disabled-by-default state, and non-authoritative state. The UI
must present this as separate test-readiness metadata, not live prompt content. It must not show
raw prompt blocks, raw vectors, raw scores, provider handles, filesystem paths, debug dumps,
semantic prompt authority controls, ranking override controls, filesystem indexing controls,
cloud/API setup, tools/plugins, autonomous actions, or a broad redesign.

Phase 17.25 through Phase 17.27 add compact semantic prompt authority policy visibility. Memory
and Settings may show authority status, decision summary, readiness summary, safety summary,
disabled/default-denied state, deterministic fallback summary, audit summary, and
non-authoritative state. This remains status-only: no activation controls, raw prompt display, raw
supplement block display, raw vectors/scores, filesystem paths, provider handles, debug dumps,
semantic ranking override controls, filesystem indexing controls, cloud/API setup, tools/plugins,
autonomous actions, or broad redesign.

Phase 17.28 through Phase 17.30 add compact controlled semantic prompt inclusion visibility.
Settings may expose the explicit semantic supplemental prompt inclusion toggle alongside local
context injection, plus inclusion status, included supplement count, budget summary, fallback/audit
summary, and deterministic-authority-preserved state. Memory may show the same compact status and
included count. The UI must not display raw prompt text, raw supplement content, vectors, scores,
provider handles, filesystem paths, debug payloads, ranking override controls, filesystem indexing
controls, cloud/API setup, tools/plugins, autonomous actions, or broad redesign.

Phase 17.31 through Phase 17.33 checkpoint the semantic UI surface. Memory and Settings may keep
the compact semantic provider/search/bridge/acceptance/assembly/authority/inclusion status rows,
but the UI must continue to present them as bounded local status metadata. Semantic inclusion stays
disabled by default and explicit opt-in. The UI must not add raw prompt viewers, semantic payload
viewers, vector/score displays, filesystem indexing controls, cloud/API setup, provider download
controls, tools/plugins, autonomous actions, ranking override controls, or broad redesign.

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
- no approval UX that triggers autonomous actions
- no sandbox runtime
- no filesystem/system actions beyond controlled app-owned transcript export, Piper file output,
  and explicit local key-value memory commit
- no particle systems or assistant-face rendering

Manual visual QA expectations are tracked in `docs/UI_QA_CHECKLIST.md`.
