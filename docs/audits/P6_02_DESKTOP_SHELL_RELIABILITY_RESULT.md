# P6-02 — Desktop Shell Reliability & State Restoration (Result)

Audit date: 2026-10-17
Status: **Implemented and validated on macOS (offscreen Qt). No commit, push, merge, reset,
stash, PR or workflow dispatch was performed.**
Branch: `feature/p6-02-desktop-shell-reliability`
Base: `origin/main` = `6035bf74cbb3f2556e29fe1bf31163a56eb5b591`
(`feat(resources): add native resource telemetry monitor adapters and sampling tests`)
Worktree: independent worktree of this repository (separate from INT-06, P7-01 and all other
active worktrees; those were not modified)

Related roadmap acceptance criteria: **AC-P6-06** (desktop/Quick Panel/tray flow) in
[`ACCEPTANCE_MATRIX.md`](../roadmaps/ACCEPTANCE_MATRIX.md); epic **EP-P6** in
[`GITHUB_ISSUE_BACKLOG.md`](../roadmaps/GITHUB_ISSUE_BACKLOG.md). This work closes the two P1
desktop defects identified by the P6-01 baseline audit:
**D-P6-02** (no window geometry/state restoration, = finding F29) and
**D-P6-03** (global error channel is dead, = finding F34).

---

## 0. Method, scope and evidence limits

### 0.1 What this task implements
| Part | Requirement | Delivered |
| --- | --- | --- |
| A | Persist width/height, position where supported, maximized vs normal; safe restart restore; never restore off-screen; handle display disconnect/resolution change; respect WM behavior (no assumed Wayland absolute positioning); avoid excessive disk writes; sensible first-launch defaults; no sensitive data | `WindowStatePersistence.h/.cpp` + `ApplicationBootstrapper` wiring |
| B | Preserve connected-vs-ready semantics; no credentials/sensitive detail; no duplicate banners for equivalent failures; don't turn chat errors into global alerts; clear/update only when state justifies; don't bypass error normalization or IPC boundaries | `DesktopErrorProjection.h` + `DesktopShellViewModel` projection + `Main.qml` banner action |
| C | Regression protection | `tests/desktop/test_desktop_shell_state.cpp` (deterministic, offscreen) |
| D | Validation | §5 below |

### 0.2 Preserved architecture (no redesign)
- The desktop remains a **pure daemon-backed IPC client**. No in-process fallback, no second
  runtime, no new transport, no Python/Electron.
- `DaemonClient` / `DesktopRuntimeClient` split, generation fencing and no-replay semantics are
  untouched. Failures continue to be normalised by the existing transport layer.
- QML stays presentation-only: the banner reads view-model properties and calls invokable slots;
  no business logic was added to QML.
- No KDE shortcuts, no new navigation, no background services, no permission-policy change.

### 0.3 Evidence limits (explicitly unverified)
- **No native GUI acceptance** on this host: Fedora KDE Plasma (X11 and Wayland), macOS-ARM
  *native* (non-offscreen), and Windows are **unverified**. All GUI validation here is
  `QT_QPA_PLATFORM=offscreen` Qt 6.12 plus deterministic unit tests. AC-P6-06 remains partially
  blocked pending a native KDE/X11 + Wayland confirmation.
- On Wayland the compositor owns placement; absolute `x/y` is intentionally **not applied** and
  the live "re-home after monitor unplug" path is a **no-op by design** (see §2.4). That is a
  documented limitation, not a defect.

---

## 1. What changed

| File | Kind | Summary |
| --- | --- | --- |
| `apps/sentinel-desktop/include/sentinel/desktop/WindowStatePersistence.h` | new | `WindowGeometry`, pure `WindowStateRules` (`resolve`, `isReachable`), `WindowStateController` |
| `apps/sentinel-desktop/src/WindowStatePersistence.cpp` | new | Geometry rules, `QSettings` load/store, debounced writes, screen-change re-homing |
| `apps/sentinel-desktop/include/sentinel/desktop/DesktopErrorProjection.h` | new | Pure daemon-state → banner-category policy |
| `apps/sentinel-desktop/include/sentinel/desktop/DesktopShellViewModel.h` | modified | New error-projection API/members |
| `apps/sentinel-desktop/src/DesktopShellViewModel.cpp` | modified | Unified error projection, daemon signal hookup, cold-start grace, banner action |
| `apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.{h,cpp}` | modified | Window-state controller wiring; startup-recovery reporting |
| `apps/sentinel-desktop/CMakeLists.txt` | modified | Register new sources in `sentinel_desktop_startup`; add `Qt6::Gui`; list new header in viewmodels |
| `ui/qml/Main.qml` | modified | Banner gets `actionLabel` + `onActionTriggered → triggerGlobalErrorAction()` |
| `tests/desktop/test_desktop_shell_state.cpp` | new | 16 deterministic slots for Part A/B |
| `tests/CMakeLists.txt` | modified | Register `test_desktop_shell_state` (offscreen, 60 s timeout) |

`git diff --stat` (tracked files): 7 files changed, 188 insertions(+), 6 deletions(-); plus 4 new
files (768 lines total).

---

## 2. Part A — window geometry & state persistence

### 2.1 Storage isolation (no sensitive data, no clobbering)
State is stored through a dedicated `QSettings` file at
`m_pathProvider.settingsFilePath() + ".window"` in `QSettings::IniFormat`. This:
- respects `--preferences-directory` (tests and portable installs never touch a developer's real
  preferences);
- does **not** clobber the existing `JsonSettingsStore` `.desktop` file;
- stores only numeric geometry plus a maximized flag — no titles, paths, session identifiers,
  content, or credentials.

Example stored file (from the live smoke run):

```ini
[mainWindow]
hasPosition=true
height=800
maximized=false
schema=1
width=800
x=2
y=2
```

A `schema=1` version key gates reads, so an older/newer unknown schema is ignored rather than
mis-decoded.

### 2.2 Deterministic, platform-independent rules — `WindowStateRules`
- `kDefaultWidth/Height = 1320×860` (the previous hard-coded `Main.qml` default) give sensible
  first-launch geometry; `kMinimumWidth/Height = 780×640` enforce the existing minimum.
- `resolve(saved, screens, primary)`:
  - enforces the minimum size;
  - **clamps size to the current topology** so a reduced resolution never yields an oversized,
    unusable window;
  - **never restores an off-screen position**: a frame that does not overlap any attached screen
    has its position dropped (`hasPosition = false`) so the WM can place it;
  - a frame that partially overlaps its screen is clamped so it stays reachable;
  - if no screens are known, position is dropped and default size is returned.
- `isReachable(x, y, w, h, screens)` is the pure predicate used both by `resolve` and the live
  screen-change handler; it ignores invalid screen rectangles.

Because the rule set is pure (no cached display state), the same saved geometry yields the same
result for a given topology — this is what makes it unit-testable.

### 2.3 `WindowStateController`
- `attach(window)` loads state, resolves it, resizes/sets position, restores maximized state,
  then subscribes to size/position/state changes.
- **Position is applied only where the platform honours absolute placement**
  (`platformHonoursPositioning()` is false for any `wayland*` platform), so no assumption is made
  that Wayland accepts `x/y`.
- Writes are **debounced (750 ms single-shot timer)** and **deduplicated** against the last
  stored value, so a continuous resize/move produces one write; a final `saveNow()` runs on
  `QGuiApplication::aboutToQuit` so state is not lost on a clean exit.
- The "normal" (non-maximized) frame is captured separately, so un-maximizing after a restart
  restores a sane size rather than the maximized dimensions.

### 2.4 Display disconnect / resolution change
`handleScreensChanged()` is connected to `QGuiApplication::screenRemoved`, `screenAdded` and
`primaryScreenChanged`. If the window is no longer reachable, it is re-homed onto the primary
screen (clamped to fit) and the new geometry is scheduled for persistence. On Wayland this path
returns early — the compositor owns placement — which is the documented, deliberate limitation.

### 2.5 Bootstrap wiring
`ApplicationBootstrapper::setupQmlEngine` creates the dedicated `QSettings` and the
`WindowStateController` after the root window exists, and attaches the `QWindow`. Nothing else in
the boot sequence changes.

---

## 3. Part B — unified global error presentation

### 3.1 Pure projection policy — `DesktopErrorProjection.h`
`desktopErrorCategoryForDaemonState()` maps the transport state to a banner category:

| `DaemonClient::ConnectionState` | Category | Banner |
| --- | --- | --- |
| `Unavailable` | `DaemonUnavailable` | "daemon unavailable", Warning, **Retry** |
| `Reconnecting` | `DaemonDisconnected` | "lost connection", Error, **Reconnect** |
| `Connected` | `None` | clears a daemon-owned banner |
| `Connecting`, `Disconnected`, `VersionMismatch`, `ShuttingDown` | `std::nullopt` | leave banner untouched |

`isDaemonErrorCategory()` distinguishes the two daemon categories from `None`/`StartupRecovery`.

### 3.2 View-model integration
- The **remote** `DesktopShellViewModel` constructor connects
  `DaemonClient::connectionStateChanged → refreshDaemonErrorProjection()`.
- `applyGlobalError()` / `clearGlobalError()` centralise state; `setGlobalError()` is retained for
  existing callers (category `None`) and now **deduplicates** identical
  message/severity/action/category, so re-projection can never stack banners.
- `dismissGlobalError()` clears the banner (the user's explicit action wins).
- `triggerGlobalErrorAction()` performs a **reconnect only** for daemon categories (never replays
  a mutation), keeping acceptance-undertain replay out of the shell.
- `reportStartupRecovery(code)` surfaces a repaired settings store as a generic, non-sensitive
  `Warning` banner. The raw `RecoveryService` identifier is never displayed.

### 3.3 Cold-start grace (prevents a false alarm)
On a normal launch the app connects *before* the bootstrapper starts the daemon, so the transport
briefly reaches `Unavailable`. To avoid flashing an error banner on every launch:
- `Reconnecting` (a real, post-ready loss) is surfaced **immediately**;
- `Unavailable` (never reached ready) is deferred by a **5 s grace window**; a daemon that is
  still starting up causes no banner, while a daemon that stays absent is still surfaced.
- `setDaemonAvailabilityGracePeriodMs()` exposes the interval (0 disables it) and doubles as the
  test seam.
- The grace deliberately survives transient `Connecting` retries (so retry loops cannot starve
  it) and is closed by `Connected`/`Reconnecting`.

### 3.4 Connected-vs-ready semantics preserved
The banner uses the same `DaemonClient` state machine that already drives `daemonConnected()`
(`daemonReachable()` == `Connected`). No new "ready" notion is invented, and a restored connection
only clears a banner this projection owns (`isDaemonErrorCategory`), so a `StartupRecovery`
warning is not silently dismissed by an unrelated reconnect.

### 3.5 What is deliberately *not* global
Provider/model/chat/run/approval failures are **not** routed to the global banner: they already
surface per-channel and must not become global alerts. The global banner is reserved for
daemon-level shell failures and one-time startup recovery, matching P6-01 §9 ("wire one unified
error projection to the existing banner").

### 3.6 QML
`ui/qml/Main.qml`'s existing `ErrorBanner` gains `actionLabel` binding and
`onActionTriggered → root.viewModel.triggerGlobalErrorAction()`. `onDismissed` still calls
`dismissGlobalError()`. No other visuals change.

---

## 4. Part C — regression protection

`tests/desktop/test_desktop_shell_state.cpp` (16 slots, offscreen, deterministic):
- `defaultGeometryWhenNothingSaved`, `enforcesMinimumSize`, `rejectsFullyOffscreenPosition`,
  `clampsPartiallyOffscreenPosition`, `clampsOversizedWindowToReducedResolution`,
  `allowsSecondaryScreenPosition`, `dropsPositionWhenNoScreens`, `preservesMaximizedFlag`,
  `reachabilityDetectsStrandedFrames` — pure Part A rules;
- `persistenceWritesCapturedGeometry`, `restoresSavedGeometryOnAttach`,
  `recoversFromInvalidSavedGeometry` — real `QSettings` round-trip through the controller;
- `daemonStateMapsToBannerCategory` — pure Part B policy including the `nullopt` states;
- `daemonUnavailableBannerAppearsAndDismisses` — banner appears against a real `DaemonClient`
  with a missing socket and dismisses;
- `daemonUnavailableBannerWaitsForGracePeriod` — grace defers then surfaces across retries;
- `repeatedEquivalentErrorsAreDeduplicated` — identical failures emit one change.

These are wired into CTest (offscreen, 60 s timeout) so CI guards both features.

---

## 5. Part D — validation evidence

Host: macOS (arm64), AppleClang, Qt 6.12.0 (`/opt/homebrew/opt/qt`), CMake/Ninja, ccache.

| Activity | Command | Result |
| --- | --- | --- |
| Full build | `cmake --build build/tests` | **success, 0 errors** (544/544 steps) |
| Focused desktop/IPC/QML set | `QT_QPA_PLATFORM=offscreen ctest -R '<desktop+QML+IPC+contract set>'` | **24/24 passed, 0 failed** (~64 s) |
| New test (direct) | `QT_QPA_PLATFORM=offscreen ./build/tests/tests/test_desktop_shell_state` | **18 passed, 0 failed, 0 skipped** (~1.3 s) |
| Complete CTest | `QT_QPA_PLATFORM=offscreen ctest --test-dir build/tests` | **143/143 passed, 0 failed** (218 s) |
| QML lint | `qmllint -I ui/qml -I build/tests/apps/sentinel-desktop $(find ui/qml -name '*.qml')` | exit 0; **113 warnings / 0 errors**, unchanged from base (added `Main.qml` lines add no warnings) |
| C++ formatting | CI-pinned `clang-format==18.1.8` via `clang-format-diff.py -p1` over `git diff -U0 -- apps core integrations plugins tests` | **clean** (empty diff) |
| New-file formatting | `clang-format --dry-run --Werror` on the 4 new/edited files | **clean** |
| Personal-path scan | `python3 tools/analysis/check_personal_paths.py` | **0 files flagged** |
| Whitespace/conflict check | `git diff --check` | **clean** |
| Secret/credential scan | `grep -Ei 'api[_-]?key\|secret\|password\|token\|bearer\|BEGIN (RSA\|OPENSSH\|PRIVATE)'` over changed files | no new material (only pre-existing API-key accessors and `MotionTokens`) |
| Static analysis | `clang-tidy -p build/tests` on the new sources | no errors; only `readability-braces-around-statements` style notes (see §5.2) |
| Headless smoke 1 | `QT_QPA_PLATFORM=offscreen sentinel-desktop --safe-mode --no-daemon-start --preferences-directory <tmp>` | stayed alive; created `settings.json.window` — Part A persistence observed end-to-end |
| Headless smoke 2 | same + `--daemon-socket <tmp>/nope.sock`, reusing the prefs dir | stayed alive; restored saved state; no crash/error output |

### 5.1 Complete CTest
`QT_QPA_PLATFORM=offscreen ctest --test-dir build/tests` → **143/143 tests passed, 0 failed**
(real 218 s), `CTEST_EXIT=0`. This is the entire `build/tests` suite, not just the P6-02 subset,
so the window-state and error-projection tests run alongside every other desktop, IPC, core,
contract and benchmark-adjacent test target.

### 5.2 Static analysis note
`clang-tidy` on `WindowStatePersistence.cpp` reports only
`readability-braces-around-statements` on single-statement `if` bodies. This matches the dominant
existing codebase style (e.g. `core/src/app/AppSettings.cpp`), and the repository's `.clang-tidy`
sets `WarningsAsErrors: ''`, so these are non-fatal and consistent with the surrounding code — no
change was made. `ApplicationBootstrapper.cpp` could not be analysed with clang-tidy 23 because
the CMake PCH was produced by AppleClang (a format mismatch); this is an environment limitation,
not a source issue, and there is no new logic in the bootstrapper beyond wiring.

---

## 6. Decisions and rationale

| Decision | Rationale |
| --- | --- |
| Separate `.window` `QSettings` file, not `DesktopSettingsStore` | Avoids clobbering the JSON settings store and needs no schema change; isolated per `--preferences-directory` |
| Pure `WindowStateRules` / `desktopErrorCategoryForDaemonState` | Fully deterministic and unit-testable without a display or daemon |
| Position applied only off-Wayland | Compositors own placement; absolute `x/y` is meaningless there |
| 750 ms debounce + dedupe + quit flush | Bounds disk writes without losing state on clean exit |
| Cold-start grace for `Unavailable` only | Prevents a false banner on every launch while still surfacing a genuinely absent daemon |
| `Reconnecting` immediate | A lost connection after ready is a real, actionable failure |
| Daemon failures only in the global banner | Provider/model/chat failures stay per-channel (no global noise) |
| Banner action = reconnect only | Never replays a mutation whose acceptance is uncertain |
| `schema` version key | Guards against mis-reading a future/foreign layout |

---

## 7. Platform limitations (unverified)

- Fedora KDE Plasma under **X11** and **Wayland**: source-reviewed only; native window restore,
  multi-monitor re-homing and Wayland "no absolute position" behaviour are **not** natively
  verified here.
- **Wayland** live re-homing after a monitor unplug is a deliberate no-op (compositor owns
  placement).
- **macOS native** (non-offscreen) and **Windows** GUI behaviour: unverified.
- The complete CTest run is on macOS only; Linux/Windows CI parity is not claimed in this document.

## 8. Remaining UI defects / recommended P6-03 scope

- Wayland Quick Panel **anchoring** (portal / layer-shell vs documented limitation).
- Native notification portal backend; system-appearance + high-DPI handling.
- Dead-chrome decision; long-conversation model pagination.
- Accessibility contract (roles/names) on base controls + `SentinelOverlayModal` (P6-01
  D-P6-04 / UI-014).
- The existing QML warning baseline is large (113 warnings) and pre-dates this task; a
  `unqualified`/`unused-imports` cleanup is worthwhile but out of P6-02 scope.

## 9. Reproduction commands

```bash
cmake --preset tests -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build/tests
QT_QPA_PLATFORM=offscreen ctest --test-dir build/tests --output-on-failure
QT_QPA_PLATFORM=offscreen ./build/tests/tests/test_desktop_shell_state

# CI-pinned formatting (v18.1.8):
python3 -m pip install --user clang-format==18.1.8
git diff -U0 -- apps core integrations plugins tests | clang-format-diff.py -p1   # must be empty

qmllint -I ui/qml -I build/tests/apps/sentinel-desktop $(find ui/qml -name '*.qml')
```
