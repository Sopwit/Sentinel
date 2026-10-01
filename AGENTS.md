# Sentinel agent instructions

Sentinel is a C++20, Qt 6/QML modular monolith. Fedora KDE Plasma is the primary target; preserve portability across Linux, macOS, and Windows.

Before architecture-sensitive work, read [architecture](docs/development/ARCHITECTURE.md), [Agent Runtime](docs/concepts/AGENT_RUNTIME.md), [security model](docs/security/SECURITY_MODEL.md), [building](docs/development/BUILDING.md), and [testing](docs/development/TESTING.md).

- Keep QML presentation-only and expose QML-safe view models.
- Agent Mode and controlled tasks execute through `AgentRuntime`; only an accepted `AgentLoop` final answer completes a run.
- Resolve active runs through `IModelRouter` and one `ModelBinding`; keep provider behavior behind `IChatProvider`.
- `ToolDescriptor`/registry contracts and `ToolExecutionGateway` are authoritative. Filesystem tools use `IFileSystemService`; processes use `ProcessExecutor`.
- Keep settings, memory, chat history, conversations, agent runs, and permission grants separate. SQLite uses Qt SQL.
- Preserve explicit permission, approval, sandbox, and platform-service boundaries. Natural-language input must never become a shell command through heuristic planning.
- Do not introduce platform-only core logic, Electron, a Python backend, or unrelated dependencies.

Validate scoped changes with the relevant tests; broad shared changes require:

```bash
cmake --preset tests
cmake --build --preset tests
ctest --preset tests --output-on-failure
```
