# Sentinel daemon development

`sentinel-daemon` is a C++20 `QCoreApplication`: no QML, GUI application or hidden window. `DaemonService` uses the existing `ApplicationControllerBuilder::withStandardDefaults` composition root. That root already owns Chat, AgentRuntime, ModelService, registry, gateway, permission services, SQLite stores and optional MCP/plugin/skill services. Small ordinary C++ accessors expose existing runtime signals/approval operations to the IPC adapter; no second planner or execution policy exists.

Build on the current macOS host:

```bash
cmake --preset no-ccache
cmake --build --preset no-ccache
./build/no-ccache/apps/sentinel-daemon/sentinel-daemon
```

CMake already installs this target. This phase adds no packaging/updater work.

The default daemon profile is Qt's `QStandardPaths::AppDataLocation` for the daemon application identity. It is separate from the Desktop's existing profile so concurrent Stage A runtimes do not share live databases. Provider credentials/configuration are not silently copied from Desktop. Configure daemon settings deliberately. `--profile-name NAME` changes the Qt application identity for isolated testing; `--portable` uses the existing executable-adjacent portable paths. `--socket /private/directory/daemon.sock` selects a private endpoint. Both profile and socket must be coordinated between clients; using different sockets with the same portable profile is not a supported multi-writer configuration.

The service acquires the socket lock before initialization, opens the existing stores, restores workspace/network/provider settings, configures persisted MCP servers and refreshes model discovery. Optional provider/MCP/plugin/speech availability does not determine IPC startup readiness. A missing provider still gives truthful runtime failure, with no new fallback path.

SIGINT/SIGTERM request event-loop shutdown through a polled signal-safe flag. IPC shutdown stops accepting clients and rejects new work, cancels Chat and shuts down AgentRuntime through their existing contracts, then quits. Destruction settles owned services/stores before releasing the socket lock. No detached daemon auto-spawn is implemented in clients.

## Verification

```bash
python3 tools/ipc/generate.py --check
ctest --test-dir build/no-ccache --output-on-failure
cargo +1.94.0 test --locked --manifest-path cli/Cargo.toml
cargo fmt --all --check --manifest-path cli/Cargo.toml
cargo clippy --manifest-path cli/Cargo.toml --all-targets -- -D warnings
```

`test_daemon_ipc` uses deterministic existing Chat/provider and Agent/planner/executor seams, with private temporary sockets/profiles. It covers typed validation, framing, compatibility, correlation, multiple clients, attach/reconnect, cancellation, grounded core Agent final mapping, approval/replay, stale sockets and ownership cleanup. `test_ipc_generated_contract` prevents schema drift. Rust tests use mock local daemons and invoke the actual CLI. Real local-provider and terminal PTY evidence is documented in the Phase 2 report.

## Desktop migration

- **Stage A (this phase):** daemon + shared typed IPC + Rust CLI/TUI. Desktop retains its certified in-process path. Daemon profile is isolated.
- **Stage B:** add a Desktop IPC adapter over existing presentation/view-model interfaces. Design explicit profile ownership/migration and event reconnection before changing the default. Do not run two authorities over one profile.
- **Stage C:** retire Desktop direct runtime composition and the legacy in-process C++ CLI after equivalent lifecycle, approvals, recovery and all baseline tests pass through IPC. Runtime authority then belongs exclusively to daemon.

Protocol/security/session details are in [IPC](../interfaces/IPC.md); terminal usage is in [CLI](../interfaces/CLI.md).

## Shutdown drain and provider lifetime

IPC shutdown acknowledges acceptance and closes the listener before draining.
SIGINT/SIGTERM enter the same idempotent path. Health/automatic model startup stops;
Chat, Agent and conversation-title cancellation is requested. A 25ms event-loop
maintenance timer checks nonblocking `tryShutdown(0)` readiness; it is not a sleep
or an assumption that work has finished. Existing clients receive
`daemon-shutting-down` for new commands. Only a fully drained runtime/title worker
emits shutdownDrained and permits application exit. No worker is detached or
forcibly destroyed. Late Chat/title results remain cancellation-filtered.

`AgentRuntime::tryShutdown(nonnegative budget)` returns false while borrowed work
is still active. State moves Running -> Stopping -> Stopped; Stopping rejects new
sessions/start/continue. Keep the runtime, providers, executor, stores and callback
dependencies alive until it returns true. Event subscribers must be nonblocking;
this API bounds worker joins, not arbitrary user callback execution. The legacy
void shutdown/destructor remains a synchronous ownership fence and may wait for
non-cooperative code: do not destroy the composition root after an incomplete
tryShutdown result. A malicious/non-returning in-process provider cannot be safely
killed by C++. The daemon remains responsive but Stopping, reports no successful
exit, and retains all ownership. A hard process termination guarantee requires an
externally supervised isolated provider/process; it is not implemented here.
Cancellation does not become a permission grant and never revives a closed run.
