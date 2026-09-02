# Sentinel

**Native Cross-Platform AI Operating Layer and Desktop Companion Core.**

---

## Overview

**Sentinel** is a modular, native AI operating layer and desktop companion built from the ground up in **C++20** and **Qt 6 / QML**. Engineered for absolute privacy, offline autonomy, and low resource overhead, Sentinel executes local streaming LLMs, manages isolated workspaces, indexes local knowledge bases via RAG, and orchestrates user-approved autonomous agent workflows — with zero telemetry, zero hidden cloud calls, and no unapproved background execution.

### Core Capabilities

- **Local AI Execution & Streaming:** Real-time streaming LLM chat powered by local loopback engines (Ollama, LM Studio, llama.cpp) and user-configured explicit cloud providers.
- **Isolated Workspaces & Memory Architecture:** Strict context partitioning across projects (Personal, Engineering, Academic, Research) backed by dedicated local SQLite stores (`memory.sqlite3`, `chat_history.sqlite3`, `local_rag.sqlite3`).
- **Explainable Local RAG:** Offline document and vector retrieval for PDF, Markdown, TXT, CSV, JSON, and source code files, featuring dynamic context budget allocation and source attribution.
- **Controlled Foreground Agent Workflows:** Safe, gated tool execution with interactive plan step editing, skip/retry actions, sandbox verification, and explicit human approval gates.
- **Native Liquid Glass Desktop Shell:** Hardware-accelerated translucent matte interface in Qt 6 / QML, tailored for Linux (Fedora KDE Plasma / Wayland), macOS, and Windows.
- **Multi-Client Architecture & CLI Dispatcher:** Unified companion suite offering the graphical desktop shell (`sentinel-desktop`), headless scriptable CLI (`sentinel-cli`), and background daemon (`sentinel-daemon`).

---

## CLI & Desktop Quick Reference

When launched without arguments, `sentinel-desktop` opens the graphical Liquid Glass companion interface. For headless administration, terminal interaction, and scripting, use the CLI commands:

| Command / Shortcut | Description |
| :--- | :--- |
| `sentinel-desktop` | Launches the native Liquid Glass desktop companion shell |
| `sentinel-cli chat "<prompt>"` | Executes a one-shot or interactive terminal chat query |
| `sentinel-cli status [--json]` | Displays health status of local AI providers, stores, and active workspace |
| `sentinel-cli models list` | Enumerates available local and configured LLM models |
| `sentinel-cli workspace switch <id>` | Switches active workspace context and memory scope |
| `sentinel-cli rag query "<text>"` | Performs semantic search across the local document knowledge base |
| `sentinel-daemon` | Runs the headless background companion and IPC coordination service |
| `Ctrl+K` / `Cmd+K` | Opens the universal Command Palette within the desktop shell |
| `Ctrl+N` / `Cmd+N` | Creates a new conversation thread |
| `Ctrl+,` / `Cmd+,` | Opens Workspace & Model Settings |
| `Ctrl+P` / `Cmd+P` | Toggles pinned status for the active conversation |
| `Ctrl+B` / `Cmd+B` | Toggles conversation sidebar visibility |
| `Ctrl+Shift+T` / `Cmd+Shift+T` | Toggles Dark / Light Liquid Glass theme |

---

## Documentation

Comprehensive guides for administrators, maintainers, and developers:

- 📦 **[Installation & Deployment Guide](docs/INSTALL.md):** System requirements, pre-built package deployment (RPM, DEB, AppImage, DMG, MSI), and local AI provider setup (Ollama / LM Studio).
- 🛠️ **[Build & Development Guide](docs/BUILD.md):** Toolchain prerequisites, compiling from source (CMake / Ninja / Qt Creator), running CTest suites, and packaging workflows.
- 🏗️ **[System Architecture & Specification](docs/ARCHITECTURE.md):** Subsystem architecture, C++ core interfaces, SQLite persistence, view models, and security boundaries.
- 🤝 **[Contributing Guidelines](CONTRIBUTING.md):** Development workflow, pull request guidelines, and code standards.
- 🔒 **[Security Policy](SECURITY.md):** Vulnerability reporting, explicit consent model, and sandbox authority rules.

---

## License

Sentinel is open-source software licensed under the **GNU General Public License v3.0 or later** (GPL-3.0-or-later). See [LICENSE](LICENSE) for details.
