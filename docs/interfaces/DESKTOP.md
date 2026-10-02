# Desktop

**Status: Implemented.** Desktop is the primary application surface; its feature set continues to evolve.

`sentinel-desktop` is the Qt/QML application surface. Its bootstrapper initializes logging, graphics, platform integrations, and the QML engine; QML consumes QML-safe view models rather than core objects.

The Desktop presents conversations, Agent Mode activity, controlled tasks, models, settings, permissions, notifications, onboarding, and local runtime status. `DesktopShellViewModel` delegates application behavior to shared controller and service boundaries.

Desktop is not a second agent implementation. Shared execution semantics are documented in [Agent Runtime](../concepts/AGENT_RUNTIME.md). Platform integration is intentionally behind core platform interfaces; Linux has the richest packaging and desktop metadata today.

The desktop uses a single-instance guard. It must render loading, empty, and failure states in QML; UI changes should be validated in a real flow, including keyboard focus and a narrow viewport.
