# UI/UX Plan

Phase 5.0 establishes a small design-system foundation for Sentinel Desktop. It does not implement
advanced motion, assistant visuals, model management, provider integration, or execution features.

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

Guidelines:

- Prefer shared tokens for repeated colors, spacing, radii, and text sizes.
- Keep component styling close to existing QML patterns.
- Do not move business logic into QML.
- Avoid broad visual rewrites during foundation work.

## Motion Guidelines

Motion is future work. Phase 5.0 only records constraints:

- Keep animation short, optional, and performance-friendly.
- Favor small transitions for panel state, navigation, and assistant feedback.
- Avoid heavy continuous animation on idle dashboards.
- Do not implement particle effects or an assistant face yet.
- Preserve acceptable behavior on Fedora KDE Plasma, macOS, and lower-power devices.

## Future Assistant Visuals

Future assistant visuals may include subtle status presence, listening/thinking indicators, and
model/runtime readiness states. These visuals should consume metadata from view models and must not
start provider calls, model execution, tool execution, or system actions.

## Current Separation

Current Phase 5.0 work is presentation foundation only:

- no provider integrations
- no networking/API keys
- no model downloads or execution
- no plugin loading
- no real tool execution
- no approval UX that triggers actions
- no sandbox runtime
- no filesystem/system actions
