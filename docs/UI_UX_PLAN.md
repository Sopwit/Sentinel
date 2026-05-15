# UI/UX Plan

Phase 5 establishes a small design-system and interaction foundation for Sentinel Desktop. It does
not implement advanced motion, assistant visuals, model management, provider integration, or
execution features.

## Direction

- Native Qt/QML desktop experience optimized first for Linux/Fedora KDE Plasma.
- Cross-platform-safe visual language that remains viable on Windows and macOS.
- Quiet, operational assistant dashboard rather than a marketing-style interface.
- Dense but readable information surfaces for chat, local memory, runtime metadata, and settings.
- Local-first posture: UI should make unavailable networking/provider capability clear without
  implying hidden cloud behavior.

## Design System Foundation

The initial QML design tokens live in `ui/qml/theme/SentinelTheme.qml`.

Token categories:

- palette: background, panel, surface, text, accent, success, and error colors
- spacing: small fixed layout spacing values
- radius: restrained panel and control rounding
- typography: stable pixel sizes for current desktop shell text
- motion: standard durations and easing values for low-cost UI transitions
- layout: compact and wide breakpoints, sidebar widths, and small spacing helpers

Guidelines:

- Prefer shared tokens for repeated colors, spacing, radii, and text sizes.
- Prefer small shared components for repeated controls such as command buttons, text fields, and
  read-only status rows.
- Keep component styling close to existing QML patterns.
- Do not move business logic into QML.
- Avoid broad visual rewrites during foundation work.

## Component Consistency Guidelines

- `SentinelButton` owns command button height, hover/focus colors, and disabled opacity.
- `SentinelTextField` owns input text color, focus border, and control height.
- `InfoRow` owns read-only label/value status row typography and compact stacking.
- `MetricCard` and nested panels should use shared card padding.
- Section headings should wrap cleanly and keep page subtitles muted.

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
- Do not implement particle effects or an assistant face yet.
- Preserve acceptable behavior on Fedora KDE Plasma, macOS, and lower-power devices.
- Avoid blur-heavy layers, shader-heavy rendering, or custom OpenGL/Vulkan render paths.

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
