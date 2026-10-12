# Sentinel

![Sentinel](resources/branding/docs/readme-header.svg)

Sentinel is a cross-platform AI agent platform built with C++20, Qt 6, and QML. Desktop and CLI are interfaces over shared application and core services; agent execution, tools, providers, memory, permissions, plugins, and MCP live behind those shared boundaries.

The Desktop is the primary user surface and Fedora KDE Plasma is the primary platform target. Linux, macOS, and Windows are built in CI. Sentinel can use a local Ollama runtime and includes provider/model, tool, plugin, MCP, local persistence, and controlled agent-execution infrastructure. Cloud use requires user-supplied credentials and an allowed network policy.

Desktop and the daemon-backed Rust CLI/TUI expose implemented runtime services. AgentRuntime acceptance remains incomplete; successful fixture tests do not establish reliable live task completion. Native Windows CLI/TUI named-pipe IPC is unavailable. Plugins, MCP, and Skills remain experimental; sandbox enforcement is platform-dependent.

The TUI V4-B macOS arm64 package is conditional local evaluation material, **not cleared for public distribution**. License/source/artwork, physical terminal acceptance, and signing/notarization gates remain open. See the [public repository audit](docs/development/SEC_P1_PUBLIC_REPOSITORY_AUDIT.md) and [TUI release evidence](docs/development/TUI_V4_B_FINAL_RELEASE_REPORT.md).

## Start here

- [Documentation](docs/README.md)
- [Installation](docs/getting-started/INSTALLATION.md)
- [Desktop](docs/interfaces/DESKTOP.md) and [CLI](docs/interfaces/CLI.md)
- [Architecture](docs/development/ARCHITECTURE.md) and [Agent Runtime](docs/concepts/AGENT_RUNTIME.md)
- [Security model](docs/security/SECURITY_MODEL.md)
- [Building](docs/development/BUILDING.md), [testing](docs/development/TESTING.md), and [contributing](CONTRIBUTING.md)
- [Roadmap](docs/ROADMAP.md)

## License

Sentinel is licensed under GPL-3.0-or-later; see [LICENSE](LICENSE). Third-party assets and dependencies retain their own terms; see [third-party notices](THIRD_PARTY_NOTICES.md).
