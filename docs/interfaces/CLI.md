# CLI and terminal UI

The canonical Rust executable, `sentinel`, connects to `sentinel-daemon` over [typed local IPC](IPC.md). It never links the C++ core or starts a hidden daemon. Rust **1.94** is the declared minimum; the Cargo workspace is `cli/`.

For installation, commands, stable output/exit contracts, provider/workspace selection and automation, see the [CLI user guide](../user/CLI.md). For composer keys, pickers, approvals, cancellation, reconnect, file references and Applied change review, see the [TUI user guide](../user/TUI.md).

```sh
./tools/terminal/install-dev.sh
sentinel --help
sentinel status --output json
sentinel tui
```

CLI and TUI retain daemon-owned conversations, immutable model bindings, permissions and AgentRuntime execution. Chat and Agent are explicit modes. Enter sends; Ctrl+O inserts a composer newline. Ctrl+C requests cancellation during active work and preserves an idle draft, exiting only when empty. Esc preserves a pending approval. Unsupported compaction/revert operations are not simulated.

The terminal uses additive IPC 1.1 operations. `terminal.attach` returns the bounded safe session snapshot, avoiding Desktop's full presentation projection; `session.attach` retains its existing Desktop behavior. `terminal.state` uses cached ModelService readiness without inspecting unobserved cloud credentials. Discovery and runtime preflight remain explicit existing authoritative operations.

## Legacy C++ CLI

`sentinel-cli` remains the existing in-process diagnostic/configuration/model interface for Desktop compatibility. Its commands are documented in the [legacy CLI reference](../reference/CLI_REFERENCE.md). It is separate from the daemon-connected Rust binary.

## Local release-candidate evaluation

Build with `cargo build --release --locked --manifest-path cli/Cargo.toml`, then run
`./tools/terminal/stage-release.sh /absolute/new/stage-directory`. The script only
stages the existing executable, canonical Core Warden runtime animations/contract
and project license; it never installs globally or publishes. Run the staged
`bin/sentinel` from any working directory. Keep `bin/` beside `share/` when moving
it. The daemon remains a separate prerequisite. This is an unsigned local RC
stage, not a macOS installer, notarized package or supported Windows distribution.

The TUI resolves `SENTINEL_CORE_WARDEN_ASSETS` first, then executable-relative
`../share/sentinel/core-warden`, then the standard checkout `cli/target/debug|release`
layout. It never uses a compiled developer home path. An invalid explicit override
stays authoritative and selects text fallback. Copying only the binary is valid
for text operation; graphical assets require the adjacent shared directory or an
explicit asset root. `cargo install` / `install-dev.sh` installs the binary only;
use an explicit asset root or the staged layout for graphics. No shell configuration
is changed automatically. Use the absolute executable path and `--version` when
checking for older PATH-shadowing copies.
