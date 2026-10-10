# V3.2.3 execution environment

## Verified baseline

macOS 27.0.1 arm64; clean baseline fbfc41d8. Personal-Brain CLI unavailable. Repository architecture, runtime, security, build/testing instructions and V3.2.2 reports/evidence were read. No private project is an acceptance target.

Before production changes, the existing real gateway/executor diagnostic was repeated in a temporary workspace. `cmake --version` exited 127; absolute Homebrew CMake exited 126 (Operation not permitted); two `/usr/bin/true` commands exited 128 (fork denied). New raw logs live under `docs/reviews/agent-v323-2026-10-10/`. The diagnostic helper's test-fixture approval is integration evidence, not autonomous Agent acceptance.

StaticSandboxPolicy marks run-command as requiring process-tree control. ToolExecutionGateway resolves exact command and workspace authority. RealToolExecutor delegates to ProcessExecutor, whose macOS sandbox grants its selected launcher and approved paths, and denies process-fork when detached children are forbidden. This is stronger than ordinary process-group cleanup. A `/bin/sh` launcher does not authorize Homebrew CMake's executable/dependency tree. Fixing PATH or adding a CMake read path cannot resolve fork confinement.

ProcessExecutor establishes a new session and signals its process group, but a permitted descendant could create another session/group. A PID snapshot followed by kill is subject to races. macOS sandbox-exec lacks the required kill-on-owner-exit primitive. Removing deny-fork or reporting a process group as full descendant ownership would weaken the existing guarantee; neither was done.

## Narrow design — SPECIFIED ONLY

A future authoritative process capability needs a structured canonical executable + argv contract, canonical toolchain/SDK dependency roots mounted/readable without writable host prefixes, an explicit working directory and private temporary directory, restricted environment, network denial, bounded stdout/stderr, wall-time and resource limits, and exact approval binding. CMake's project files are executable untrusted inputs, not a reason to authorize arbitrary host access. The daemon must audit launch identity, scope and outcomes through the existing gateway; Rust must not launch a second shell path.

Before permitting children, the backend needs ownership which survives descendant exec/setsid/double-fork and reliably terminates descendants on cancellation, timeout and supervisor/daemon crash. A separately reviewed containment/supervisor or isolated VM execution backend may supply this; ordinary macOS process groups and polling do not. Entitled OS mediation and VM designs require separate engineering/security evaluation, not an unverified promise in this phase.

Required adversarial tests include detached grandchildren, parent/supervisor crash, PID reuse, symlink/executable replacement, dependency resolution, writes outside workspace/temp, secret-bearing environment, networking, timeout, output flooding and cancellation while compiler children are active. A new capability must remain fail-closed when the necessary containment is unavailable.

## Linux comparison

The existing Linux implementation uses Bubblewrap user/PID/IPC namespaces, die-with-parent, read-only system roots, scoped workspace binds, restricted environment and optional network namespace. That is a materially different containment mechanism. It must be exercised on a Linux host before acceptance is claimed.

The available Docker CLI points to desktop-linux, but its daemon socket is absent. No usable Linux runtime was observed. No VM/container image was downloaded, no Docker service was bootstrapped and no unknown SSH host was contacted. Linux comparison is NOT VALIDATED; this is an environment limitation, not a Linux failure or success.

macOS authorized CMake build and cancellation-during-build remain BLOCKED. No sandbox, approval scope or process path was relaxed.
