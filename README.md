# Sentinel

Sentinel is a cross-platform AI agent platform built with C++20, Qt 6, and QML. Desktop and CLI are interfaces over shared application and core services; agent execution, tools, providers, memory, permissions, plugins, and MCP live behind those shared boundaries.

The Desktop is the primary user surface and Fedora KDE Plasma is the primary platform target. Linux, macOS, and Windows are built in CI. Sentinel can use a local Ollama runtime and includes provider/model, tool, plugin, MCP, local persistence, and controlled agent-execution infrastructure. Cloud use requires user-supplied credentials and an allowed network policy.

Desktop, the agent runtime, tools, providers, permissions, memory, and sessions are implemented. The CLI and daemon are partial; plugins and MCP are experimental; sandbox enforcement remains platform-dependent.

## Start here

- [Documentation](docs/README.md)
- [Installation](docs/getting-started/INSTALLATION.md)
- [Desktop](docs/interfaces/DESKTOP.md) and [CLI](docs/interfaces/CLI.md)
- [Architecture](docs/development/ARCHITECTURE.md) and [Agent Runtime](docs/concepts/AGENT_RUNTIME.md)
- [Security model](docs/security/SECURITY_MODEL.md)
- [Building](docs/development/BUILDING.md), [testing](docs/development/TESTING.md), and [contributing](CONTRIBUTING.md)
- [Roadmap](docs/ROADMAP.md)

## License

Sentinel is licensed under GPL-3.0-or-later; see [LICENSE](LICENSE).
