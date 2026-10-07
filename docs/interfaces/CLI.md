# CLI and terminal UI

The canonical Rust executable, `sentinel`, connects to `sentinel-daemon` over [typed local IPC](IPC.md). It never links the C++ core or starts a hidden daemon. Rust **1.94** is the declared minimum; the Cargo workspace is `cli/`.

For installation, commands, stable output/exit contracts, provider/workspace selection and automation, see the [CLI user guide](../user/CLI.md). For composer keys, pickers, approvals, cancellation, reconnect, file references and Applied change review, see the [TUI user guide](../user/TUI.md).

```sh
./tools/terminal/install-dev.sh
sentinel --help
sentinel status --output json
sentinel tui
```

CLI and TUI retain daemon-owned conversations, immutable model bindings, permissions and AgentRuntime execution. Chat and Agent are explicit modes. Enter inserts a composer newline; Ctrl+S sends. Ctrl+C requests cancellation during active work and exits while idle. Esc preserves a pending approval. Unsupported compaction/revert operations are not simulated.

The terminal uses additive IPC 1.1 operations. `terminal.attach` returns the bounded safe session snapshot, avoiding Desktop's full presentation projection; `session.attach` retains its existing Desktop behavior. `terminal.state` uses cached ModelService readiness without inspecting unobserved cloud credentials. Discovery and runtime preflight remain explicit existing authoritative operations.

## Legacy C++ CLI

`sentinel-cli` remains the existing in-process diagnostic/configuration/model interface for Desktop compatibility. Its commands are documented in the [legacy CLI reference](../reference/CLI_REFERENCE.md). It is separate from the daemon-connected Rust binary.
