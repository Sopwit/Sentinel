# P7-01 — Ro-ASD Fedora Integration & Preinstallation Readiness

Status: **Audit complete — integration design proposed, implementation not started.**
Classification: Packaging and distribution integration audit (read-only across repositories).

## Document control

| Field | Value |
| --- | --- |
| Task | P7-01 Ro-ASD Fedora Integration & Preinstallation Readiness |
| Phase | P7 — Linux Distribution Integration |
| Date | 2026-10-17 |
| Sentinel repository | https://github.com/sentinel-projects/sentinel |
| Audit branch | `audit/p7-01-roasd-integration` |
| Audit HEAD | `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` (`6035bf74cbb3`) |
| Base | `origin/main` @ `6035bf74cbb3f2556e29fe1bf31163a56eb5b591` (verified; `git merge-base HEAD origin/main` = HEAD) |
| Worktree | `/Users/emir/.local/share/opencode/worktree/d272e3/audit/p7-01-roasd-integration` |
| Ro-ASD project | https://github.com/Project-Ro-ASD (inspected read-only) |
| Host used for checks | macOS (Darwin), Qt 6.12.0, CMake 4.4.4, Python 3.15 — no Fedora host available |

No commit, push, merge, PR, workflow dispatch, or external repository modification was performed.
The Ro-ASD organization was inspected read-only through the public GitHub API and raw file endpoints.

---

## 0. Scope, method, and evidence limitations

### 0.1 Method

- Sentinal-side evidence was read directly from the audit worktree at
  `6035bf74cbb3` (paths and line numbers below are relative to the worktree root).
- Ro-ASD-side evidence was read read-only from public GitHub repositories via the GitHub REST API
  and `raw.githubusercontent.com` on 2026-10-17. No Ro-ASD file was written.
- Only static/CI/packaging evidence was collected. No Fedora host, no Ro-ASD image, no RPM build,
  no `dnf`, no system-package installation, and no Ro-ASD image build was performed.
- Static checks that were executable on the audit host were run (see §F / Appendix C).

### 0.2 Evidence limitations (must not be read as validation)

1. **No `ENV-ROASD` environment exists.** `docs/roadmaps/ACCEPTANCE_MATRIX.md:63` records
   `ENV-ROASD = "Unavailable: image/version/owner/repository access undecided"`. All native
   lifecycle, tray, Wayland/KDE and SELinux outcomes are therefore **pending**, not verified.
2. **No Fedora execution evidence.** Sentinel CI runs on `ubuntu-22.04`
   (`.github/workflows/ci-linux.yml:18`). Fedora/RPM behavior is asserted by packaging sources,
   not by a Fedora run. `docs/development/SEC_P1_3_DISTRIBUTION_COMPLIANCE_REPORT.md:98` marks the
   Linux desktop binary **BLOCKED / NOT VERIFIED natively**.
3. **Ro-ASD is in development.** `Ro-ASD-release/VERSION.yaml` sets `phase: development`,
   `product_release: 44`, `fedora_release: 44`, `component_version: 0.1.2`. Interfaces inspected
   today may change.
4. **`Ro-image-compose` is not publicly accessible.** It is named as the owner of image/package
   composition in `.github/docs/fedora-44-release-compose-baseline-tr.md`, but it does not appear in
   the org's public repository list. Its manifest format is therefore **unverified**.
5. **No repository access or credentials** for Ro-ASD package acceptance, signing, channel
   publication, or the Ro-ASD image were available, consistent with `PD-D06` still being
   *Pending Decision* (`docs/roadmaps/ACCEPTANCE_DECISIONS.md`, "PD-D06 — Ro-ASD packaging and support").
6. **No RPM tooling on the audit host** (`rpmlint`, `rpmbuild`, `rpmspec`, `desktop-file-validate`
   absent). RPM spec linting and desktop-file validation are recorded as pending, except for
   AppStream validation which was available (`appstreamcli`).

### 0.3 Products of this task

This document itself. No code, packaging, CI, or external change was produced, per the isolation
rules. Any follow-up change belongs to **P7-02** (§10).

---

## 1. Verified Sentinel Linux capabilities (Part A)

Legend: **Implemented** (code/assets exist and are exercised somewhere), **Partial** (assets exist
but are incomplete, unwired, or unexercised), **Absent**.

| # | Area | Status | Primary evidence |
| --- | --- | --- | --- |
| A1 | Fedora build compatibility | Partial | Qt 6.5+ requirement satisfied by Fedora 44 (Qt 6.10/6.11); no Fedora CI |
| A2 | Qt 6 / QML dependencies | Implemented | `CMakeLists.txt:60,63`; Fedora `qt6-*` requires in spec |
| A3 | RPM packaging status | Partial | 2 specs + COPR script exist; not CI-wired; version drift; not signed |
| A4 | C++ runtime dependencies | Partial | Uses system Qt + optional helpers; no DT_NEEDED audit |
| A5 | Rust CLI/TUI packaging | Absent | `cli/` builds + CI-tests only; no RPM/deb/man; not in CMake/release |
| A6 | Executable paths | Implemented | `%{_bindir}` via GNUInstallDirs |
| A7 | Desktop entry | Implemented | `.desktop` installed; valid except no `DBusActivatable` |
| A8 | Icon / AppStream metadata | Partial | Valid but minimal; no screenshots; version drift |
| A9 | Installation prefixes | Implemented | GNUInstallDirs; one hard-coded `lib/systemd/user` |
| A10 | XDG config/data | Partial | `QStandardPaths` for data/config; socket not XDG (`~/.sentinel`) |
| A11 | Daemon startup/shutdown | Implemented | `DaemonService`, SIGINT/SIGTERM, drain, socket lock |
| A12 | Systemd user-service integration | Partial | Unit shipped; **dual-start** with desktop auto-launch; no hardening |
| A13 | Tray / KDE Plasma behavior | Partial | Qt `QSystemTrayIcon` only; no StatusNotifier/KDE-specific code |
| A14 | Wayland/X11 compatibility | Partial | Delegated to Qt; no explicit handling; unverified on Wayland |
| A15 | SELinux implications | Absent | No `.te`, `restorecon`, `fcontext`, or policy anywhere |
| A16 | polkit requirements | Absent | No polkit action/policy files |
| A17 | Release artifact generation | Partial | DEB/RPM/AppImage/tar on tag; draft-only; unsigned; Rust excluded |
| A18 | D-Bus activation | Partial/declared | Service file installed; **no bus registration**; Exec/SystemdService mismatch |

### A1. Fedora build compatibility

- Sentinel requires `find_package(Qt6 6.5 REQUIRED COMPONENTS Core Gui Quick Qml Sql Test Network
  Widgets LinguistTools Multimedia Concurrent OPTIONAL_COMPONENTS Pdf)` (`CMakeLists.txt:60`) and
  `qt_standard_project_setup(REQUIRES 6.5)` (`:63`). C++20 (`:51-53`), CMake ≥ 3.24 (`:1`).
- Fedora 44 (the Ro-ASD base) ships `qt6-qtbase 6.10.2`/updates `6.11.2`, and Fedora 45/Rawhide
  carry 6.11.2. The minimum Qt 6.5 requirement is therefore satisfied by Fedora 44 **without**
  vendored Qt. This is the strongest positive Fedora compatibility signal.
- CI never runs on Fedora: `.github/workflows/ci-linux.yml:18` uses `ubuntu-22.04` with Qt from
  `jurplel/install-qt-action` `6.11.*` (`:42-45`). There is **no Fedora/RPM build job**.
- `docs/development/BUILDING.md:32` states "Fedora KDE Plasma is the primary desktop target; do not
  put Linux-only assumptions in core code."

### A2. Qt 6 / QML dependencies

Implemented. Required modules are declared (`CMakeLists.txt:60`). The Fedora spec mirrors them:
`BuildRequires: qt6-qtbase-devel, qt6-qtdeclarative-devel, qt6-qtsql-devel, qt6-qtmultimedia-devel,
qt6-qttools-devel`; `Requires: qt6-qtbase, qt6-qtdeclarative, qt6-qtsql, qt6-qtmultimedia`
(`packaging/linux/fedora-kde/sentinel-desktop.spec:9-21`). CPack carries an equivalent RPM requires
string (`cmake/SentinelCPack.cmake:55`).

### A3. RPM packaging status — Partial

Present but not production-ready:

- Two duplicate specs: `packaging/linux/rpm/sentinel-desktop.spec` and
  `packaging/linux/fedora-kde/sentinel-desktop.spec` (near-identical; changelog differs).
- COPR helper `packaging/linux/fedora-kde/build_copr.sh` builds a tarball/SRPM and submits only with
  explicit `--submit` (`:57-73`). **No workflow invokes it.**
- Spec ships `%{_bindir}/sentinel-desktop`, `sentinel-daemon`, `sentinel-cli`
  (`packaging/linux/fedora-kde/sentinel-desktop.spec:87-89`), the desktop file, hicolor icons, and
  metainfo — **but not the Rust `sentinel` binary**.
- **Version drift:** spec/metainfo/snap hard-code `1.0.0` while CMake and the Rust workspace are
  `1.0.0-rc.8` (`CMakeLists.txt:5`, `cli/Cargo.toml:6`). A tagged release today would produce a
  metadata/package version mismatch.
- **No Linux package signing** exists (`cmake/SentinelSigning.cmake` has only WIN32/APPLE branches;
  `cmake/SentinelSignPackages.cmake` signs only Windows installers). See also §6.

### A4. C++ runtime dependencies — Partial

Runtime library dependencies come from the system Qt packages plus optional helpers recommended by
the spec (`libnotify, xdg-utils, libsecret, avahi-tools`, `:24-27`). No `DT_NEEDED`/`ldd`
inventory exists in-tree, and no bundled third-party shared libraries are installed. `Sentinel_core`
is a static library and is not installed (`core/CMakeLists.txt:7`); plugin SDK/sample plugins are
not installed either — this is fine for a monolith but means external plugin packaging has no
installed SDK surface.

### A5. Rust CLI/TUI packaging — Absent

- Workspace `cli/Cargo.toml:2` members: `sentinel-ipc, sentinel-cli, sentinel-tui, sentinel-mascot`;
  `version = "1.0.0-rc.8"`, `rust-version = "1.94"`, `edition = 2024`.
- The shipped binary is named **`sentinel`** (`crates/sentinel-cli/Cargo.toml:17-18`), distinct from
  the C++ `sentinel-cli` installed by CMake — two terminal front-ends coexist.
- No RPM/deb/man pages/musl-static build anywhere under `cli/`; the only installer is a dev script
  `tools/terminal/install-dev.sh:5` (`cargo install --path .../sentinel-cli --locked`).
- Not referenced by CMake or `release.yml`; `docs/release/RELEASE_PROCESS.md:7` confirms the Desktop
  workflow "does not build or clear the Rust package".
- `cli/Cargo.lock` contains **139 `[[package]]` entries**.

### A6. Executable paths — Implemented

`include(GNUInstallDirs)` and `install(TARGETS ... RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})` for
`sentinel-desktop` (`apps/sentinel-desktop/CMakeLists.txt:402,404-413`), `sentinel-cli`
(`apps/sentinel-cli/CMakeLists.txt:24-33`), `sentinel-daemon`
(`apps/sentinel-daemon/CMakeLists.txt:44-53`), and `sentinel-plugin-host`. All land in `/usr/bin`.

### A7. Desktop entry — Implemented

`packaging/linux/dev.sentinel.Sentinel.desktop`, installed by CMake
(`apps/sentinel-desktop/CMakeLists.txt:457-459`).
`Exec=sentinel-desktop %U`, `Icon=dev.sentinel.Sentinel`, `Type=Application`,
`StartupWMClass=dev.sentinel.Sentinel`, `MimeType=x-scheme-handler/sentinel;`, `Categories=Utility;`.
The `sentinel://` scheme is actually handled in-process
(`core/src/platform/WinProtocolHandler.cpp:130`, `SingleInstanceGuard.cpp:28`).
**Gap:** no `DBusActivatable=true`, and no `X-KDE-*` keys.

### A8. Icon and AppStream metadata — Partial

- Icons live canonically in `resources/branding/app-icon/linux/{16,22,24,32,48,64,128,256,512}x*/apps/sentinel.png`
  and `linux/scalable/apps/sentinel.svg`; CMake installs all nine PNG sizes + SVG into hicolor
  (`apps/sentinel-desktop/CMakeLists.txt:461-470`). Tray assets exist under
  `resources/branding/tray/linux/`. **No 1024×1024 icon** exists despite a stale CPack exclude entry
  (`cmake/SentinelCPack.cmake:62`).
- `packaging/linux/dev.sentinel.Sentinel.metainfo.xml`: id `dev.sentinel.Sentinel`, launchable
  `dev.sentinel.Sentinel.desktop`, single `<releases>` entry `1.0.0` (2026-07-30), `oars-1.1`,
  `<provides><binary>sentinel-desktop</binary>`. Missing screenshots, `<developer id=...>`,
  `<branding>`, `<translation>`.
- `packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml` is **byte-identical** to the base
  file — no Fedora-specific customization.
- Validation run on the audit host (see Appendix C): `appstreamcli validate --no-net` passes with two
  informational findings (`developer-name-tag-deprecated`, `developer-info-missing`).

### A9. Installation prefixes — Implemented

Standard GNUInstallDirs prefixes: binaries `/usr/bin`, desktop
`/usr/share/applications`, icons `/usr/share/icons/hicolor/...`, metainfo `/usr/share/metainfo`,
docs `${CMAKE_INSTALL_DOCDIR}`, config template `/etc/sentinel`, D-Bus service
`/usr/share/dbus-1/services` (`apps/sentinel-desktop/CMakeLists.txt:456-494`).
**One defect:** the systemd user unit is installed to
`${CMAKE_INSTALL_PREFIX}/lib/systemd/user` with a hard-coded `lib`
(`apps/sentinel-desktop/CMakeLists.txt:488-490`) rather than `${CMAKE_INSTALL_LIBDIR}`. On multiarch
`/usr/lib64` systems this could diverge, though Fedora uses `/usr/lib/systemd/user`.

### A10. XDG configuration/data locations — Partial

- Data/config/logs use `QStandardPaths` (`core/src/platform/StandardPathProvider.cpp:16-24,81-148`):
  settings under `AppConfigLocation`, databases under `~/Documents/Sentinel/*.sqlite3`
  (`:85-128`), logs/crashes under `AppLocalDataLocation` (`:130-148`). The daemon profile uses
  `QStandardPaths::AppDataLocation` (`apps/sentinel-daemon/service/DaemonService.cpp:59-62`).
- **The IPC socket is not XDG-based:** default `QDir::homePath() + "/.sentinel/run/daemon.sock"`
  (`apps/sentinel-daemon/service/DaemonIpcServer.cpp:345-351`; Windows named pipe variant), matched
  by the desktop client (`apps/sentinel-desktop/src/DaemonClient.cpp:33-35`) and the Rust IPC crate
  (`cli/crates/sentinel-ipc/src/lib.rs:178-179`).
- `packaging/linux/etc/sentinel/config.json.template` is installed to `/etc/sentinel` but **no
  production code reads `system_defaults` or `/etc/sentinel`** — it is installed-but-inert.

### A11. Daemon startup and shutdown — Implemented

`apps/sentinel-daemon/main.cpp:23-63` is a `QCoreApplication` with `--portable`, `--socket`,
`--profile-name`; it claims the endpoint before opening stores
(`apps/sentinel-daemon/service/DaemonService.cpp:50-57`), installs SIGINT/SIGTERM handlers
(`main.cpp:53-54`), and drains through `DaemonIpcServer::requestShutdown()`
(`DaemonIpcServer.cpp:466-494`) before `QCoreApplication::quit()`. The legacy CLI is documented as
an in-process exception, not a daemon client (`docs/development/DAEMON.md`).

### A12. Systemd user-service integration — Partial

- Unit `packaging/linux/systemd/sentinel-desktop.service`: `Type=exec`,
  `ExecStart=/usr/bin/sentinel-daemon`, `Restart=on-failure`, `RestartSec=5s`, `Slice=app.slice`,
  `WantedBy=default.target` (`:7-14`). It is a **user** unit.
- **No hardening directives** exist (`NoNewPrivileges`, `ProtectSystem`, `ProtectHome`,
  `PrivateTmp`, `RestrictAddressFamilies`, `SystemCallFilter`, etc. are all absent).
- **No systemd readiness integration** (`sd_notify`/`LISTEN_FDS` unused), and the unit is not
  socket-activated.
- **Dual-start risk:** the desktop also launches the daemon detached at startup
  (`apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp:82-102`), while the unit starts it
  at login (`WantedBy=default.target`). The daemon's `QLockFile` + stale-socket probe
  (`DaemonIpcServer.cpp:418-454`) prevents a competing listener, but the two launch paths are an
  ownership/UX hazard that must be resolved deliberately (see §5).

### A13. Tray / KDE Plasma behavior — Partial

- Tray is Qt-generic: `QSystemTrayIcon` + `QMenu` in
  `apps/sentinel-desktop/src/NativeCompanionAdapter.cpp:83-116`, availability-gated at `:187`; a
  second tray for notifications in `apps/sentinel-desktop/src/DesktopShellViewModel.cpp:642-676`; QML
  `TrayCompanionWindow.qml` (registered in `ui/qml/qmldir:43`).
- `core/src/app/CompanionService.cpp:180-181,222-224` describes Linux behavior as
  "StatusNotifier/AppIndicator/system tray through Qt tray when supported; degrades gracefully."
- **No `StatusNotifierItem`/`KStatusNotifierItem`/`KGlobalAccel` code exists**; KDE integration is
  whatever Qt's platform theme provides. Unverified on Plasma.

### A14. Wayland/X11 compatibility — Partial

- No source sets a QPA plugin. The only `QT_QPA_PLATFORM` uses are tests and `%check`
  (`tests/.../test_legacy_cli_exit_codes.cpp:316`, `.../test_agent_reliability_harness.cpp:4000`,
  spec `:52`, CI `:116,151`) — all `offscreen`.
- Graphics backend is selected by RHI: Linux defaults to `OpenGL` unless `QSG_RHI_BACKEND`/
  `QT_QUICK_BACKEND` overrides (`apps/sentinel-desktop/src/GraphicsBackend.cpp:38-59`,
  `bootstrap/GraphicsInitializer.cpp:15-21`). No explicit Wayland/X11 branch.
- Flatpak requests both `--socket=wayland` and `--socket=fallback-x11`
  (`packaging/linux/flatpak/dev.sentinel.Sentinel.json:9-21`), but this is unverified.

### A15. SELinux — Absent

Repo-wide search finds **no** SELinux/AppArmor artifacts: no `*.te`, no `restorecon`/`semanage`/
`fcontext`, no policy module, no `selinux` string. `policy` hits are application-level concepts
(`PolicyEvaluator`, `PermissionPolicyService`), not MAC policy. Fedora/Ro-ASD SELinux denials are
therefore entirely uncharacterized.

### A16. polkit — Absent

No polkit action/policy file under `packaging/`. Sentinel performs no privileged system action, so
polkit is not currently required by Sentinel itself; it becomes relevant only through Ro-ASD's
installer/store (see §2, §6).

### A17. Release artifact generation — Partial

- `.github/workflows/release.yml` triggers on `v*` tags (`:3-5`). On Linux it configures/builds
  `package-ready`, installs to `dist/`, runs `cpack` (`.deb`/`.rpm`), tars, builds an AppImage via
  pinned `linuxdeploy`, and writes `SHA256SUMS-Linux.txt` (`:39-80`).
- macOS produces a DMG (signed/notarized only with secrets, `:108-147`); Windows produces EXE/MSI
  (`:158-245`); a `publish` job creates a **draft** release (`:274-286`) and states publication
  remains blocked by SEC-P1.2/SEC-P1.3 (`:280-281`).
- **Gaps:** no Linux signing; the Rust CLI/TUI is not built or packaged; Flatpak/Snap/COPR are never
  produced; and the `package-ready` build preset builds **only the `sentinel-desktop` target**
  (`CMakePresets.json:116-121`), while the Fedora spec's `%files` expects `sentinel-daemon` and
  `sentinel-cli` too — the CPack/release path and the spec/COPR path diverge.

### A18. D-Bus activation — Partial (declared only)

`packaging/linux/dbus/dev.sentinel.Sentinel.service` (`Name=dev.sentinel.Sentinel`,
`Exec=/usr/bin/sentinel-desktop`, `SystemdService=sentinel-desktop.service`) is installed, but
**no code registers on the session bus** (`QDBus`/`registerService`/`registerObject` absent in
app/core code), and `SystemdService` points at the daemon unit while `Exec` launches the GUI — an
internal mismatch. D-Bus activation is currently inert.

---

## 2. Verified Ro-ASD packaging interfaces (Part B)

All Ro-ASD statements below are file-cited to the public repositories inspected read-only on
2026-10-17. Anything not evidenced is explicitly marked unverified.

### B1. Distribution base and architecture

- `Ro-ASD-release/VERSION.yaml`: `product_name: Ro-ASD`, `product_release: 44`,
  `base_distribution: Fedora Linux`, `fedora_release: 44`, `phase: development`,
  `primary_architecture: x86_64`, `component_artifact_manifest: component-artifact-manifest-v1`,
  `component_version: 0.1.2`.
- Org profile README: "**Ro-ASD** is a Fedora-based Linux distribution … leveraging … Btrfs,
  Wayland, PipeWire."
- `Ro-Repo/README.md`: "`x86_64` Tier 1 ve release blocker; `aarch64` Tier 2 Technology Preview'dur."
  (x86_64 is Tier 1 and release-blocking; aarch64 is Tier 2 tech preview.)

### B2. The application release contract (`ro-app-template`)

`ro-app-template` is the canonical template for Ro-ASD application repositories. Its README defines
the contract a component such as Sentinel must satisfy:

1. `.roasd/app.json` metadata (`schema_version: 1`, `component`, `package_names`,
   `fedora_release: 44`, `architectures: [x86_64, aarch64]`, `source_rpm: true`,
   `trusted_release_workflow: .github/workflows/release.yml`).
2. Two project-specific, **fail-closed** hooks: `tools/project-ci` (build/test/lint) and
   `tools/build-rpm <version> <arch> <outdir>` (must write the exact Fedora 44 RPM/SRPM set).
3. A stable CI status check named **`ro-app-gate`** that org/repository policy depends on.
4. A `release.yml` that runs only for exact `v*` tags and: validates `app.json`; verifies the tag
   resolves to the workflow commit; builds Fedora 44 RPMs for the declared architectures; verifies
   package name/version/`.fc44`/architecture/SRPM parity; creates a draft release; emits
   `SHA256SUMS` + `component-artifact-manifest-v1.json`; creates GitHub artifact attestations;
   verifies the asset set; then publishes.
5. **No production signing key is present in the component workflow** — "Production signing remains
   centralized in Ro-Repo."
6. Official producer onboarding is a reviewed change to `Ro-Repo/config/producers-v1.yaml`
   ("Creating a repository from this template … does **not** make it an official package producer").

### B3. The artifact manifest schema (`component-artifact-manifest-v1`)

From `.github/contracts/component-artifact-manifest-v1.schema.json`:

| Field | Constraint |
| --- | --- |
| `component.name` | `^[a-z0-9][a-z0-9+.-]*$` |
| `source.repository` | **`^https://github\.com/Project-Ro-ASD/[A-Za-z0-9_.-]+$`** |
| `source.commit` | 40-hex |
| `source.tag` | `^v[0-9]...` |
| `build.kind` | `ci` \| `release` |
| `build.fedora_release` | **const `44`** |
| `build.builder_image` | **`^registry\.fedoraproject\.org/fedora:44@sha256:[0-9a-f]{64}$`** |
| `artifacts` | array, **minItems 2** (RPM + SRPM) |
| `artifacts[].release` | `^[A-Za-z0-9+_.~]+\.fc44$` |
| `artifacts[].arch` | enum `x86_64` \| `noarch` \| `src` |

Three constraints are decisive for Sentinel (see §9):

1. **`source.repository` must be a `Project-Ro-ASD/*` repository.** A package built directly from
   `github.com/sentinel-projects/sentinel` cannot be represented by the manifest.
2. **The build must run in a pinned `registry.fedoraproject.org/fedora:44@sha256:...` image**, not a
   GitHub-hosted Ubuntu runner with `install-qt-action`.
3. **`artifacts[].arch` has no `aarch64` value**, even though `app.json`/producers allow aarch64
   architectures — an internal contract ambiguity to confirm with Ro-ASD.

### B4. Producer registry and acceptance (`Ro-Repo`)

- `Ro-Repo/README.md`: "Ro-Repo, Ro-ASD paketlerini kabul eden, merkezi olarak imzalayan ve
  degismez repository snapshot'lari … olusturan yayin aracidir" (accepts packages, signs them
  centrally, builds immutable snapshots). V2 trust chain: fixed producer/tag/commit + manifest/hash
  + provenance → acceptance → central RPM signature → immutable snapshot → beta/stable view.
- `Ro-Repo/config/producers-v1.yaml` and `producers-v2.json` define per-repository producer policy.
  Example fields: `allowed_package_names`/`package_names`, `architectures`, `risk_class`
  (`normal-app`, `critical-desktop`, `critical-system`), `promotion_group`, `required_tests`
  (`dependency-solve, clean-install, upgrade, file-conflict, rpmlint, smoke` + `plasma-integration`,
  `login-session` for `critical-desktop`), `srpm_required`, `sbom_required`,
  `allow_fedora_override`, `trusted_signer_workflow`.
- `Ro-Repo/docs/PRODUCER-REGISTRY-V2.md`: a *repository* is the source/trust boundary; a *component*
  is an independently released policy unit inside it. `producers-v1.yaml` remains temporarily
  supported; new production acceptance uses v2.
- Public keys (fingerprints in `Ro-ASD-release` README): RPM signing subkey
  `38CB87F6FBD645309432A6E22E02DEE8828B769B`; repository-metadata signing
  `1715DDA529B9ADB46D47CB389A62B8F9026E75E5`. Private keys never enter component repos.

### B5. Channels and client repository configuration

- `Ro-ASD-release` README: `ro-asd-repos` defines DNF channels `ro-asd-beta` (open by default),
  `ro-asd-beta-source` (closed), `ro-asd-stable` (closed), `ro-asd-stable-source` (closed). The beta
  channel is open first because the public Ro-Repo V2 tree has no stable channel yet. All definitions
  enforce **both** RPM and repository-metadata signature verification and depend on
  `ro-asd-keyring >= 0.1.0`.
- `Ro-Store/packaging/repository/ro-asd.repo`:
  `baseurl=https://repo.ro-asd.org/rpm/fedora/44/{beta,stable}/$basearch/`, `gpgcheck=1`,
  `repo_gpgcheck=1`, `metadata_expire=300`, `skip_if_unavailable=0`.

### B6. Image composition, installer, and first-run ownership

- `.github/docs/fedora-44-release-compose-baseline-tr.md` names repo ownership:
  `.github` (contracts), `Ro-ASD-release` (identity/policy/branding/defaults/kernel/desktop meta),
  **`Ro-image-compose`** (Fedora base, profile, future image/package/payload definitions),
  `Ro-Repo` (signed publishing/snapshot/promotion), `Ro-Main` (intro). It states the first real image
  work starts only when `repository_snapshot` is non-null and bound to a signed immutable Fedora 44
  candidate; null snapshot at `candidate`/`release` is rejected. Image build uses **KIWI**.
  `ro-Control` and `Ro-Store` are deliberately **not** added to the standard profile until security
  blockers are cleared.
- `ro-Installer` README: Ro-Installer installs a prepared Ro-Compose filesystem to disk and is
  "**not a distro builder**". It **does not own** "general ISO composition/remixing, **runtime
  Internet dependency, runtime DNF package installation as product policy**, repository
  creation/wiring, COPR configuration, …". Its privileged boundary is
  `user GUI → pkexec /usr/libexec/ro-installer-helper → /usr/libexec/ro-installer-backend`.
- `Ro-ASD-release/docs/DESKTOP-STANDARD-PROFILE-V1.md`: `ro-asd-desktop-standard` requires only the
  foundation (`ro-asd-release >= 0.1.2`, `ro-asd-keyring >= 0.1.0`, `ro-asd-repos >= 0.1.0`,
  `ro-asd-defaults >= 0.1.0`, `ro-asd-branding >= 0.1.0`); the Fedora KDE package set is selected by
  "the future Ro-ASD compose definition", and optional Ro apps (`ro-control`, `ro-music`,
  `ro-terminal`) are not required.

### B7. Defaults, branding, and per-app ownership

- `Ro-ASD-release/docs/DEFAULTS-OWNERSHIP-V1.md` rules: never write into an existing user's home; do
  **not** seed `/etc/skel/.config`; do not use KConfig immutable markers; system admin-editable files
  under `/etc` use `%config(noreplace)`; do not replace files owned by `kde-settings` or another
  Ro-ASD component; **a new default has exactly one owner**. Ownership matrix places
  "**Application-specific defaults → The application package itself**."
- `Ro-ASD-release/docs/BRANDING-CONTRACT-V1.md`: do not replace `/usr/lib/os-release`, Fedora
  `system-release`, or provide the `system-release` capability.

### B8. Application store (`Ro-Store`)

- `Ro-Store` README: a Qt6/QML graphical store that installs/removes/updates **RPM** apps through
  **DNF5 daemon + D-Bus**, adds/enables the Ro-ASD repo, and uses **polkit** for repo configuration
  (`org.projectro.RoStore.repository.manage`, helper `/usr/libexec/ro-store/ro-store-repo-helper`).
  Flatpak is explicitly out of scope; Ro-ASD apps stay in their own catalog.

### B9. Integration approach assessment (what is verifiable)

| Candidate approach | Verdict | Basis |
| --- | --- | --- |
| **Native RPM via the Ro-ASD producer/repo contract** | **Recommended, verifiable target** | `ro-app-template` + `component-artifact-manifest-v1` + `Ro-Repo` acceptance/signing/channels |
| Distribution package repository (Ro-Repo) | Required complement | Producers are onboarded into `Ro-Repo/config/producers-*`; updates delivered through `ro-asd-*` channels |
| Image package manifest (Ro-image-compose) | Needed for "default preinstalled" | Compose owns base/profile/image; **manifest format not public** |
| Direct `.deb`/AppImage/Flatpak into the image | Not supported by evidence | `Ro-Store` excludes Flatpak; `ro-Installer` rejects runtime DNF as product policy |

---

## 3. Packaging and runtime dependency matrix

### 3.1 Build-time dependencies

| Sentinel requirement | Fedora 44 package | Present in spec? | Notes |
| --- | --- | --- | --- |
| CMake ≥ 3.24 (presets need 3.27+) | `cmake`, `ninja-build` | Yes | `SPEC:8-10` |
| C++20 compiler | `gcc-c++` | Yes | `SPEC:9` |
| Qt Core/Gui/Quick/Qml/Sql | `qt6-qtbase-devel`, `qt6-qtdeclarative-devel`, `qt6-qtsql-devel` | Yes | `SPEC:12-14` |
| Qt Multimedia/Concurrent/Network/Widgets | `qt6-qtmultimedia-devel`, (`qt6-qtbase-devel`) | Multimedia yes; Concurrent/Network/Widgets implicit via qtbase | `CMakeLists.txt:60` |
| Qt LinguistTools | `qt6-qttools-devel` | Yes | `SPEC:16` |
| desktop-file-utils | `desktop-file-utils` | Yes | `SPEC:17` |
| AppStream validation | `libappstream-glib` (spec) / `appstreamcli` (CI) | Yes | `SPEC:18` |
| Rust 1.94 (CLI/TUI) | `rust-toolchain`/rustup | **No** | Rust package not built in spec |

### 3.2 Runtime dependencies

| Requirement | Fedora 44 package | Declared? | Notes |
| --- | --- | --- | --- |
| Qt runtime | `qt6-qtbase`, `qt6-qtdeclarative`, `qt6-qtsql`, `qt6-qtmultimedia` | Yes | `SPEC:20-23` |
| Icon theme | `hicolor-icon-theme` | Yes | `SPEC:24` |
| Notifications | `libnotify` | Recommends | `SPEC:25` |
| URL/scheme opening | `xdg-utils` | Recommends | `SPEC:26` |
| Credential Secret Service | `libsecret` / `gnome-keyring` | Recommends | `SPEC:27` |
| mDNS/avahi (provider discovery) | `avahi-tools` | Recommends | `SPEC:28` |
| Local model runtime | `ollama` (external) | Not declared | user-chosen provider, no bundled weights |
| Sandbox | `bubblewrap` | **Not declared** | required for Linux sandbox policy (`docs/security/SANDBOX.md`) |

### 3.3 Ro-ASD-side contract fields Sentinel must produce

| Ro-ASD field | Required value | Sentinel current state |
| --- | --- | --- |
| `app.json.component` | stable lowercase id, e.g. `sentinel` | n/a (no `.roasd/app.json`) |
| `app.json.fedora_release` | `44` | n/a |
| `app.json.architectures` | `x86_64` (+ `aarch64` if claimed) | n/a |
| `tools/build-rpm` / `tools/project-ci` | implemented, fail-closed | n/a |
| CI check `ro-app-gate` | present | n/a |
| `component-artifact-manifest-v1.json` | emitted from `v*` release | n/a |
| `source.repository` | `Project-Ro-ASD/...` | `sentinel-projects/sentinel` (does not match) |
| `build.builder_image` | pinned `fedora:44@sha256:...` | Ubuntu runner (does not match) |
| RPM `release` | `*.fc44` | spec uses `1%{?dist}` → `1.fc44` on Fedora 44 (ok) |

---

## 4. Recommended integration architecture

Sentinel stays an independent project. Ro-ASD-specific integration lives entirely at the packaging
and distribution boundary, consistent with `PD-D06` ("package app only, no weights/auto-download")
and AD-A01/AD-A02.

```
sentinel-projects/sentinel (unchanged core; independent releases)
        │  tagged source + LICENSE + THIRD_PARTY_NOTICES
        ▼
Ro-ASD packaging boundary (owned by Ro-ASD, reviewed):
        │   • .roasd/app.json
        │   • tools/build-rpm  → builds sentinel RPM+SRPM in fedora:44 container
        │   • tools/project-ci → runs sentinel's own tests/metadata checks
        │   • release.yml (v*)  → component-artifact-manifest-v1 + attestations
        ▼
Ro-Repo  (accept → central production RPM sign → immutable snapshot → channel)
        │   channels: ro-asd-beta (open) / ro-asd-stable (closed today)
        ▼
Ro-image-compose (KIWI)    +   Ro-Store (DNF5/D-Bus/polkit)
        │   preinstalls sentinel by default            updates/removals via DNF
        ▼
Ro-ASD image  →  ro-Installer deploys prepared filesystem (no runtime DNF policy)
```

Design rules:

1. **No fork of Sentinel architecture.** The Ro-ASD RPM packages the same `sentinel-desktop`,
   `sentinel-daemon`, and (if in scope) Rust `sentinel` binaries produced from tagged Sentinel
   source.
2. **Packaging repo location is a decision, not an assumption.** Because the manifest schema
   requires `Project-Ro-ASD/*`, the two viable options are (a) a Ro-ASD-owned packaging/recipe
   repository under `Project-Ro-ASD` that builds Sentinel from a pinned upstream tag, or (b) a
   reviewed relaxation/extension of the manifest schema. Sentinel's own repository should not be
   renamed or moved.
3. **Signing stays centralized in Ro-Repo.** Sentinel does not need, and must not hold, Ro-ASD
   production signing keys; component CI produces unsigned RPMs plus provenance.
4. **Update/rollback follows Ro-Repo + DNF5.** Rollback per `Ro-Repo/docs/REMOTE-BETA-ROLLBACK.md`
   and `RETENTION-POLICY.md`; version pinning via `promotion-baselines-v1.json`.
5. **Defaults ownership respects `DEFAULTS-OWNERSHIP-V1`.** Any configuration Sentinel ships is
   owned by the Sentinel package itself, placed under `/etc`/`/usr/share` as
   `%config(noreplace)` where admin-editable, never written into user homes or `/etc/skel`.

---

## 5. Desktop and daemon startup policy

Today Sentinel has **two** daemon launch paths plus a Quick-Panel autostart. The recommended policy
resolves them so the Ro-ASD image has exactly one owner.

| Path | Current behavior | Evidence | Recommended policy |
| --- | --- | --- | --- |
| Desktop auto-launch | `ensureBackgroundDaemon()` starts `sentinel-daemon` **detached** unless `--safe-mode` or `--no-daemon-start` | `ApplicationBootstrapper.cpp:82-102,134-153,260` | Keep as the default single owner; daemon exits if the socket lock is held |
| systemd user unit | `ExecStart=/usr/bin/sentinel-daemon`, `WantedBy=default.target` | `packaging/linux/systemd/sentinel-desktop.service:7-14` | Do **not** enable by default; either ship it `--disabled` or remove login activation, so it is not a second starter |
| Quick-Panel autostart | Writes `~/.config/autostart/dev.sentinel.Sentinel.desktop` only when the user enables it | `NativeCompanionAdapter.cpp:347-372` | Keep; explicit user-controlled, product-policy governed |
| D-Bus activation | File installed but inert | `packaging/linux/dbus/dev.sentinel.Sentinel.service` | Either implement bus registration + fix `Exec`/`SystemdService`, or drop the file to avoid a false contract |

Additional policy requirements:

- **Privilege:** the daemon is a **user-level** `QCoreApplication` and must run as the session user.
  No system service, no `root`, no setuid. This matches `ro-asd`'s user-session model.
- **Broad privileges:** none. `bubblewrap` sandboxing applies to *tool execution*, not to daemon
  startup; the daemon should not be started with a privileged unit or capabilities.
- **Autostart governance:** Sentinel must not auto-launch the daemon with elevated scope; on a Ro-ASD
  image the only automatic daemon start should be the desktop's detached launch (or an explicit,
  documented user service), decided by product policy and tested for the "one owner" criterion.

---

## 6. Security requirements (Part D)

| Concern | Requirement | Current state / evidence |
| --- | --- | --- |
| User vs system service | User-level only; no root daemon | User unit + `QCoreApplication`; no system unit (`systemd/...service`) |
| Socket permissions | Private dir 0700, owner-only socket | `DaemonIpcServer.cpp:391-416` (`chmod 0700`, reject `0077`), `:455` `UserAccessOption` |
| Peer identity | Reject foreign UID | `DaemonIpcServer.cpp:539-563,572-580` (`SO_PEERCRED`/`getpeereid`) |
| Process sandbox | Linux `bubblewrap` with namespace/fs/optional net; deny if unavailable | `docs/security/SANDBOX.md`; `bubblewrap` not declared in RPM |
| Package install privileges | Via Ro-ASD helpers (DNF5/polkit), not Sentinel | `Ro-Store` polkit `org.projectro.RoStore.repository.manage`; `ro-Installer` pkexec helper |
| Application update trust | Ro-Repo central signing; RPM + repo metadata GPG verification | `ro-asd.repo` `gpgcheck=1`, `repo_gpgcheck=1`; keys `38CB87F6…`, `1715DDA5…` |
| Filesystem ownership | One package owns each file; `%config(noreplace)`; no user-home writes | `DEFAULTS-OWNERSHIP-V1.md`; Sentinel installs `config.json.template` only |
| SELinux denials | Must be measured on the Ro-ASD image; none characterized | **Absent** in Sentinel; unverified for the image |
| polkit requirements | Only for privileged repo/system mutation | Sentinel: none; Ro-ASD store/installer: yes |
| IPC access | Socket lock + per-UID peer check + UserAccessOption | as above |
| Provider credential storage | OS Secret Service preference; never bundled | `docs/security/SECRETS.md`; `libsecret` Recommends `SPEC:27` |
| Release signing / provenance | Component CI unsigned + attestations; production signing in Ro-Repo | `ro-app-template` README; `Ro-Repo` central signing; Sentinel has **no** Linux signing |
| Telemetry / privacy | No telemetry; no automatic model download | `docs/security/SECURITY_MODEL.md` ("Sentinel documents no telemetry collection"); `packaging/linux/etc/sentinel/config.json.template` `telemetry_enabled: false`, `automatic_updates_enabled: false` |

No security exception is proposed to simplify packaging. Where Sentinel currently lacks a control
(SELinux policy, linux package signing), the requirement is recorded as an **open item**, not waived.

---

## 7. Installation / upgrade / removal plan

### 7.1 Installation (default preinstall)

1. Sentinel RPM is accepted by Ro-Repo and published to `ro-asd-beta` (and later `ro-asd-stable`).
2. `Ro-image-compose` (KIWI) includes the tagged Sentinel RPM in the `desktop-standard` compose
   definition, pinned to an exact version/`promotion-baseline`.
3. `ro-Installer` deploys the prepared filesystem. **No runtime DNF install**, no model download, no
   credentials bundled.
4. On first login, `plasma-setup`/`ro-assist` own OOBE; Sentinel's own onboarding runs on first
   Sentinel launch (§Appendix A).

### 7.2 Upgrade

- DNF5 updates Sentinel from the Ro-Repo channel. Package upgrades must preserve user data
  (settings under `AppConfigLocation`, SQLite stores, models).
- Version-matched cross-repo contract: `app.json`/manifest version must equal the RPM version and the
  pinned compose version (`SENTINEL_PLATFORM_ROADMAP.md` P7 criterion 5).
- Rollback via `Ro-Repo` snapshot/retention policy; no destructive reinstall.

### 7.3 Removal

- `dnf remove` (or `Ro-Store`) removes package-owned files. Per `DEFAULTS-OWNERSHIP-V1` and P7
  criterion 3, removal must **not** silently erase user-owned conversations, memory databases, or
  models (they live under user paths, not package paths).
- The `/etc/sentinel/config.json.template` is `%config`; removal should not delete an admin-modified
  config without an explicit choice.

### 7.4 Data boundaries (must hold across all lifecycle steps)

| Data | Owner | Location | Removed on uninstall? |
| --- | --- | --- | --- |
| App settings | user | `AppConfigLocation/settings.json` | No (user-owned) |
| Memory/chat/conversations/SQLite | user | `~/Documents/Sentinel/*.sqlite3` | No (user-owned) |
| Models | user | provider-managed (`ModelStorageManager`) | No |
| Daemon profile/runtime | user | `QStandardPaths::AppDataLocation` | No |
| System config template | package | `/etc/sentinel/…` | `%config` semantics |
| Binaries/assets/metadata | package | `/usr/bin`, `/usr/share/...` | Yes |

---

## 8. Fedora KDE compatibility risks

| Risk | Severity | Evidence | Mitigation |
| --- | --- | --- | --- |
| No Fedora CI; all Linux CI is Ubuntu 22.04 | High | `ci-linux.yml:18`; `SEC_P1_3…:98` BLOCKED | Add a Fedora 44 build/test job (P7-02) |
| Metadata/package version drift (`1.0.0` vs `1.0.0-rc.8`) | Medium | `CMakeLists.txt:5`, spec/metainfo/snap | Single-source version in packaging |
| Two specs, divergent content | Medium | `packaging/linux/rpm/*`, `packaging/linux/fedora-kde/*` | One canonical spec |
| Release/CPack builds only `sentinel-desktop` | High | `CMakePresets.json:116-121` vs spec `%files` | Build daemon/cli in the package preset |
| Daemon dual-start (desktop + systemd) | Medium | `ApplicationBootstrapper.cpp:82-102`, unit | Resolve to one owner (§5) |
| systemd unit lacks hardening | Medium | unit `:7-14` | Add hardening directives |
| D-Bus file inert / mismatched | Medium | dbus service; no `QDBus` code | Implement or remove |
| Tray/KDE unverified (Qt-generic only) | Medium | `NativeCompanionAdapter.cpp:83-116`; no StatusNotifier code | Native Plasma/Wayland test |
| Wayland/X11 unverified | Medium | no QPA handling; RHI OpenGL | Test on Plasma Wayland |
| Socket not XDG; Flatpak path mismatch | Low (image) / High (Flatpak) | `~/.sentinel/run/daemon.sock`; flatpak finish-args | Align or document; Flatpak out of Ro-ASD scope |
| SELinux denials unmeasured | Medium | no SELinux artifacts | Measure on image; add policy if needed |
| Rust CLI unpacks | Low–Medium | no RPM; not in CI package | Decide CLI/TUI scope (PD-D08) |
| AppStream minimal (no screenshots/developer id) | Low | metainfo; `appstreamcli` infos | Improve metadata |

---

## 9. Required decisions with Ro-ASD maintainers

These are decisions the Ro-ASD side must make or confirm; none can be inferred safely.

| ID | Decision | Why it blocks |
| --- | --- | --- |
| D1 | **Where does the Sentinel producer/manifest live?** A `Project-Ro-ASD/*` packaging repo, or a schema relaxation for `source.repository`? | The manifest schema requires `^https://github\.com/Project-Ro-ASD/…$`; Sentinel is `sentinel-projects/sentinel` |
| D2 | **Fedora 44 pinned builder.** Do Ro-ASD accept a component-owned Fedora 44 (digest-pinned) RPM build, or must Ro-Repo build? | `build.builder_image` requires `registry.fedoraproject.org/fedora:44@sha256:…`; Sentinel CI is Ubuntu |
| D3 | **Package names and split.** One `sentinel-desktop` RPM (bundling daemon/cli) or separate `sentinel` / `sentinel-daemon` / `sentinel-cli` RPMs? | `app.json.package_names` + `producers-*` `allowed_package_names` must match exactly |
| D4 | **aarch64 support.** Is aarch64 required now? | Schema `artifacts[].arch` has no `aarch64`; Ro-Repo treats aarch64 as Tier-2 preview |
| D5 | **Architectures / risk class / required tests.** Which `risk_class` and which of `dependency-solve, clean-install, upgrade, file-conflict, rpmlint, smoke` apply? | `producers-*` entries are reviewed policy |
| D6 | **channels.** Will Sentinel ship in `ro-asd-beta` only initially (stable closed)? | Stable channel is not published in the public Ro-Repo V2 tree |
| D7 | **default preinstall.** Is Sentinel added to the `desktop-standard` compose and/or the profile meta-package? | `ro-asd-desktop-standard` currently requires only foundation; compose is not public |
| D8 | **Onboarding/OOBE handoff.** Does `plasma-setup`/`ro-assist` invoke Sentinel onboarding, or does Sentinel own it? | `DESKTOP-STANDARD-PROFILE-V1` records the lifecycle split but not app onboarding |
| D9 | **CLI/TUI scope.** Ship the Rust `sentinel` binary in the image, and under what package name (PRD-D08 is open)? | Avoids `sentinel` vs `sentinel-cli` collision confusion |
| D10 | **SELinux policy expectation.** Is an SELinux policy module required for Sentinel, or does the default `targeted` policy suffice? | No SELinux evidence exists on either side |
| D11 | **Support/ownership.** Who owns cross-repo triage and rollback (PD-D06)? | Recorded blocker in `PD-D06`; no assigned human |

---

## 10. Concrete P7-02 implementation scope

Ordered, testable scope for the follow-up implementation (in Sentinel's own repositories plus a
Ro-ASD-reviewed packaging repository). No architecture fork; no bundled models/credentials.

**Sentinel repository changes (P7-02, Sentinel-owned):**

1. **Version source of truth:** derive spec/metainfo/snap versions from `SENTINEL_APP_VERSION`
   (`CMakeLists.txt:5`) so `1.0.0-rc.8` is consistent across metadata.
2. **Single Fedora spec:** collapse `packaging/linux/rpm/*` and `packaging/linux/fedora-kde/*` into
   one canonical `sentinel-desktop.spec`, `Release: 1%{?dist}`, shipping `sentinel-desktop` (+ a
   decision on `sentinel-daemon`/`sentinel-cli`).
3. **Build parity:** make the packaging build produce all shipped binaries (fix
   `CMakePresets.json` `package-ready` `targets`, or use a dedicated package preset) so the CPack and
   spec paths agree.
4. **AppStream/metadata:** add `<developer id>`, screenshots, and a release entry matching the actual
   version; keep ids `dev.sentinel.Sentinel`.
5. **systemd hardening + single owner:** add `NoNewPrivileges`, `ProtectSystem=strict`, `ProtectHome`,
   `PrivateTmp`, `RestrictAddressFamilies=AF_UNIX`; decide enable-vs-desktop-launch so only one
   daemon start path is automatic.
6. **D-Bus:** implement session-bus registration and fix `Exec`/`SystemdService`, or remove the file.
7. **Rust CLI/TUI packaging decision:** either add an RPM/man-page/static build, or record explicit
   deferral (ties to D9).
8. **Fedora CI job:** add a Fedora 44 (container) configure/build/test/`rpmlint`/`desktop-file-validate`/
   `appstreamcli` job; keep it independent of Ro-ASD.
9. **Dependency declaration:** add `bubblewrap` runtime dependency/recommendation for Linux sandbox.
10. **RPM autorelease/metadata lint** wired into CI.

**Ro-ASD packaging repository (P7-02, Ro-ASD-owned, reviewed):**

11. `.roasd/app.json` with `component: sentinel`, `package_names`, `fedora_release: 44`,
    `architectures`, `source_rpm: true`, `trusted_release_workflow`.
12. `tools/build-rpm` (Fedora 44 digest-pinned container) and `tools/project-ci` (calls Sentinel's
    own tests/metadata checks); fail-closed.
13. `release.yml` for `v*` producing RPM+SRPM, `SHA256SUMS`, `component-artifact-manifest-v1.json`,
    and GitHub attestations; no production keys.
14. Onboarding change to `Ro-Repo/config/producers-v1.yaml`/`producers-v2.json` (reviewed).
15. Compose/profile change to preinstall `sentinel` in `desktop-standard` (Ro-image-compose).

**Tests to satisfy (maps to Appendix B):** clean install, upgrade, removal, offline/no-model startup,
daemon IPC, tray, CLI/TUI, desktop+daemon coexistence — on a real x86_64 UEFI/Plasma/Wayland Ro-ASD
image.

---

## Appendix A — First-run experience design & acceptance mapping (Part C)

The requested first-run guarantees map to existing Sentinel components; each is **implemented** or an
explicit **P7-02 task**.

| # | Requirement | Status | Evidence / P7-02 action |
| --- | --- | --- | --- |
| C1 | Installable by default | P7-02 (Ro-ASD compose) | Package + compose change (§10) |
| C2 | No LLM model downloaded without consent | Implemented | Onboarding model step only *selects*; "No local models found. **Download one from the Models page after setup.**" `ui/qml/onboarding/ModelSetupStep.qml:76`; onboarding steps `OnboardingScreen.qml:64-67` |
| C3 | API credentials never bundled | Implemented | `docs/security/SECRETS.md` (OS secret store preference); no credentials in packaging; `libsecret` only recommended |
| C4 | Daemon not auto-started with broad privileges | Implemented (design) | User-level `QCoreApplication`; detached user launch; **P7-02:** ensure systemd unit is not auto-enabled, add hardening |
| C5 | Autostart explicitly governed | Implemented | Quick-Panel autostart written only on explicit user enable: `NativeCompanionAdapter.cpp:347-372` |
| C6 | Missing providers reported clearly | Implemented | Truthful no-provider failure, no fallback: `docs/development/DAEMON.md` ("A missing provider still gives truthful runtime failure, with no new fallback path"); onboarding `ProviderSetupStep.qml`; `FinishStep.qml:96-103` "Set up later" |
| C7 | Install/uninstall preserve user-data boundaries | Implemented (design) | Data under user paths (`StandardPathProvider.cpp`); P7 criterion 3; **P7-02:** verify with synthetic profiles |
| C8 | Fedora package upgrades are safe | P7-02 | Requires Fedora test job + upgrade fixtures |
| C9 | Ro-ASD changes don't break generic Fedora packages | Design | Integration confined to packaging boundary; no Fedora identity replacement (`BRANDING-CONTRACT-V1`) |

Onboarding wizard exists at `ui/qml/onboarding/` (Welcome → ProcessingMode → ProviderSetup →
ModelSetup → VoiceSetup → PrivacyConsent → Finish) with a `PrivacyConsentStep.qml`. The model step
does not start downloads. This satisfies `AC-P3-01`'s intent (no fresh-install model acquisition
without choice) at the UI level; end-to-end consent trace remains part of P7-02.

---

## Appendix B — Compatibility validation matrix (Part E)

Legend: **U** = unit/static evidence available today; **P** = pending native execution (no Ro-ASD
image); **B** = blocked by an open decision.

| Cell | Target | Required evidence | Status |
| --- | --- | --- | --- |
| E1 | Fedora KDE native hardware | Fedora/Plasma/Wayland version, install/run | **P** |
| E2 | Ro-ASD installed environment | Image digest, package/service identity | **P / B (ENV-ROASD unavailable)** |
| E3 | Clean installation | Zero unchosen model downloads/credentials; files present | **P** (UI evidence **U**) |
| E4 | Package upgrade | Version/data preserved, one owner | **P** |
| E5 | Package removal | User data preserved; package files removed | **P** |
| E6 | Offline startup | No network dependency at start | **P** |
| E7 | No-model startup | Truthful missing-provider/model state | **P** (code **U**) |
| E8 | Daemon IPC | Socket perms, peer-UID, lock, single owner | **U** (unit-verified) → **P** native |
| E9 | System tray | Plasma tray/StatusNotifier presence | **P** |
| E10 | Rust CLI/TUI | Build, package, connect to daemon | **U** (build/CI) → packaging **B** (D9) |
| E11 | Desktop + daemon coexistence | Exactly one daemon owner; no double start | **U** (lock) → **P** native (§5) |
| E12 | SELinux behavior | `ausearch`/`audit2allow` perms, no denials | **P** |

Existing acceptance rows already encode most of this and remain **Blocked**:
`docs/roadmaps/ACCEPTANCE_MATRIX.md:215` (`AC-P7-01`), `:216` (`AC-P7-02`), `:63` (`ENV-ROASD`).

---

## Appendix C — Validation executed and results (Part F)

Static checks executed on the audit host against the worktree at `6035bf74cbb3`:

| Check | Command | Result |
| --- | --- | --- |
| Working-tree cleanliness | `git diff --check` | clean |
| Base verification | `git merge-base HEAD origin/main` | `6035bf74…` = HEAD |
| AppStream (base) | `appstreamcli validate --no-net packaging/linux/dev.sentinel.Sentinel.metainfo.xml` | **Pass** — infos: `developer-name-tag-deprecated`, `developer-info-missing` |
| AppStream (fedora-kde) | `appstreamcli validate --no-net packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml` | **Pass** (identical findings) |
| Desktop entry parse | Python `configparser` on the `.desktop` | **OK** — `Exec=sentinel-desktop %U`, `Icon=dev.sentinel.Sentinel`, `Type=Application` |
| Flatpak JSON | Python `json.load` on `packaging/linux/flatpak/dev.sentinel.Sentinel.json` | **OK** |
| Metainfo XML | Python `ElementTree.parse` | **OK** (both files) |

Checks **not** run (documented as pending, tooling absent or out of isolation scope):

- `desktop-file-validate` — binary absent on host (pending).
- `rpmlint` / `rpmbuild` / `rpmspec -P` — absent on host (pending; Fedora job required).
- `appstreamcli validate --pedantic`/`appstream-util` — partially available; only the above ran.
- Full `cmake --preset tests` build/`ctest` — not executed (no authorized Fedora/native session;
  the task requires no system alteration; not a packaging gate for this audit).
- No `dnf` installs, no RPM builds, no Ro-ASD image build, no release-channel or signing changes.

---

## Appendix D — Evidence index

**Sentinel (paths relative to worktree root):**

- Packaging/CMake: `CMakeLists.txt`; `apps/sentinel-desktop/CMakeLists.txt`; `apps/sentinel-cli/CMakeLists.txt`;
  `apps/sentinel-daemon/CMakeLists.txt`; `apps/sentinel-plugin-host/CMakeLists.txt`; `core/CMakeLists.txt`;
  `cmake/SentinelCPack.cmake`; `cmake/SentinelSigning.cmake`; `cmake/SentinelSignPackages.cmake`;
  `CMakePresets.json`.
- Linux assets: `packaging/linux/dev.sentinel.Sentinel.desktop`;
  `packaging/linux/dev.sentinel.Sentinel.metainfo.xml`; `packaging/linux/fedora-kde/*`;
  `packaging/linux/rpm/*`; `packaging/linux/systemd/sentinel-desktop.service`;
  `packaging/linux/dbus/dev.sentinel.Sentinel.service`; `packaging/linux/etc/sentinel/config.json.template`;
  `packaging/linux/flatpak/*`; `packaging/linux/snap/snapcraft.yaml`.
- Runtime: `apps/sentinel-daemon/main.cpp`; `apps/sentinel-daemon/service/DaemonService.cpp`;
  `apps/sentinel-daemon/service/DaemonIpcServer.cpp`; `apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp`;
  `apps/sentinel-desktop/src/NativeCompanionAdapter.cpp`; `apps/sentinel-desktop/src/DaemonClient.cpp`;
  `apps/sentinel-desktop/src/GraphicsBackend.cpp`; `core/src/platform/StandardPathProvider.cpp`;
  `cli/crates/sentinel-ipc/src/lib.rs`; `ui/qml/onboarding/*`.
- Docs: `docs/development/BUILDING.md`; `docs/development/TESTING.md`; `docs/development/DAEMON.md`;
  `docs/release/PACKAGING.md`; `docs/release/RELEASE_PROCESS.md`; `docs/security/*`;
  `docs/roadmaps/SENTINEL_PLATFORM_ROADMAP.md` (P7); `docs/roadmaps/ACCEPTANCE_DECISIONS.md` (AD-A01/A02, PD-D06);
  `docs/roadmaps/ACCEPTANCE_MATRIX.md` (ENV-ROASD, AC-P7-01/02); `docs/roadmaps/GITHUB_ISSUE_BACKLOG.md` (EP-P7).
- CI: `.github/workflows/{release,ci-linux,ci-rust,ci-macos,ci-windows,codeql-analysis}.yml`;
  `.github/dependabot.yml`.

**Ro-ASD (public, read-only, 2026-10-17):**

- `Project-Ro-ASD/.github`: `profile/README.md`;
  `docs/fedora-44-release-compose-baseline-tr.md`;
  `contracts/component-artifact-manifest-v1.schema.json`.
- `Project-Ro-ASD/ro-app-template`: `README.md`; `.roasd/app.json`; `tools/{build-rpm,project-ci,validate-roasd-app.py}`.
- `Project-Ro-ASD/Ro-ASD-release`: `README.md`; `VERSION.yaml`; `docs/{DESKTOP-STANDARD-PROFILE-V1,DEFAULTS-OWNERSHIP-V1,BRANDING-CONTRACT-V1,KERNEL-POLICY-V1}.md`;
  `packages/ro-asd-*`.
- `Project-Ro-ASD/Ro-Repo`: `README.md`; `config/{producers-v1.yaml,producers-v2.json,fedora-44-package-names.txt,promotion-baselines-v1.json}`;
  `docs/{PRODUCER-REGISTRY-V2,OPERATIONS,PRODUCTION-SIGNING-CI,...}.md`; `keys/production`.
- `Project-Ro-ASD/ro-Installer`: `README.md`; `ro-installer.spec`.
- `Project-Ro-ASD/Ro-Store`: `README.md`; `packaging/{linux,repository,polkit}`.
- Repository inventory: 22 public repositories (classification via `roasd_type` custom property).

**External baseline:** Fedora Packages (`qt6-qtbase` 6.10.2/6.11.2 on Fedora 44), consulted 2026-10-17.

---

## Summary of readiness

- **Sentinel is Fedora-compatible at the Qt level** (Qt 6.5+ requirement met by Fedora 44) and has
  real Linux packaging assets, but the **RPM path is not distribution-grade**: no Fedora CI, version
  drift, two divergent specs, a build preset that does not build the daemon/CLI it ships, no Linux
  signing, no SELinux/polkit, an inert D-Bus file, and an unhardened dual-start systemd unit.
- **Ro-ASD's integration contract is well-defined and verifiable**: `ro-app-template` +
  `component-artifact-manifest-v1` + `Ro-Repo` acceptance/signing/channels + `Ro-image-compose` +
  `Ro-Installer`. Preinstallation is a **compose/profile** decision, not a runtime-install decision.
- **The safest approach is a native RPM produced under the Ro-ASD producer contract and onboarded
  into Ro-Repo**, with Sentinel's core untouched. Three contract mismatches (source repository
  namespace, pinned Fedora 44 builder, aarch64/manifest ambiguity) require explicit Ro-ASD decisions
  (§9).
- **No first-run/consent risks were found** in Sentinel's onboarding (no auto model download, no
  bundled credentials, explicit autostart), but native/Ro-ASD validation remains **blocked** until an
  `ENV-ROASD` image exists.
