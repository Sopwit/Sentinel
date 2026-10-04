# CLI and terminal UI

The Rust `sentinel` binary connects to `sentinel-daemon` over [typed local IPC](IPC.md). It never links the C++ core or auto-starts a hidden daemon. Start the daemon explicitly. Rust **1.94** is the declared minimum; the workspace is `cli/`.

```bash
cargo +1.94.0 build --locked --manifest-path cli/Cargo.toml
./cli/target/debug/sentinel status
./cli/target/debug/sentinel models
./cli/target/debug/sentinel sessions
./cli/target/debug/sentinel chat "Yalnızca MAVI yaz."
./cli/target/debug/sentinel agent "List the actual workspace root entries."
echo "Read CMakeLists.txt and report its project name." | ./cli/target/debug/sentinel run
./cli/target/debug/sentinel --output json chat "hello"
./cli/target/debug/sentinel --output stream-json agent "task"
./cli/target/debug/sentinel tui
./cli/target/debug/sentinel attach SESSION_ID
./cli/target/debug/sentinel shutdown
```

Global `--socket PATH` overrides the private default endpoint; `--version` derives from canonical `SENTINEL_APP_VERSION`. `run` is the non-interactive Agent command. An explicit argument wins over stdin. Text mode emits final output to stdout and diagnostics to stderr. JSON mode emits structured terminal payload/state; `stream-json` emits daemon events as newline-delimited JSON. Commands such as status/models/sessions return their structured response payload.

Interactive CLI approvals prompt on stderr and accept `y`; otherwise deny. Non-terminal stdin denies approvals explicitly. No automatic grant flag is supplied. Ctrl+C sends `run.cancel` through a separate IPC control connection, including while awaiting approval. Cancellation does not imply daemon shutdown.

| Exit | Meaning |
|---|---|
| 0 | Success |
| 1 | Runtime/input failure or denied approval |
| 2 | Cancelled |
| 3 | Daemon unavailable/disconnected or unsupported transport |
| 4 | Protocol major incompatibility |

Reconnect explicitly by invoking another command or `attach`; the daemon keeps active work alive after client loss. Oversized output/backpressure errors are visible failures, never synthetic successful completion.

## Ratatui foundation

The `sentinel-tui` crate uses the same IPC client. `sentinel tui` opens session navigation, output, composer and a state/provider/model/version line. Up/Down select a session and attach its snapshot; Enter sends Chat. Enter `/agent task` to start Agent. Output deltas render incrementally, and terminal events replace the final text rather than append duplicates. Ctrl+C cancels active work; when idle it exits. Approval modal shows tool, risk/resources and runtime detail: `y` allows, `n` or Esc denies. Grants are one approval only. Terminal state is restored on exit/error. This is foundation UI, not final visual polish.

## Legacy C++ CLI

`sentinel-cli` remains the existing in-process diagnostic/configuration/model interface for Desktop compatibility. Its exact commands are documented in the [legacy CLI reference](../reference/CLI_REFERENCE.md). It is not the daemon-connected Rust binary and is planned for retirement only after the Desktop IPC migration passes equivalent regression gates.
