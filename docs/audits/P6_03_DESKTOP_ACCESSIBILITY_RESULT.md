# P6-03 — Desktop Accessibility & Keyboard Usability (Result)

Audit date: 2026-10-17
Status: **Implemented and validated on macOS (offscreen Qt). No commit, push, merge, reset,
stash, PR or workflow dispatch was performed.**
Branch: `feature/p6-03-desktop-accessibility`
Base: `main` = `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb`
(`test(cmake): drop duplicated P4-03 test registrations introduced by integration merge`)
Worktree: the shared repository checkout; other workstreams' uncommitted changes present in the
working tree were **not** touched by P6-03 (see §0.4).

Related findings: P6-01 **D-P6-04** (accessibility metadata absent on the six core base controls
and the overlay modal) = **UI-014**, and the P6-02 §8 follow-on note "Accessibility contract
(roles/names) on base controls + `SentinelOverlayModal`". Roadmap criterion **AC-P6-06**
(desktop/Quick Panel/tray flow);

---

## 0. Method, scope and evidence limits

### 0.1 What this task implements
| Part | Requirement | Delivered |
| --- | --- | --- |
| A | Accessible roles / names / descriptions, keyboard activation, focus indicators, disabled semantics and tab behaviour on the six core base controls; no duplicate announcements, no unnecessary bindings, no focus loops; preserve existing mouse/keyboard behaviour | `SentinelButton`, `SentinelTextField`, `SentinelComboBox` (control + delegate), `SentinelSwitch`, `SentinelSlider`, `SentinelSpinBox` |
| B | Approval + recovery overlay dialog semantics, names, initial focus, keyboard actions and safe focus restoration; approval never easier to trigger; no permission-policy change; daemon IPC authorization untouched | `SentinelOverlayModal.qml` focus contract + `Main.qml` approval/recovery overlays |
| C | Preserve P6-02 (window-state persistence, geometry restore, daemon global-error banner, reconnect, desktop settings); do not touch unrelated Agent/provider/resource components | No P6-02 or unrelated component was modified; P6-02 tests re-run green (§5.2) |
| D | Deterministic offscreen Qt/QML accessibility tests + lint/format/privacy validation | New `--accessibility-smoke` mode + `accessibility_certification.qml` fixture (§5) |

### 0.2 Preserved architecture (no redesign)
- QML remains **presentation-only**. No new property is required from a view model, and no business
  logic was moved into QML.
- **No permission-policy change.** The approval overlay still calls the same
  `viewModel.respondToDaemonApproval(false/true)` / `cancelDaemonRun()` slots; the daemon IPC
  authorization path is untouched.
- **Approval is not made easier to trigger.** The default-focus contract pins focus to *Deny*; a
  stray Enter/Space on the freshly opened dialog can only deny. `Allow once` is reachable only by an
  explicit Tab then activation.
- No global **Wayland shortcuts**, no visual redesign, no new navigation, no new dependencies.

### 0.3 Evidence limits (explicitly unverified)
- **No native screen-reader verification.** VoiceOver (macOS), Orca (GNOME/Wayland/X11) and NVDA
  (Windows) were **not** exercised. All accessibility validation is programmatic
  (`QAccessible::queryAccessibleInterface` role/name/state assertions) and keyboard-event driven
  under `QT_QPA_PLATFORM=offscreen` with Qt 6.12. This matches the P6-01 limitation (UI-014).
- **No native GUI acceptance**: Fedora KDE Plasma (X11 and Wayland), macOS-ARM *native*
  (non-offscreen) and Windows remain **unverified** for these changes.
- Focus-ring *pixel* appearance on each platform/theme was not re-verified; only that the focus
  indicator bindings (`activeFocus`) already present are preserved.

### 0.4 Shared-worktree note
This checkout contained pre-existing, uncommitted work from other workstreams (Fedora packaging,
CI workflow, and a resource-policy telemetry adapter). P6-03 did not author, modify or revert any of
those files. Only the components listed in §1 belong to P6-03.

---

## 1. What changed (exact modified components)

| File | Kind | Summary |
| --- | --- | --- |
| `ui/qml/components/base/SentinelButton.qml` | modified | `Accessible.role/name/description/pressed` (name = label, description = tooltip); Enter/Return activation |
| `ui/qml/components/base/SentinelTextField.qml` | modified | `Accessible.role: EditableText`, name (placeholder fallback), `accessibleDescription` + `readOnly` |
| `ui/qml/components/base/SentinelComboBox.qml` | modified | `Accessible.role: ComboBox`, `expandable/expanded`; delegate → `ListItem` with name/selected/checked |
| `ui/qml/components/base/SentinelSwitch.qml` | modified | `Accessible.role: Switch`, `checkable/checked/pressed` |
| `ui/qml/components/base/SentinelSlider.qml` | modified | `Accessible.role: Slider`, `accessibleDescription` |
| `ui/qml/components/base/SentinelSpinBox.qml` | modified | `Accessible.role: SpinBox`, `accessibleDescription`, `readOnly` |
| `ui/qml/components/dialogs/SentinelOverlayModal.qml` | modified | `initialFocusItem` / `previousFocusItem` contract, `applyInitialFocus()` / `restorePreviousFocus()` on `aboutToShow` / `aboutToHide` |
| `ui/qml/Main.qml` | modified | Approval + recovery overlays: `Accessible.Dialog` + name/description, `StaticText` labels, `initialFocusItem` on Deny / Restore |
| `tests/helpers/settings_ui_certification_probe.cpp` | modified | New `--accessibility-smoke` mode (checks 201–236) + flag/load-path wiring |
| `tests/helpers/accessibility_certification.qml` | **new** | Disposable offscreen fixture mounting the production base controls and `SentinelOverlayModal` |
| `tests/CMakeLists.txt` | modified | Registers `test_desktop_accessibility` (offscreen, `TIMEOUT 60`) |
| `docs/audits/P6_03_DESKTOP_ACCESSIBILITY_RESULT.md` | **new** | This document |

### 1.1 Empirical constraints that shaped the implementation
All three were confirmed on this host against Qt 6.12 offscreen:
1. `Accessible.*` **cannot** be attached to a `Popup` itself — Qt emits a runtime warning
   (`Accessible must be attached to an object deriving from Item or Action`) and applies nothing.
   The correct target is the popup's `contentItem` (the convention already used by
   `NotificationCenterPanel.qml`). The base therefore relies on the usage site naming `contentItem`.
2. A plain `Text` item is **not** exposed as an accessibility element, but `Label` **is** (role
   `StaticText`). This is why the overlay titles/descriptions were migrated to `Label` with an
   explicit `Accessible.name`, and why there is no duplicate-announcement risk from the button's
   internal `Text`.
3. Qt Quick Controls (Basic) activates a focused button with **Space only**; Enter/Return is *not*
   handled. This is a real keyboard/screen-reader gap and is the reason Part A adds Enter/Return
   activation to `SentinelButton` (see §2.2).

---

## 2. Part A — the six core base controls

### 2.1 Roles, names, descriptions
- **`SentinelButton`** — `Accessible.Button`; name = visible `text`, falling back to `tooltipText`
  for icon-only buttons; description = tooltip only when it carries information beyond the label
  (avoids repeating the name); `Accessible.pressed` tracks `down`.
- **`SentinelTextField`** — `Accessible.EditableText`; name defaults to the placeholder text; new
  `accessibleDescription` property; `Accessible.readOnly` mirrors `readOnly`.
- **`SentinelComboBox`** — `Accessible.ComboBox`, `expandable: true`,
  `expanded: control.popup.visible`. The popup delegate (default `ItemDelegate`, previously exposed
  with an empty name) is now `Accessible.ListItem` with a name from the rendered label and
  `selected`/`checked` mirroring `currentIndex`.
- **`SentinelSwitch`** — `Accessible.Switch`, `checkable`, `checked`, `pressed`.
- **`SentinelSlider`** — `Accessible.Slider`; value range/position remain derived by
  QtQuick.Controls; new `accessibleDescription`.
- **`SentinelSpinBox`** — `Accessible.SpinBox`; value/editable state derived; `readOnly` mirrors
  non-editable mode; new `accessibleDescription`.

Each role/description is a single declarative binding — no extra `Connections`, no per-instance
wiring, so no unnecessary bindings and no duplicate announcements. A usage site that sets
`Accessible.name` (e.g. `"Provider"`) overrides the component default as usual.

### 2.2 Keyboard activation and focus
- `focusPolicy: Qt.StrongFocus` was already present on the interactive controls and is asserted by
  the test (check 211).
- **`SentinelButton` now also activates on Enter/Return** (`Keys.onReturnPressed` /
  `Keys.onEnterPressed`), consuming the event so it cannot also reach an ancestor handler and fire
  the action twice. Space behaviour is unchanged.
- Existing focus indicators (`activeFocus` bindings on border/glow/scale) were **not** altered.

### 2.3 Disabled semantics and tab behaviour
- Disabled controls keep their accessible role but cannot take focus and cannot be activated
  (asserted by checks 204/214/215). No `Accessible.role` is dropped when `enabled === false`.
- Tab navigation still reaches the next visible focusable control (check 223). `activeFocusOnTab`
  handling was not changed.

---

## 3. Part B — approval and recovery overlays

### 3.1 Focus contract in `SentinelOverlayModal.qml`
The overlay gained a small, reusable keyboard contract:

- `initialFocusItem` — the item that receives focus when the overlay opens. When unset, the first
  visible, enabled, tab-focusable direct child of the content is used, preserving earlier behaviour.
- `previousFocusItem` — the item that owned focus when the overlay opened.
- `applyInitialFocus()` runs from **`aboutToShow`** (deferred one event-loop turn with
  `Qt.callLater`) so it lands *after* the popup has taken focus itself.
- `restorePreviousFocus()` runs from **`aboutToHide`** so focus is returned the moment the overlay
  starts hiding.

**Why not `onOpened` / `onClosed`?** Those signals are only emitted once the enter/exit transition
has finished. They do not fire when transitions are disabled (reduced motion → `MotionTokens.fast ==
0`) or never tick (headless), and they fire *late* (≈90 ms) otherwise, which can strand focus. The
`aboutToShow` / `aboutToHide` pair is transition-independent. This was verified both for the
`open()`/`close()` API path (fixture approval overlay; `Main.qml` recovery overlay) and for the
`visible`-property path used by the daemon approval overlay in `Main.qml` (`visible:` binding) and
by the fixture recovery overlay (§5.3).

### 3.2 Approval overlay (`Main.qml`)
- `Accessible.role: Dialog` on `contentItem` with name **"Tool approval required"** and a
  description explaining the decision; the title and request detail labels are `StaticText`.
- `initialFocusItem: daemonApprovalDeny` replaces the previous `onOpened: …forceActiveFocus()`.
  Focus therefore lands on **Deny**.
- `closePolicy: Popup.NoAutoClose` is unchanged — Escape does not dismiss the approval prompt.
- **No permission-policy change and no easier approval path**: the only way to reach *Allow once* is
  an explicit Tab then activation; Enter/Return on the default focus denies.

### 3.3 Recovery overlay (`Main.qml`)
- `Accessible.role: Dialog` on `contentItem` with name **"Restore previous session"** and a
  description; the title is `StaticText`.
- `initialFocusItem: recoveryRestore` — the non-destructive *Restore* action is the default, so a
  stray Enter restores the draft rather than discarding it. *Discard* stays reachable via
  Tab / Shift+Tab.
- On close the previously focused control is restored (§3.1).

---

## 4. Part C — P6-02 preservation

- No file owned by P6-02 was modified: `WindowStatePersistence.*`, `DesktopErrorProjection.h`,
  `DesktopShellViewModel.*`, `ApplicationBootstrapper.*` and the settings store are untouched.
- Window-state persistence, geometry restore, the daemon global-error banner, reconnect handling and
  desktop settings therefore keep their P6-02 behaviour and tests (§5.2).
- Unrelated Agent, provider, resource-management and telemetry components were not modified.

---

## 5. Part D — validation

### 5.1 Deterministic accessibility test (`test_desktop_accessibility`)
New `--accessibility-smoke` mode in the existing `settings_ui_certification_probe` (offscreen Qt),
loading the new `tests/helpers/accessibility_certification.qml`. It queries real
`QAccessibleInterface` objects and drives real key events; it does not use synthetic success paths.

Checks (exit code = first failing check):

| Code | Check |
| --- | --- |
| 201–206 | Button role/name/description; icon-only button name; disabled semantics; text field role/name/description |
| 207–210 | Combo box, switch, slider and spin box role/name |
| 211–213 | Strong focus policy; button focus; **Space and Enter** activation |
| 214–215 | Disabled control cannot focus and cannot be activated |
| 216–218 | Text entry; switch keyboard toggle; exposed checked state |
| 219–220 | Slider arrow-increase; spin box arrow-increase |
| 221–222 | Combo popup opens; exposed expanded state |
| 223 | Tab reaches the next visible focusable control |
| 224–226 | Approval opens; `Dialog` role/name; **initial focus is Deny** |
| 227–229 | Escape does **not** dismiss the approval (NoAutoClose); **Enter denies** (never approves); Tab order reaches *Allow once* |
| 230–231 | Approval closes; no accidental approval recorded |
| 232–234 | Recovery opens via `visible`; **initial focus is Restore**; `Dialog` role/name |
| 235–236 | Escape closes recovery; **focus is restored** to the previously focused control |

### 5.2 Test results
| Suite | Command | Result |
| --- | --- | --- |
| Accessibility smoke | `QT_QPA_PLATFORM=offscreen … --accessibility-smoke` | **exit 0** (all checks 201–236) |
| Targeted desktop/IPC/QML set | `ctest -R 'test_desktop_accessibility\|test_extensions_surface\|test_settings_surface\|test_desktop_ipc\|test_desktop_shell_view_model\|test_desktop_shell_state\|test_desktop_generated_contract\|test_ipc_generated_contract'` | **8/8 passed** (incl. `test_desktop_ipc`, 28.7 s) |
| Full CTest | `ctest --test-dir build/tests --output-on-failure -j 4` | **150/150 passed**, exit 0 (46.4 s) |
| P6-02 regression | `test_desktop_shell_state`, `test_desktop_shell_view_model` within the full run | passed |

### 5.3 Keyboard behaviour verified (offscreen)
- **Focused button**: Space activates; **Enter/Return now activates**; disabled button never takes
  focus and never activates.
- **Text field**: typing updates the value; `readOnly` exposed.
- **Switch**: Space toggles and the accessible `checked` state updates.
- **Slider / spin box**: arrow keys change the value.
- **Combo box**: Space opens the popup and the accessible `expanded` state tracks it; Escape closes.
- **Tab**: moves to the next visible focusable control (no focus loop observed).
- **Approval overlay**: opens with focus on *Deny*; Escape keeps it open; Enter denies; Tab reaches
  *Allow once*.
- **Recovery overlay**: opens (driven through the `visible` property in the fixture — the same path
  the daemon approval overlay uses) with focus on *Restore*; Escape closes it and focus returns to
  the control that had it before.

### 5.4 QML lint (`qmllint 6.12.0`, 72 files)
Both invocations produce an **identical warning set before and after** the P6-03 change, i.e. P6-03
adds **zero** new warnings.

| Invocation | Before | After |
| --- | --- | --- |
| `qmllint -I ui/qml -I build/tests/apps/sentinel-desktop $(find ui/qml -name '*.qml')` (relative paths, as in P6-02) | 0 errors, **113 warnings** | 0 errors, **113 warnings** |
| Same over absolute paths | 0 errors, 110 warnings | 0 errors, 110 warnings |

The 3-warning difference between the two invocations is a qmllint path-resolution artifact
(`[import] Type <Singleton> not declared as singleton in qmldir …` reported for the three
`ui/qml/theme/*.qml` singletons only when they are passed through relative paths). It is unrelated
to P6-03 and appears identically before and after. The pre-existing warnings are dominated by
`unqualified` context-property access and `unused-imports` (documented in
`docs/development/DEFERRED_QML_WARNINGS.md`); none is on a P6-03 line.

### 5.5 Runtime QML warnings
`--accessibility-smoke` emits **no** QML runtime warnings (only one benign
`qt.qpa.fonts: … missing font family "Sans Serif"` note from the offscreen platform). No
`Accessible`-attachment warning is produced, confirming no `Accessible.*` was attached to a bare
`Popup`.

### 5.6 C++ formatting (CI-pinned `clang-format==18.1.8`)
`clang-format-diff.py -p1` over `git diff -U0 -- apps core integrations plugins tests` is
**empty (clean)**.

> Note on scope: the pinned formatter re-wraps a small number of **pre-existing** one-line
> statements in the same `main()` of `settings_ui_certification_probe.cpp`, because clang-format's
> line-range expansion always covers that region once the file's diff is formatted. Those changes
> are whitespace/wrapping only and are contained in the single helper file P6-03 already edits.

### 5.7 Privacy and whitespace
- `python3 tools/analysis/check_personal_paths.py` → **1 pre-existing flag**:
  `docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md` (committed at `b257dce8`,
  unrelated to P6-03, not modified here). No P6-03 file contains a personal path.
- `git diff --check` → clean (no whitespace errors).

---

## 6. Unverified native GUI scenarios

- Native screen readers (**VoiceOver / Orca / NVDA**) were not run; only programmatic role/name/state
  assertions and keyboard events under the offscreen platform.
- Native **Fedora KDE Plasma** (X11 and Wayland), native **macOS-ARM** (non-offscreen) and
  **Windows** rendering, focus-ring pixels, high-contrast themes, DPI scaling and IME behaviour for
  the changed controls were not verified.
- Real daemon-driven approval (live `AgentRuntime` request → approval popup) was not exercised; the
  IPC authorization path was not modified, and the fixture mirrors the approval contract with
  synthetic state only.

---

## 7. Remaining defects / recommended P6-04 scope

1. **Native assistive-technology certification.** Run the approval/recovery flows under Orca
   (Wayland + X11), VoiceOver and NVDA; capture announced role/name/state and focus-order evidence.
   *Closes the residual part of UI-014.*
2. **Native onboarding/checklist for keyboard-only use** across the full shell (rail, command
   palette, settings categories, chat composer).
3. **Finish the accessibility contract outside the six base controls** — the P6-01 §5.8 audit found
   `Accessible.*` in only 20/72 QML files; extend names/roles to remaining custom controls
   (`RuntimeDetailPopup`, `RuntimeBadge`, `SentinelOrb`, `GlowSurface`, onboarding steps).
4. **Token/theme correctness follow-ups from P6-01 §5.2/§5.9** (hardcoded hex, the two diverging
   motion token sets, `SentinelTheme.reducedMotion` dead flag, "Auto (System)" appearance), which
   also affect accessibility (contrast, reduced motion).
5. **QML lint baseline cleanup** (113 warnings) — out of P6-03 scope but worth a dedicated pass.

---

## 8. Reproduction commands

```bash
# Build the probe / desktop
cmake --build build/tests --target settings_ui_certification_probe -j 8

# Deterministic offscreen accessibility smoke
QT_QPA_PLATFORM=offscreen \
  ./build/tests/tests/settings_ui_certification_probe.app/Contents/MacOS/settings_ui_certification_probe \
  --accessibility-smoke

# Registered test + full suite
ctest --test-dir build/tests --output-on-failure -R test_desktop_accessibility
ctest --test-dir build/tests --output-on-failure -j 4

# QML lint (expect 0 errors, 113 warnings, identical to base)
qmllint -I ui/qml -I build/tests/apps/sentinel-desktop $(find ui/qml -name '*.qml')

# CI-pinned formatting (expect empty)
python3 -m pip install --user clang-format==18.1.8
git diff -U0 -- apps core integrations plugins tests | clang-format-diff.py -p1

# Privacy + whitespace
python3 tools/analysis/check_personal_paths.py
git diff --check
```
