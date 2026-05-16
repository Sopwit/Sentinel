# UI/UX Plan

Phase 5 establishes a small design-system and interaction foundation for Sentinel Desktop. It does
not implement advanced motion, assistant visuals, model management, provider integration, or
execution features.

Phase 5.4 translates the useful UI direction from the `lovable-tasarim` design reference into
native Qt/QML. For visual identity, `lovable-tasarim` is the source of truth. The
React/Vite/Tailwind project remains a reference only and is not integrated into the production app.

Phase 5.4.5 is a stabilization checkpoint. It should reduce obvious risk in the existing QML and
docs without adding new product features, UI systems, runtime behavior, or web dependencies.

Phase 5.5 reconstructs the visual shell around the `lovable-tasarim` identity more directly:
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
- Workspace-oriented layout: ultra-thin left status rail, central Sentinel presence scene, floating
  right interaction surface, bottom command dock, and compact top system status.
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
- layout: compact and wide breakpoints, sidebar widths, and small spacing helpers
- mode visuals: accent, panel color, glow scale, and status text helpers for Companion, Focus,
  Mission, System, Minimal, and Tactical modes

Guidelines:

- Prefer shared tokens for repeated colors, spacing, radii, and text sizes.
- Prefer small shared components for repeated controls such as command buttons, text fields, and
  read-only status rows.
- Keep component styling close to existing QML patterns.
- Match `lovable-tasarim` composition before adding generic utility-app dashboard conventions.
- Do not move business logic into QML.
- Avoid broad visual rewrites during foundation work.

## Component Consistency Guidelines

- `SentinelButton` owns command button height, hover/focus colors, and disabled opacity.
- `SentinelTextField` owns input text color, focus border, and control height.
- `InfoRow` owns read-only label/value status row typography and compact stacking.
- `MetricCard` and nested panels should use shared card padding.
- Section headings should wrap cleanly and keep page subtitles muted.
- `ShellPanel` owns glass-like panel borders and lightweight corner brackets.
- `SentinelDock` owns bottom navigation and page switching through the existing view model.
- `SentinelOrb` owns the lightweight central presence geometry.
- `SentinelTelemetry` owns small floating readouts for safe view-model metadata.
- `WorkspacePresence` composes the central ambient presence scene and reads only safe view-model
  state.
- `Atmosphere` owns the low-cost ambient shell background.
- Mode-aware helpers in `SentinelTheme.qml` are presentation helpers only; they should not become
  routing, provider, execution, or workflow logic.

## Adaptive Layout Guidelines

The shell should support three practical desktop width categories:

- Compact: narrow windows should reduce margins, use the compact sidebar width, and stack dense
  forms or dashboard panels.
- Normal: default desktop windows should keep the sidebar visible and preserve current page
  structure.
- Wide: wider windows can place dashboard overview and chat side by side.

Guidelines:

- Prefer wrapping grids and smaller margins over hidden controls.
- Keep the sidebar available at compact widths, but allow labels to elide before layout breaks.
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

- `lovable-tasarim` is the visual source of truth for Phase 5.x composition and atmosphere.
- The Dashboard/Core page should prioritize the central presence scene over metric-card density.
- Navigation should feel dock-led and cinematic, with the left rail reduced to ambient identity and
  local status.
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

## Current Separation

Current Phase 5 work is presentation foundation only:

- no provider integrations
- no networking/API keys
- no model downloads or execution
- no plugin loading
- no real tool execution
- no approval UX that triggers actions
- no sandbox runtime
- no filesystem/system actions
- no particle systems or assistant-face rendering

Manual visual QA expectations are tracked in `docs/UI_QA_CHECKLIST.md`.
