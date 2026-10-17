# P7-02A — Fedora 44 Packaging Foundation & Ro-ASD Readiness Result

Status: **Foundation changes complete — uncommitted, pending maintainer review. Native/Fedora
execution not performed (no Fedora host, no RPM tooling).**

Phase: P7 — Fedora / Ro-ASD Integration
Date: 2026-10-17
Branch: `feature/p7-02a-fedora-packaging-foundation`
Base and verified `main`: `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb`
(`git rev-list --left-right --count main...origin/main` → `0  0`)

## Document control

| Field | Value |
| --- | --- |
| Task | P7-02A Fedora 44 Packaging Foundation & Ro-ASD Readiness |
| Predecessor | `docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md` |
| Repository | Sentinel only (`sentinel-projects/sentinel`) |
| Host | macOS (Darwin, arm64), Qt 6.12.0, CMake 4.4.4, AppStream 1.2.1 — **not Fedora** |
| Ro-ASD access | **Not re-inspected in this task.** P7-01's read-only findings are the baseline. No Ro-ASD repository was cloned, read, or modified. |
| Commit/push/merge/release | None performed. All changes left uncommitted. |

## 0. Method and evidence limits

- Pre-flight checks, source inspection and the changes below were performed in the single existing
  repository directory. No clone, worktree or separate build tree was created.
- Fedora/RPM tooling (`rpmbuild`, `rpmspec`, `rpmlint`, `systemd-analyze`, `snapcraft`) is absent on
  the host, so **no RPM was built and no Fedora command was executed**. Spec and unit edits are
  static corrections, explicitly flagged as awaiting a Fedora run.
- Ro-ASD interfaces were taken from P7-01 rather than re-fetched. Everything Ro-ASD-specific in this
  document is therefore re-stated from P7-01 and is **not** new verification.
- No new Ro-ASD policy was invented. Everything that requires a Ro-ASD decision is recorded in §9
  and left open.

### 0.1 Workspace integrity incident (must be read before the file list)

While this task ran, **other concurrent sessions operated in the same repository directory** and
switched `HEAD` repeatedly under it. The reflog for `HEAD` (newest first) shows:

```
checkout: moving from feature/p4-04-resource-policy-adapter to refactor/p2-m03-native-provider-protocols
checkout: moving from feature/p7-02a-fedora-packaging-foundation to feature/p4-04-resource-policy-adapter
checkout: moving from feature/p6-03-desktop-accessibility to feature/p7-02a-fedora-packaging-foundation
checkout: moving from main to feature/p6-03-desktop-accessibility
```

Consequences and handling:

1. The working tree was clean at pre-flight (`git status --porcelain` empty) and is **not** clean now
   for reasons unrelated to this task: `core/CMakeLists.txt`, `tests/CMakeLists.txt`,
   `tests/helpers/settings_ui_certification_probe.cpp`, several `ui/qml/**` files and untracked
   `core/**/ResourcePolicyTelemetryAdapter.*`, `tests/core/test_resource_policy_adapter.cpp`,
   `tests/helpers/accessibility_certification.qml` belong to other in-flight sessions.
   **They were not modified or reverted by this task.**
2. `HEAD` was restored to `feature/p7-02a-fedora-packaging-foundation` (tree-neutral: every branch
   in this repository currently points at the same commit, so no file content changed). Restoring it
   is not durable — a concurrent session moved `HEAD` away again during this task, so the checked-out
   branch should be treated as unstable evidence and re-checked before review. Every branch here
   points at `cd9ec8bc`, so the working-tree content of the P7-02A changes is unaffected either way.
3. One of the other session's files did not compile during the first build attempt, which blocked the
   required preset build. The full suite was therefore run with that single unrelated fixture
   excluded (`-E test_resource_policy_adapter`); see §11.
4. Because uncommitted changes are not branch-bound, the P7-02A edits remain valid but are exposed to
   another session running `git stash`, `git clean`, or `git checkout -- .`. A copy of every changed
   file was placed in the session's approved temporary directory as a precaution. **A maintainer
   should serialise packaging tasks in one directory, or give each task its own worktree.**

### 0.2 Disk

`df -h .` → 228 GiB total, 110 GiB used, **93 GiB available (55 %)**. Existing build directories were
reused (`build/release`, `build/tests`); no new build tree and no duplicate release/debug build was
created. See §8.

---

## 1. Pre-flight verification

| Step | Result |
| --- | --- |
| Repository root | Confirmed (`git rev-parse --show-toplevel` = working directory) |
| `AGENTS.md` | Read; architecture/security/packaging boundaries honoured |
| `git fetch origin --prune` | Completed; no fetch errors |
| Local `main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| `origin/main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| Reconciled | Yes — ahead/behind `0 0` |
| Working tree | Clean at pre-flight (later contaminated by concurrent sessions, §0.1) |
| P7-01 audit | Read in full (827 lines) |
| Disk | 93 GiB available |
| Branch | `git switch -c feature/p7-02a-fedora-packaging-foundation main` — created from `main` |

No commit, push, merge, reset, stash, PR or workflow dispatch was performed.

---

## 2. Part A — Packaging audit (verified against P7-01)

Legend: **Confirmed** = re-verified in this task; **Corrected** = confirmed and fixed here;
**Refuted** = P7-01 finding does not hold; **Open** = confirmed but requires a decision.

| # | P7-01 finding | Status here | Evidence |
| --- | --- | --- | --- |
| A3 | Two near-identical RPM specs that differ only in `%changelog` | **Confirmed** | `diff` showed only the `%changelog` block differing |
| A3 | Spec/metainfo/snap pin `1.0.0` while CMake/Rust are `1.0.0-rc.8` | **Corrected** | `CMakeLists.txt:5`; `cli/Cargo.toml:6`; spec/metainfo/snap see §3 |
| A5 | Rust CLI/TUI has no RPM/deb/man and is outside CMake/release | **Confirmed** | `cli/` has no packaging inputs; `release.yml` never builds it |
| A8 | `packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml` is byte-identical to the base file | **Confirmed** | `diff` → identical (still identical after this task's edits) |
| A9 | systemd unit installed to hard-coded `${CMAKE_INSTALL_PREFIX}/lib/systemd/user` | **Refuted** | Fedora loads user units from `/usr/lib/systemd/user`; `${CMAKE_INSTALL_LIBDIR}` is `lib64` on Fedora, so the hard-coded `lib` is **correct**. Deliberately not changed. |
| A12 | systemd unit has no hardening directives | **Corrected** | `packaging/linux/systemd/sentinel-desktop.service` |
| A12 | Dual daemon start: desktop detached launch + unit `WantedBy=default.target` | **Open** | §6; ownership policy not adopted |
| A17 | `package-ready` builds only `sentinel-desktop` while packaging ships more | **Corrected** | `CMakePresets.json`; defect proved empirically, §4.1 |
| A17 | Spec `%files` omits installed artifacts | **Corrected** | plugin host, `share/sentinel`, `share/licenses/sentinel`, `share/doc/Sentinel`, §4.2 |
| A18 | D-Bus activation file installed but inert, with contradictory `Exec`/`SystemdService` | **Corrected** | No `QDBus`/`registerService` anywhere under `apps/`, `core/`, `plugins/`, `integrations/`; file removed, §5.3 |
| §3.2 | `bubblewrap` not declared for the Linux sandbox | **Corrected** | Spec `Recommends`/`BuildRequires` + CPack RPM recommends |
| §8 | No Fedora CI; all Linux CI is Ubuntu 22.04 | **Corrected (new workflow, unexecuted)** | `.github/workflows/ci-fedora.yml`, §7 |
| §3.1 | `libappstream-glib` used as the AppStream build dependency | **Corrected to the modern tool** | `appstream` + `appstreamcli`, §5.4 |
| — | `tools/analysis/check_personal_paths.py` | **New blocker found** | Fails on pristine `main`; §12.1 |

---

## 3. Part B — Version consistency

### 3.1 Canonical source is established by existing release policy

`CMakeLists.txt:3-5` states the identity is "Canonical human-controlled release identity", and
`cli/crates/sentinel-ipc/build.rs` **parses `set(SENTINEL_APP_VERSION "…")` out of
`CMakeLists.txt`** at build time. Two independent artifacts already derive from it. Therefore
`SENTINEL_APP_VERSION = 1.0.0-rc.8` is the canonical source and the packaging metadata at `1.0.0`
was the drifted side. No new version was invented and no release policy was changed.

### 3.2 RPM encoding of a pre-release version

RPM forbids `-` inside `Version` (it separates version and release). The pre-release marker is
therefore `~`, which also makes the pre-release sort **before** the final `1.0.0`:

| Artifact | Before | After |
| --- | --- | --- |
| `CMakeLists.txt` `SENTINEL_APP_VERSION` | `1.0.0-rc.8` | `1.0.0-rc.8` (unchanged, canonical) |
| `cli/Cargo.toml` `[workspace.package] version` | `1.0.0-rc.8` | `1.0.0-rc.8` (unchanged) |
| both RPM specs `Version:` | `1.0.0` | `1.0.0~rc.8` → NEVRA `…-1.fc44` |
| both AppStream `<release version>` | `1.0.0` | `1.0.0-rc.8` |
| `snapcraft.yaml` `version:` | `1.0.0` | `1.0.0-rc.8` |
| CPack `CPACK_PACKAGE_VERSION` | `1.0.0` (numeric prefix of the canonical value) | unchanged — derived, consistent |

`~` is accepted by Ro-ASD's `component-artifact-manifest-v1` artifact release pattern
`^[A-Za-z0-9+_.~]+\.fc44$`, so the encoding does not create a new Ro-ASD contract gap.

### 3.3 Drift cannot silently return

New `tools/packaging/check_packaging_consistency.py` fails if any of the above diverges from
`CMakeLists.txt`, if the duplicated packaging inputs diverge from each other, or if a spec ships a
binary the `package-ready` preset does not build. It is wired into the Linux quality job and into the
new Fedora metadata job. Deliberately **not** covered (and left as release-metadata work, §12.2):
`packaging/macos/appcast.xml`, `packaging/macos/Cask/sentinel.rb`,
`packaging/windows/winget/Sopwit.Sentinel.yaml` still reference `1.0.0` artifacts and release URLs;
they describe published store/channel metadata rather than build identity, and changing them would
require a release/publication decision.

---

## 4. Part C — RPM build parity

### 4.1 Verified defect: `package-ready` could not be installed at all

`CMakePresets.json`'s `package-ready` build preset declared `"targets": ["sentinel-desktop"]`, while
`apps/sentinel-daemon/CMakeLists.txt`, `apps/sentinel-cli/CMakeLists.txt` and
`apps/sentinel-plugin-host/CMakeLists.txt` each install their target **unconditionally**
(`install(TARGETS …)` with no `OPTIONAL`, no platform guard). `git archive`-independent proof on the
host, using a two-target reproduction with the same construction:

```
$ cmake --install <build> --prefix <stage>       # only target "a" built, "b" not built
CMake Error at cmake_install.cmake:51 (file):
  file INSTALL cannot find "<build>/b": No such file or directory.
install exit=1
```

Therefore `cmake --install build/package-ready --prefix dist` in `.github/workflows/release.yml`
fails on **Linux, macOS and Windows** after building only `sentinel-desktop`. The CPack
(DEB/RPM) and AppImage steps downstream of it can never be reached. Fix: the `package-ready` build
preset now builds every binary the packaging installs:

```json
"targets": ["sentinel-desktop", "sentinel-daemon", "sentinel-cli", "sentinel-plugin-host"]
```

This is the same parity requirement P7-01 raised ("Build daemon/cli in the package preset"),
extended to `sentinel-plugin-host`, which is also installed and is discovered at runtime by
`core/src/plugin/PluginHostSession.cpp` via `sentinel-plugin-host` on `PATH`.

### 4.2 Verified defect: the Fedora spec could not have passed `%files` inspection

`install()` destinations on Linux, enumerated from the CMake sources, versus the previous spec
`%files`:

| Installed path | Previous `%files` | Now |
| --- | --- | --- |
| `/usr/bin/sentinel-desktop`, `sentinel-daemon`, `sentinel-cli` | present | present |
| `/usr/bin/sentinel-plugin-host` | **missing** | added |
| `/usr/share/sentinel/core-warden/**` (animation PNGs, `manifest.json`, `state-contract.json`) | **missing** | added (`%{_datadir}/sentinel`) |
| `/usr/share/licenses/sentinel/**` (LICENSE, `THIRD_PARTY_NOTICES.md`, tabler, lucide, fonts) | **missing** | added (`%{_datadir}/licenses/sentinel`) |
| `/usr/share/doc/Sentinel/{LICENSE,README.md}` | **missing** | added (`%{_datadir}/doc/Sentinel`) |

`CMAKE_INSTALL_DOCDIR` was confirmed to expand to `share/doc/Sentinel` for `project(Sentinel)` with
`GNUInstallDirs`. With Fedora's default `%_unpackaged_files_terminate_build 1`, the previous file
list would have failed the build. No functionality was removed to make the package pass.

### 4.3 Unchanged, deliberately

- **Two specs remain.** They are now **byte-identical** and a checker fails on any divergence.
  Collapsing them into one path (and choosing whether the canonical spec eventually lives in a
  Ro-ASD-owned `sentinel-packaging` repository, per P7-01 §4 rule 2 and `PD-D06`) is a decision, not
  a defect fix — left open (§9).
- **`-DSENTINEL_BUILD_NUMBER="%{release}"`** in the specs sets the build number to e.g. `1.fc44`
  instead of the `rc.8 → 8` derivation. Low impact (display only on Linux; the numeric field matters
  only to the Windows `.rc` template). Left as-is and recorded in §12.
- **Rust packaging scope.** `cli/` still has no RPM, man page or static build; the shipped Rust
  binary is `sentinel` (from `crates/sentinel-cli`) and would collide conceptually with the C++
  `sentinel-cli`. Package splitting is decision D3/D9 and is left open (§9). No Rust artifact was
  added or removed.
- **Installation prefixes.** `%{_bindir}`/`%{_datadir}`/`%{_sysconfdir}` are unchanged and
  Fedora-correct; the systemd user-unit prefix (`<prefix>/lib/systemd/user`) is unchanged because it
  is correct on Fedora (§2 A9).
- **Package removal behaviour.** No `%preun`/`%postun` logic deletes user data; user-owned
  settings/SQLite/models live under `QStandardPaths` homes, not package paths. No change was needed.
  `/etc/sentinel/config.json.template` stays `%config(noreplace)`.

---

## 5. Part D — Fedora desktop integration

### 5.1 systemd user unit hardening (corrected)

`packaging/linux/systemd/sentinel-desktop.service` now sets `NoNewPrivileges`, `PrivateTmp`,
`PrivateDevices`, `ProtectSystem=full`, `ProtectKernelTunables`, `ProtectKernelModules`,
`ProtectControlGroups`, `ProtectClock`, `ProtectHostname`, `RestrictSUIDSGID`, `RestrictRealtime`,
`RestrictAddressFamilies=AF_UNIX AF_INET AF_INET6 AF_NETLINK`, `SystemCallArchitectures=native` and
`LockPersonality`. `AF_NETLINK` is included because the daemon resolves names
(`core/src/proxy/ProxyResolver.cpp`) and Qt/glibc may use netlink for address configuration.

Three things are **deliberately not** restricted, documented inline in the unit:

- `ProtectHome` / `ProtectSystem=strict` — the daemon owns its socket (`~/.sentinel/run`), profile
  directory and user data under the session user's home; P7-01's suggested `ProtectSystem=strict`
  would have made those paths read-only and broken the daemon.
- `RestrictNamespaces` — approved tool execution sandboxes programs with bubblewrap user
  namespaces; blocking namespaces at unit level would break it.

Still a **user** unit; no system service, no `root`, no capabilities, no privileged execution.

### 5.2 Duplicate daemon activation (open — policy not adopted)

Unchanged by design: the desktop still launches the daemon detached
(`ApplicationBootstrapper::ensureBackgroundDaemon`, `ApplicationBootstrapper.cpp:82-104`, suppressed
by `--safe-mode`/`--no-daemon-start`) and the unit still has `WantedBy=default.target`. The socket
lock plus stale-socket probe still prevents a competing listener, so there is still exactly one
process owner. Resolving which path is *the* automatic owner is product/ownership policy.

Also deliberate: the spec still does **not** call `%systemd_user_post`/`%systemd_user_preun`. Fedora's
macros can apply `systemctl --user preset`, which risks enabling the unit at login — the exact
"automatic login startup" this task forbids enabling. Adding them must wait for the ownership
decision, and the omission is now visible rather than accidental.

### 5.3 D-Bus metadata (corrected by removing an unfulfillable declaration)

`packaging/linux/dbus/dev.sentinel.Sentinel.service` declared D-Bus activation for
`dev.sentinel.Sentinel`, but:

- no code registers on the session bus — repo-wide search for `QDBus`, `QDBusConnection`,
  `registerService`, `registerObject`, `SESSION_BUS` and `org.freedesktop.DBus` under `apps/`,
  `core/`, `plugins/` and `integrations/` returns **no matches**;
- the two fields contradicted each other: `Exec=/usr/bin/sentinel-desktop` (GUI) while
  `SystemdService=sentinel-desktop.service` starts `sentinel-daemon`, which never claims the name.

The file, its `install()` rule in `apps/sentinel-desktop/CMakeLists.txt` and its `%files` entry were
removed. This removes **no behaviour** (activation could never succeed) and eliminates a false
contract that would otherwise be shipped into a Ro-Repo snapshot. `.desktop` `DBusActivatable=true`
was deliberately **not** added — it would instruct the desktop environment to activate over D-Bus and
fail. If a D-Bus contract is wanted, session-bus registration must be implemented first (§12.2).

### 5.4 AppStream / desktop metadata

- `appstreamcli validate --no-net` on the edited base metainfo: **pass**, with the same two
  informational findings as P7-01 (`developer-name-tag-deprecated`, `developer-info-missing`).
  Removing the `<release date>` attribute was tested and **fails** (`release-time-missing`), so the
  release version was changed while the pre-existing date `2026-07-30` was kept verbatim — no release
  date was invented. The stale "D-Bus session activation" sentence in the release notes was corrected.
- `<developer id=…>` / screenshots were **not** added: they require a developer identity and real
  screenshots. Left as a decision (§12.2), not invented.
- `.desktop` entry is unchanged (`Exec=sentinel-desktop %U`, `Icon=dev.sentinel.Sentinel`); it parses
  and its `Exec` binary is shipped by the spec (now enforced by the checker).

### 5.5 XDG paths, bubblewrap, lifecycle

- XDG/config/socket paths are unchanged. The non-XDG socket (`~/.sentinel/run/daemon.sock`) is a
  known, documented boundary; changing it touches the C++ daemon, the desktop client and the Rust IPC
  crate together and is out of scope for a packaging-foundation task.
- `bubblewrap` is now declared: `Recommends: bubblewrap` plus `BuildRequires: bubblewrap` (the spec
  `%check` runs the full CTest suite, whose Linux sandbox paths need the backend) and
  `CPACK_RPM_PACKAGE_RECOMMENDS` += `bubblewrap`. `Recommends` rather than `Requires` because the
  application runs without it and only denies sandboxed *tool execution* — the fail-closed behaviour
  described in `docs/security/SANDBOX.md`.
- AppStream build dependency moved from `libappstream-glib` (legacy `appstream-util`) to `appstream`
  (`appstreamcli`, the modern Fedora tool), with `%check` preferring `appstreamcli` and falling back
  to `appstream-util` when present. **Not verified on Fedora** (§11).

---

## 6. Daemon startup findings (summary)

| Path | Behaviour today | P7-02A treatment |
| --- | --- | --- |
| Desktop detached launch (`ensureBackgroundDaemon`) | default starter unless `--safe-mode`/`--no-daemon-start` | unchanged |
| systemd user unit | installed, not enabled by the package, `WantedBy=default.target` if a user enables it | hardened; still not auto-enabled; `%systemd_user_post` deliberately omitted |
| Quick-Panel autostart | written only on explicit user opt-in (`NativeCompanionAdapter`) | unchanged |
| D-Bus activation | inert and contradictory | removed |

No privileged service, no broad filesystem access, no automatic login startup and no destructive
migration was introduced. The "one owner" criterion still holds through the daemon's socket lock.

---

## 7. Part E — Fedora CI

New `.github/workflows/ci-fedora.yml` (Fedora 44 container, `registry.fedoraproject.org/fedora:44`),
independent of Ro-ASD. Existing Ubuntu/macOS/Windows/Rust CI is preserved.

| Job | Validates |
| --- | --- |
| `packaging-metadata` | packaging metadata consistency; `desktop-file-validate`; `appstreamcli validate --no-net`; `rpmspec -P` parse of both specs; SRPM build via `packaging/linux/fedora-kde/build_copr.sh`; *advisory:* `rpmlint`, `systemd-analyze verify` |
| `build-package` | distribution Qt 6 deps + CMake configure, `package-ready` build, `cmake --install` staging, and an explicit layout/parity assertion that all four shipped binaries and every installed metadata path exist |
| `test` | `tests` preset build on Fedora; *advisory:* `ctest` (container user-namespace availability may affect bubblewrap-backed tests) |
| `rust-feasibility` | *advisory:* Fedora toolchain version versus `cli/Cargo.toml` `rust-version = 1.94`, and `cargo metadata --locked` resolution |

Honesty limits recorded in the workflow header itself: the workflow has **never executed**, so jobs
that depend on container capabilities are marked `continue-on-error` until a maintainer confirms a
green run; and a green Fedora container run is **not** evidence of Ro-ASD image compatibility. The
image is tag-pinned, not digest-pinned — Ro-ASD's `build.builder_image` digest requirement is a
Ro-ASD-side decision (§9) and is not hard-coded here. Nothing is published, no signing key is
configured and no workflow is dispatched.

`qmllint`, `clang-format` and `clang-tidy` are intentionally left to the Ubuntu quality job.

---

## 8. Part G — Disk constraints

- Reused `build/release` and `build/tests` for configure/build evidence; no new build tree.
- One incremental preset build + one `ctest` run; the two-target CMake install reproduction and the
  AppStream experiments were run in the approved temporary directory, not in the repository.
- No ccache, worktree, cache or project file was deleted.

---

## 9. Part F — Ro-ASD boundary: open decisions and blockers

No Ro-ASD identity, repository, builder digest, package split or architecture was hard-coded.
Ro-ASD's independently maintained responsibilities are untouched: package-producer onboarding,
Ro-Repo acceptance/signing, image composition, release channels and final preinstallation policy.

Carried forward from P7-01 (unchanged, still blocking), with P7-02A impact:

| ID | Decision needed | P7-02A impact |
| --- | --- | --- |
| D1 | Where the producer/manifest lives — a `Project-Ro-ASD/*` packaging repo, or a schema relaxation for `source.repository` | Blocks creating `.roasd/app.json`; Sentinel's own repo must not be renamed |
| D2 | Digest-pinned Fedora 44 builder: component-owned build vs Ro-Repo build | The new CI deliberately uses an unpinned tag; Ro-ASD's digest pin is not adopted |
| D3 | Package names and split (one `sentinel-desktop` vs split packages) | The spec still ships one package with four binaries; split remains open |
| D4 | aarch64 requirement (schema has no `aarch64` artifact arch) | `package-ready`/spec remain x86_64-neutral |
| D5 | `risk_class` and required test set | Not encoded |
| D6 | Channels (beta only vs stable) | Not encoded |
| D7 | Default preinstall in `desktop-standard`/profile | Not encoded |
| D8 | Onboarding/OOBE handoff | Untouched |
| D9 | CLI/TUI scope and the Rust `sentinel` vs C++ `sentinel-cli` naming | Rust packaging left out; §4.3 |
| D10 | SELinux policy expectation | No SELinux artifacts added; unmeasured |
| D11 | Support/ownership for cross-repo triage and rollback | Unchanged |

Additional Sentinel-internal decisions raised by P7-02A (not Ro-ASD's to make):

| ID | Decision | Why it is open |
| --- | --- | --- |
| S1 | Collapse the two identical RPM specs into one, and where the surviving spec lives | May move to a Ro-ASD-owned `sentinel-packaging` repository (PD-D06) |
| S2 | Daemon ownership: desktop detached launch vs systemd user unit as the single automatic starter | Product/ownership policy; decides §5.2 and whether `%systemd_user_post` is added |
| S3 | Whether a D-Bus activation contract is wanted at all | Requires implementing session-bus registration first |
| S4 | AppStream release history (a real date/notes per RC) and `<developer id>` | Requires release data and developer identity |
| S5 | Cross-platform release metadata (`appcast.xml`, Cask, Winget) version policy | Publication decision, not build identity |

---

## 10. Exact changed files

Verified `main` = `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb`; branch
`feature/p7-02a-fedora-packaging-foundation`. All changes are **uncommitted**.

```
 .github/workflows/ci-fedora.yml                                  | new
 .github/workflows/ci-linux.yml                                   |   2 ++
 CMakePresets.json                                                |   5 +-
 apps/sentinel-desktop/CMakeLists.txt                             |   4 ---
 cmake/SentinelCPack.cmake                                        |   2 +-
 packaging/linux/dbus/dev.sentinel.Sentinel.service               |   4 ---  (deleted)
 packaging/linux/dev.sentinel.Sentinel.metainfo.xml               |   4 +--
 packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml    |   4 +--
 packaging/linux/fedora-kde/sentinel-desktop.spec                 |  34 ++++++--
 packaging/linux/rpm/sentinel-desktop.spec                        |  37 ++++++--
 packaging/linux/snap/snapcraft.yaml                              |   2 +-
 packaging/linux/systemd/sentinel-desktop.service                 |  23 +++++
 tools/packaging/check_packaging_consistency.py                   | new
 docs/audits/P7_02A_FEDORA_PACKAGING_FOUNDATION_RESULT.md         | new (this file)
```

Per-file intent:

1. `.github/workflows/ci-fedora.yml` — new Fedora 44 packaging/build/test workflow (§7).
2. `.github/workflows/ci-linux.yml` — one added step running the packaging metadata checker.
3. `CMakePresets.json` — `package-ready` build preset now builds every packaged binary (§4.1).
4. `apps/sentinel-desktop/CMakeLists.txt` — removed the inert D-Bus `install()` rule (§5.3).
5. `cmake/SentinelCPack.cmake` — RPM recommends `bubblewrap`.
6. `packaging/linux/dev.sentinel.Sentinel.metainfo.xml` — canonical release version; stale D-Bus
   claim removed from release notes.
7. `packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml` — kept byte-identical to (6).
8. `packaging/linux/fedora-kde/sentinel-desktop.spec` and 9. `packaging/linux/rpm/sentinel-desktop.spec`
   — canonical `Version: 1.0.0~rc.8`; `appstream` + `bubblewrap` build/runtime deps; `appstreamcli`-first
   `%check`; `%files` completed (plugin host, core-warden, licenses, docs) and D-Bus entry dropped;
   merged, byte-identical changelog with a new entry.
10. `packaging/linux/snap/snapcraft.yaml` — canonical version.
11. `packaging/linux/systemd/sentinel-desktop.service` — conservative hardening (§5.1).
12. `packaging/linux/dbus/dev.sentinel.Sentinel.service` — deleted (§5.3).
13. `tools/packaging/check_packaging_consistency.py` — new drift/parity checker.
14. `docs/audits/P7_02A_FEDORA_PACKAGING_FOUNDATION_RESULT.md` — this result.

**Not changed by this task** (present in the working tree from other sessions, §0.1):
`core/CMakeLists.txt`, `tests/CMakeLists.txt`, `tests/helpers/settings_ui_certification_probe.cpp`,
`ui/qml/**`, `core/include/sentinel/core/policy/ResourcePolicyTelemetryAdapter.h`,
`core/src/policy/ResourcePolicyTelemetryAdapter.cpp`, `tests/core/test_resource_policy_adapter.cpp`,
`tests/helpers/accessibility_certification.qml`.

---

## 11. Validation executed

Executed on the host (macOS arm64, Qt 6.12.0, CMake 4.4.4, AppStream 1.2.1):

| Check | Command | Result |
| --- | --- | --- |
| Canonical-base comparison | `git rev-list --left-right --count main...origin/main` | `0  0` |
| CMake configure (reused `build/release`) | `cmake --preset release` | **pass** (`Configuring done`, `Generating done`) |
| CMake configure (reused `build/tests`) | `cmake --preset tests` | **pass** |
| Preset build | `cmake --build --preset tests` | **pass** (after the other session's fixture was fixed/unblocked) |
| Tests | `ctest --preset tests -E test_resource_policy_adapter --output-on-failure` | **pass — 148/148**, 176.9 s |
| Packaging metadata + version/parity | `python3 tools/packaging/check_packaging_consistency.py` | **pass** — canonical `1.0.0-rc.8`, RPM `1.0.0~rc.8`, preset binaries `sentinel-cli, sentinel-daemon, sentinel-desktop, sentinel-plugin-host` |
| Checker non-vacuity | mutated copies (spec version, snap version, spec drift, preset target removal) in a temporary tree | **fails as designed** on all four mutations |
| AppStream | `appstreamcli validate --no-net packaging/linux/dev.sentinel.Sentinel.metainfo.xml` | **pass** — infos: `developer-name-tag-deprecated`, `developer-info-missing` (unchanged from P7-01) |
| AppStream negative control | release entry without `date` | **fails** `release-time-missing` (why the date was retained) |
| `install()` parity defect proof | two-target CMake install reproduction in the temporary directory | **error reproduced** — `file INSTALL cannot find …` |
| `CMAKE_INSTALL_DOCDIR` resolution | minimal `project(Sentinel)` + `GNUInstallDirs` | `share/doc/Sentinel` (confirms the spec path) |
| JSON | `CMakePresets.json` parsed | **valid** |
| YAML | `ci-fedora.yml`, `ci-linux.yml`, `snapcraft.yaml` parsed | **valid** |
| Desktop entry / unit INI | `configparser` parse | **valid**; `Exec=sentinel-desktop %U`; unit sections `Unit/Service/Install` |
| Whitespace | `git diff --check` on the changed paths | **clean** |

Test-evidence caveat: the 148/148 run executed in a working tree that also contained **another
session's uncommitted `core/` changes** and excluded its in-progress fixture. It is strong evidence
that this task's changes break nothing, but it is **not** a clean-branch certification.

---

## 12. Missing native validation (explicitly not claimed)

1. **No Fedora execution.** `rpmbuild`, `rpmspec`, `rpmlint` and `systemd-analyze` are absent, so the
   spec edits (version encoding, `%files` additions, `appstream`/`bubblewrap` dependencies,
   `appstreamcli`-first `%check`) and the systemd unit are **static only**. The first Fedora run must
   confirm them.
2. **No Fedora CI run.** `.github/workflows/ci-fedora.yml` was written but never dispatched; container
   tags, package names (`qt6-*`, `mesa-libGL-devel`, `vulkan-loader-devel`, `google-noto-sans-fonts`),
   `rpmlint` severity handling, `systemd-analyze verify` dependency resolution and container
   user-namespace availability for bubblewrap are all unverified.
3. **No Ro-ASD image** (`ENV-ROASD` unavailable) — no package/service identity, lifecycle or
   acceptance evidence; SELinux behaviour remains unmeasured.
4. **No bubblewrap execution path here** — the Linux sandbox branch of `ProcessSandbox` cannot run on
   macOS; only its fail-closed structure was inspected.
5. **No RPM/DEB/AppImage/Snap/Flatpak artifact was produced**, and no package signing exists on
   Linux (unchanged from P7-01).
6. **Rust packaging feasibility is a probe only** — nothing was compiled on Fedora and no Rust RPM
   exists.
7. **Contaminated evidence** from concurrent sessions in the shared directory (§0.1).

### 12.1 Pre-existing CI blocker found (not caused by this task)

`tools/analysis/check_personal_paths.py` **fails on pristine `main`** because
`docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md` records an absolute worktree path in
its document-control table. That is the second step of the Linux `quality` job, so main's Linux CI
cannot be green, and the new packaging-consistency step placed after it will not execute until it is
fixed. The file was deliberately **not** edited here (it is another task's committed audit artifact
and the path is intentional evidence). Recommended: sanitize that one line, then confirm the quality
job is green.

---

## 13. Recommended P7-02B scope

Ordered, all Sentinel-owned unless noted:

1. **Make main's Linux CI green** by sanitizing the personal path in the P7-01 audit (§12.1), then
   confirm the quality job reaches the new packaging-consistency step.
2. **Run the new Fedora workflow once**, fix any package-name/severity fallout, remove the
   `continue-on-error` advisory markers, and confirm `rpmbuild -ba` (binary RPM) succeeds so the
   completed `%files` list is proven rather than argued.
3. **Add RPM binary-build verification** to the Fedora job (`rpmbuild -ba` once ctest/ccache budget
   allows) including `%files` unpackaged-file detection, and align the spec's `%doc`/`%license` with
   the CMake-installed `share/doc/Sentinel` and `share/licenses/sentinel` paths (S4-adjacent cleanup).
4. **Collapse the two identical RPM specs into one** (S1) once the eventual home is decided with
   Ro-ASD (`sentinel-packaging` vs this repository).
5. **Decide daemon ownership** (S2) and, depending on the outcome, either give the unit a documented
   `%systemd_user_post`/preset policy or make the detached desktop launch the sole automatic starter.
6. **Rust packaging decision** (D9): RPM + man page + static or dynamically linked build for
   `sentinel-cli`/`sentinel-tui`, with an explicit resolution of the `sentinel` vs `sentinel-cli`
   naming collision.
7. **Raise `-DSENTINEL_BUILD_NUMBER` correctness** in the specs (`1.fc44` vs the derived `8`).
8. **Session-bus registration decision** (S3): implement `QDBusConnection::sessionBus().registerService`
   and restore a corrected activation file, or record D-Bus activation as permanently withdrawn.
9. **AppStream history and developer identity** (S4): real RC dates/notes, `<developer id>`,
   screenshots.
10. **Ro-ASD-owned work, not to be started here:** `.roasd/app.json`, `tools/build-rpm` and
    `tools/project-ci` in a `Project-Ro-ASD/*` repository, digest-pinned Fedora 44 builder,
    `component-artifact-manifest-v1` emission, `ro-app-gate`, producer onboarding, channel selection
    and compose preinstallation — all gated on D1–D11.

---

## 14. Summary of readiness

- **Build parity is now real, not assumed.** The verified defect that made `cmake --install` (and
  therefore CPack, DEB/RPM and AppImage) impossible on every platform is fixed, and the fix is
  guarded by an automated checker that also prevents version drift and spec/`%files` binary drift.
- **Version consistency is resolved** against the canonical `SENTINEL_APP_VERSION` that CMake and the
  Rust IPC crate already derive from, using RPM-correct pre-release encoding.
- **Fedora desktop integration is safer and more honest:** hardened user unit within the daemon's real
  privilege and path envelope, `bubblewrap` declared, legacy AppStream tooling replaced, and an inert
  D-Bus contract removed instead of shipped.
- **Fedora CI now exists** as a prepared, self-documenting, non-publishing workflow; its strictness
  must be confirmed on a real Fedora 44 run.
- **Still blocked by missing native evidence and Ro-ASD decisions:** no Fedora execution, no Ro-ASD
  image (`ENV-ROASD` unavailable), no Linux signing, no SELinux characterisation, and D1–D11 open.
  Sentinel remains independently installable on generic Fedora; nothing Ro-ASD-specific was assumed.
