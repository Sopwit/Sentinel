# Roadmap

This roadmap describes direction, not a release contract. Current functionality is documented elsewhere in this tree. The [platform roadmap](roadmaps/SENTINEL_PLATFORM_ROADMAP.md) and [acceptance matrix](roadmaps/ACCEPTANCE_MATRIX.md) define proposed P0–P12 scope and evidence requirements; this overview does not change their thresholds or adoption status.

## In progress

- Agent Runtime: harden real local/provider execution, tools, observations, and cancellation.
- Providers and models: improve local runtime support and credential-gated cloud integrations.
- Context and memory: continue local semantic retrieval and session-quality work.
- Security and sandbox: deepen policy enforcement and platform-specific isolation.

## Planned

- Desktop: improve the Qt/QML workspace and platform integration, with Fedora KDE Plasma remaining the primary target.
- CLI/TUI: harden the implemented daemon-backed Rust terminal surfaces. The small in-process C++ dispatcher remains a legacy compatibility exception; its disposition is pending.
- Extensions: stabilize and expand compatibility, lifecycle, and trust-boundary work for the existing experimental plugin and MCP foundations.
- Developer experience: improve extension guidance, diagnostics, and release validation.

## Exploratory

- Additional IDE/API experiences beyond the implemented daemon IPC used by production Desktop and Rust CLI/TUI; existing WebUI/ACP stubs are not product integrations.
- Broader platform support and deployment integration.

Planned interfaces are not current product claims. See [Architecture](development/ARCHITECTURE.md) for implemented executable surfaces.
