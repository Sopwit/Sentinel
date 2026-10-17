# INT-07 — Shared Checkout Recovery & Integration Audit (Result)

Audit date: 2026-10-17. Phase: cross-cutting integration recovery.
Priority: Critical. Mode: safe recovery, reconciliation and independent verification.

Task: recover and reconcile four development tasks that were performed concurrently in **one shared
Git working directory**, then produce an exact, safe procedure for a maintainer to convert the mixed
uncommitted state into a verified integration branch.

- **Recovered change sets** — P2-M03 (native Anthropic/Gemini protocol extraction), P4-04 (resource
  telemetry policy adapter), P6-03 (Qt/QML desktop accessibility), P7-02A (Fedora packaging
  foundation).
- **No repository mutation of the working change sets.** No clone, no worktree, no
  `reset`/`clean`/`stash`/`checkout`/`switch`/`rebase`/`merge`/`cherry-pick`/`commit`/`push` was
  executed. Branch was **not** switched. Every preservation and validation step is read-only with
  respect to tracked source.
- **One intentional, documented correction** was applied (the pre-existing personal-path leak in
  `docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md`); see §8. It is a one-line privacy
  sanitization, backed up and reversible.

---

## 0. Verified Git identities

| Item | Value |
| --- | --- |
| Repository root | `<personal-home>/Desktop/Projects/Sentinel-sanitized` |
| `git rev-parse HEAD` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| Subject of HEAD | `test(cmake): drop duplicated P4-03 test registrations introduced by integration merge` |
| Checked-out branch | `feature/p4-04-resource-policy-adapter` (**not** authoritative — see §0.1) |
| Local `main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| `origin/main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| `main` vs `origin/main` | `0 ahead / 0 behind` (`git rev-list --left-right --count main...origin/main` → `0  0`) |
| Git version | 2.56.0 |
| Working tree | **dirty**: 30 modified + 1 deleted (unstaged) + 11 untracked; **0 staged**; **0 unmerged** |

### 0.1 Ownership/identity incident (material)

**Every local branch points at the identical commit `cd9ec8bc`.** The branch name therefore carries
**no ownership information** about the uncommitted work:

```
feature/p4-04-resource-policy-adapter        cd9ec8bc
feature/p6-03-desktop-accessibility          cd9ec8bc
feature/p7-02a-fedora-packaging-foundation   cd9ec8bc
refactor/p2-m03-native-provider-protocols    cd9ec8bc
main                                          cd9ec8bc
origin/HEAD -> origin/main                    cd9ec8bc
```

The reflog records the concurrent branch switching that caused this (newest first):

```
checkout: moving from refactor/p2-m03-native-provider-protocols to feature/p4-04-resource-policy-adapter
checkout: moving from feature/p6-03-desktop-accessibility    to refactor/p2-m03-native-provider-protocols
checkout: moving from feature/p7-02a-fedora-packaging-foundation to feature/p6-03-desktop-accessibility
checkout: moving from feature/p6-03-desktop-accessibility    to feature/p7-02a-fedora-packaging-foundation
checkout: moving from feature/p7-02a-fedora-packaging-foundation to feature/p6-03-desktop-accessibility
checkout: moving from refactor/p2-m03-native-provider-protocols to feature/p7-02a-fedora-packaging-foundation
checkout: moving from feature/p4-04-resource-policy-adapter  to refactor/p2-m03-native-provider-protocols
...
checkout: moving from main                                   to feature/p6-03-desktop-accessibility
```

Because uncommitted changes are not branch-bound, all four change sets survived the switching and
coexist in the current tree. No content was destroyed by the switching itself.

**No branch switch occurred during INT-07.** The last reflog entry is the switch that happened before
this task began; INT-07's own actions are validated against the identity above.

---

## 1. Part A — Preservation (performed before any source modification)

Recovery directory (outside the repository, in the approved temp root):

```
/private/var/folders/jn/rtnh3wpd52537ppg_tls1nm40000gn/T/opencode/int-07-recovery-20261017T215059
```

| Artefact | Contents |
| --- | --- |
| `meta/` | `root.txt`, `head.txt`, `branch.txt`, `main.txt`, `origin-main.txt`, `origin-head.txt`, `status.txt`, `status-v2.txt`, `branches.txt`, `refs.txt`, `reflog.txt` (200 entries), `log.txt`, `staged.txt`, `modified-tracked.txt`, `deleted-tracked.txt`, `untracked.txt`, `diffstat.txt` |
| `patches/unstaged.pre-p7-01-fix.patch` | binary-safe `git diff --binary` of the tree **as found** (87 866 bytes) |
| `patches/unstaged.post-fix.patch` | binary-safe `git diff --binary` after the INT-07 sanitization |
| `patches/staged.patch` | binary-safe staged diff — **0 bytes** (nothing was staged) |
| `tree/` | byte-for-byte backup of every modified + untracked file, relative paths preserved |
| `deleted-originals/` | HEAD content of the one deleted tracked file |
| `pre-implementation/NOTE_p7-01-personal-path-fix.md` | the Part-D correction documented **before** it was applied |
| `SHA256SUMS`, `SHA256SUMS.worktree`, `RECOVERY_DIR.txt` | integrity manifests |

**Backup verification (all passed):**

- `shasum -a 256 -c SHA256SUMS` → every saved file matches (66 files).
- `git apply --check -R --binary patches/unstaged.pre-p7-01-fix.patch` → **OK**: the pre-fix patch
  exactly reproduces the working tree as found (a forward `--check` correctly fails, since the
  changes are already applied).
- `git apply --check -R --binary patches/unstaged.post-fix.patch` → **OK**: the post-fix patch
  exactly reproduces the current working tree.
- `unstaged.post-fix.patch` is non-empty and readable; `staged.patch` is empty by design.

No full repository copy and no duplicate build artifacts were created. The current filesystem
contents are treated as valuable and are recoverable from `tree/` + `deleted-originals/` + the
patches.

---

## 2. Part B — File ownership matrix

Legend: **P2** = P2-M03, **P4** = P4-04, **P6** = P6-03, **P7** = P7-02A, **INT7** = this task.
"Overlap" = touched by more than one of the four change sets.

| File | Owner | State | Overlap | In `main`/`origin/main` | Expected |
| --- | --- | --- | --- | --- | --- |
| `core/include/sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h` | P2 | modified +28 | no | no | yes |
| `core/src/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.cpp` | P2 | modified +40 | no | no | yes |
| `core/include/sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h` | P2 | modified +27 | no | no | yes |
| `core/src/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.cpp` | P2 | modified +43 | no | no | yes |
| `core/src/model/ModelService.cpp` | P2 | modified +24/−55 | no | no | yes |
| `tests/core/test_anthropic_messages_protocol.cpp` | P2 | modified +92 | no | no | yes |
| `tests/core/test_gemini_generate_content_protocol.cpp` | P2 | modified +87 | no | no | yes |
| `tests/core/test_native_provider_contract.cpp` | P2 | modified +51 | no | no | yes |
| `docs/audits/P2_M03_NATIVE_PROVIDER_PROTOCOL_EXTRACTION_RESULT.md` | P2 | new | no | no | yes |
| `core/include/sentinel/core/policy/ResourcePolicyTelemetryAdapter.h` | P4 | new (156 lines) | no | no | yes |
| `core/src/policy/ResourcePolicyTelemetryAdapter.cpp` | P4 | new (426 lines) | no | no | yes |
| `tests/core/test_resource_policy_adapter.cpp` | P4 | new (631 lines) | no | no | yes |
| `core/CMakeLists.txt` | P4 | modified +2 | no | no | yes |
| `docs/audits/P4_04_RESOURCE_TELEMETRY_POLICY_ADAPTER_RESULT.md` | P4 | new | no | no | yes |
| `ui/qml/components/base/SentinelButton.qml` | P6 | modified +22 | no | no | yes |
| `ui/qml/components/base/SentinelTextField.qml` | P6 | modified +11 | no | no | yes |
| `ui/qml/components/base/SentinelComboBox.qml` | P6 | modified +20 | no | no | yes |
| `ui/qml/components/base/SentinelSwitch.qml` | P6 | modified +8 | no | no | yes |
| `ui/qml/components/base/SentinelSlider.qml` | P6 | modified +6 | no | no | yes |
| `ui/qml/components/base/SentinelSpinBox.qml` | P6 | modified +7 | no | no | yes |
| `ui/qml/components/dialogs/SentinelOverlayModal.qml` | P6 | modified +45/−10 | no | no | yes |
| `ui/qml/Main.qml` | P6 | modified +19/−1 | no | no | yes |
| `tests/helpers/settings_ui_certification_probe.cpp` | P6 | modified +344/−20 | no | no | yes |
| `tests/helpers/accessibility_certification.qml` | P6 | new (177 lines) | no | no | yes |
| `docs/audits/P6_03_DESKTOP_ACCESSIBILITY_RESULT.md` | P6 | new | no | no | yes |
| `.github/workflows/ci-fedora.yml` | P7 | new | no | no | yes |
| `.github/workflows/ci-linux.yml` | P7 | modified +2 | no | no | yes |
| `CMakePresets.json` | P7 | modified +4/−1 | no | no | yes |
| `apps/sentinel-desktop/CMakeLists.txt` | P7 | modified −4 | no | no | yes |
| `cmake/SentinelCPack.cmake` | P7 | modified +1/−1 | no | no | yes |
| `packaging/linux/dbus/dev.sentinel.Sentinel.service` | P7 | **deleted** (−4) | no | no | yes |
| `packaging/linux/dev.sentinel.Sentinel.metainfo.xml` | P7 | modified +2/−2 | no | no | yes |
| `packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml` | P7 | modified +2/−2 | no | no | yes |
| `packaging/linux/fedora-kde/sentinel-desktop.spec` | P7 | modified +30/−4 | no | no | yes |
| `packaging/linux/rpm/sentinel-desktop.spec` | P7 | modified +33/−4 | no | no | yes |
| `packaging/linux/snap/snapcraft.yaml` | P7 | modified +1/−1 | no | no | yes |
| `packaging/linux/systemd/sentinel-desktop.service` | P7 | modified +23 | no | no | yes |
| `tools/packaging/check_packaging_consistency.py` | P7 | new (148 lines) | no | no | yes |
| `docs/audits/P7_02A_FEDORA_PACKAGING_FOUNDATION_RESULT.md` | P7 | new | no | no | yes |
| `tests/CMakeLists.txt` | **P4 + P6** | modified +7 | **YES (registration only)** | no | yes |
| `docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md` | **INT7** | modified −1/+1 | no | no | correction (§8) |
| `docs/audits/INT_07_SHARED_CHECKOUT_RECOVERY_RESULT.md` | **INT7** | new (this file) | no | no | yes |
| `cfhead_check.cpp` | **none** | untracked scratch | no | no | **NO — exclude** (§7) |

**Totals.** Entry counts changed as this audit progressed; stated exactly:

| Point in time | Modified | Deleted | Untracked | Entries |
| --- | --- | --- | --- | --- |
| As found (Part A) | 29 | 1 | 11 | 41 |
| After the §4.2 privacy fix | 30 | 1 | 11 | 42 |
| Final (this deliverable added as untracked) | 30 | 1 | 12 | 43 |

The final 12 untracked entries are the 11 in §6.4 **plus** `cfhead_check.cpp` (which must not be
staged). `git diff --stat` → **31 files changed, 982 insertions(+), 110 deletions(−)** (30 modified +
the 1 deletion).

### 2.1 The only overlap

`tests/CMakeLists.txt` holds **two independent, non-adjacent hunks**:

- P6-03 (`+# P6-03: …` + `add_test(NAME test_desktop_accessibility …)` + `set_tests_properties(… TIMEOUT 60)`),
  inserted after the `test_extensions_surface` block (~line 411).
- P4-04 (`+# P4-04: read-only telemetry → policy-input adapter translation.` +
  `add_sentinel_core_test(test_resource_policy_adapter core/test_resource_policy_adapter.cpp)`),
  appended at EOF (~line 448).

The hunks do not conflict and both are present. `core/CMakeLists.txt` is P4-04 only; P2-M03
deliberately required no CMake change (documented in its report).

### 2.2 Ownership verified against the reports

- **P4-04 SHA-256 fingerprints match the report exactly** — independent proof its artefacts are
  uncorrupted: `ResourcePolicyTelemetryAdapter.h` `eacd0d34…33ce9`, `.cpp` `604fcc8b…101cb`,
  `test_resource_policy_adapter.cpp` `9c551590…790fc`.
- P2-M03: all four protocol declarations/definitions present
  (`anthropicToolDeclaration`, `buildAnthropicNativeRequestBody`, `anthropicToolResultBlocks`,
  `buildGeminiNativeRequestBody`, `geminiFunctionResponseParts`) and `ModelService.cpp` now delegates
  to them.
- P6-03: all eight QML files carry the `Accessible.*`/focus declarations; the probe exposes
  `--accessibility-smoke`; the fixture exists.
- P7-02A: two RPM specs are now **byte-identical** at `Version: 1.0.0~rc.8`; the D-Bus service file
  is deleted; the systemd unit carries the hardening directives; `ci-fedora.yml` and the packaging
  checker exist.

### 2.3 Missing / conflicting changes

- **Missing expected modifications:** **none.** Every file named in the four reports is present with
  the reported shape.
- **Unexpected modifications:** `cfhead_check.cpp` (untracked, unreferenced scratch copy of the
  certification probe **without** the P6-03 additions — see §7) and the INT-07 sanitization of the
  P7-01 audit (§8).
- **Conflicts:** **none.** No unmerged index entries; no conflict markers anywhere in the tree; the
  single overlapping file (`tests/CMakeLists.txt`) is coherent.
- **Stale-overwrite check:** no file is a regression of already-integrated code. All four change sets
  extend `cd9ec8bc`; none replaces a newer committed version with an older one.

---

## 3. Part C — Combined-state validation

### 3.1 All four implementations are physically present

Anthropic + Gemini golden request-byte tests, the resource adapter's 18 deterministic cases, the
accessibility keyboard/modal-focus changes and the Fedora packaging parity/version changes are all
present and were exercised (§4). Details: §2.2.

### 3.2 Preserved prior work (regression guards)

| Prior work | Verification | Result |
| --- | --- | --- |
| P4-02 native resource telemetry | `ResourceTelemetry.h`, adapters and suites present; `test_native_resource_telemetry`, `test_resource_monitor_sampling` pass | OK |
| P4-03.1 five corrections | present in `core/src/policy/ResourcePolicy.cpp`: `ascendingBytesSignal`/`descendingBytesSignal` (correct RSS direction), `outcome.present = inputs.powerSource.usable()`, `!std::isfinite(value) \|\| value < 0.0 \|\| value > 1.0`, `!configurationProblems_.isEmpty() \|\| !thresholdsCalibrated`, `saturatingDeadlineMs(…)` on all cooldown assignments | OK |
| P6-02 desktop shell | committed files untouched: `WindowStatePersistence.{h,cpp}`, `DesktopErrorProjection.h`, `DesktopShellViewModel.{h,cpp}`, `ApplicationBootstrapper.{h,cpp}`; `test_desktop_shell_state`, `test_desktop_shell_view_model` pass | OK |
| CI-05 Windows guards | Windows guards present in IPC/executor suites; `test_daemon_ipc*` pass | OK |

### 3.3 Every newly registered CTest target exists

| Registration | Introduced by | Target/source | Present |
| --- | --- | --- | --- |
| `test_desktop_accessibility` (CTest #138) | P6-03 | command `$<TARGET_FILE:settings_ui_certification_probe>`, offscreen, `TIMEOUT 60`; fixture `tests/helpers/accessibility_certification.qml` | yes (target #138, ran) |
| `test_resource_policy_adapter` (CTest #150) | P4-04 | `add_sentinel_core_test(test_resource_policy_adapter core/test_resource_policy_adapter.cpp)`; the macro `FATAL_ERROR`s if the source is missing | yes (ninja target + test #150, ran) |

No other task added or renamed a CTest target. `ctest -N` lists **150** tests, matching the previous
reports' target count.

**Honesty statement (required):** the previous reports' 150-target runs, and INT-07's own 150/150 run
(§4), were produced on this **combined, mixed working tree**. They are **not** four independently
validated branches, and they must not be presented as such. They demonstrate that the four change
sets co-exist and pass together; they do not attribute any individual pass to one branch.

---

## 4. Part D — Verification-gap resolution and validation evidence

Environment: macOS 27.0.1 (Darwin, arm64), Apple clang 21.0.0, CMake 4.4.4, Ninja, Qt 6.12.0
(Homebrew `/opt/homebrew/opt/qt`), qmllint 6.12.0, Python 3.15.0. Existing `build/tests` reused;
**no new worktree / no duplicate build tree**. Disk: 91 GiB available before and after.

### 4.1 `test_resource_policy_adapter` exclusion — resolved

P7-02A ran its suite with `-E test_resource_policy_adapter` because, at that moment, **another
session's in-progress adapter fixture did not compile** during a first build attempt. INT-07
re-examined this directly:

- The fixture is now complete: `tests/core/test_resource_policy_adapter.cpp` (631 lines) defines
  **18** test slots; **all 18 report case names from the P4-04 report are present**.
- It compiles: `cmake --build build/tests --target test_resource_policy_adapter` → **exit 0**.
- It runs: `test_resource_policy_adapter` → **20 passed, 0 failed** (18 cases + `initTestCase` +
  `cleanupTestCase`).

**Conclusion:** the exclusion was a transient concurrency artefact, not a defect. It is no longer
justified.

### 4.2 Personal-path scanner failure involving the P7-01 audit — investigated and fixed

`python3 tools/analysis/check_personal_paths.py` **fails on the pristine checkout** (exit 1) because
`docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md:17` records the absolute worktree path
`<personal-home>/.local/share/opencode/worktree/d272e3/audit/p7-01-roasd-integration`. Introduced by
commit `b257dce8` (P7-01); unrelated to the four change sets; **it is the second step of the Linux
`quality` job**, so `main` Linux CI cannot be green and the P7-02A packaging-consistency step that
follows it never runs.

The scanner was **not** suppressed and no assertion was removed. The root cause was fixed by
replacing the personal home-directory segment with `<personal-home>` (one line), documented before
implementation in the recovery archive. After the fix: **0 files flagged, exit 0**.

### 4.3 Deterministic test evidence (INT-07, combined tree)

| Suite | Count | Result |
| --- | --- | --- |
| `test_anthropic_messages_protocol` | **14 passed** (was 10) | pass — incl. `buildAnthropicNativeRequestBody` golden bytes `{"max_tokens":512,"messages":[{"content":"hello world","role":"user"}],"model":"claude-test"}` |
| `test_gemini_generate_content_protocol` | **13 passed** (was 10) | pass — incl. golden bytes `{"contents":[{"parts":[{"text":"hi"}],"role":"user"}],"generationConfig":{"maxOutputTokens":1024},"model":"gemini-test"}` |
| `test_native_provider_contract` | **77 passed** | pass — incl. `nativeRequestBodyByteEquivalence` (real `SelectedEndpointChatProvider` POST body, Anthropic + Gemini) |
| `test_resource_policy_adapter` | **20 passed / 0 failed** (18 cases + init/cleanup) | pass |
| `test_resource_policy` / `test_resource_policy_review` | 25 / 13 | pass (P4-03/P4-03.1 unchanged) |
| `test_desktop_accessibility` (offscreen) | exit 0 | pass |
| **Full sequential CTest** (`ctest --preset tests --output-on-failure`) | **150 / 150 passed, 0 failed, 0 skipped, 0 blacklisted**, 183.05 s | **pass** |

### 4.4 Checks

| Check | Command | Result |
| --- | --- | --- |
| Configure | `cmake --preset tests` | pass (exit 0) |
| Full build | `cmake --build build/tests -j 4` | pass (exit 0) |
| Accessibility smoke | `QT_QPA_PLATFORM=offscreen …/settings_ui_certification_probe --accessibility-smoke` | **exit 0** (checks 201–236) |
| IPC generated contracts | `python3 tools/ipc/generate.py --check` / `generate_desktop.py --check` | pass (exit 0 both) |
| Packaging consistency | `python3 tools/packaging/check_packaging_consistency.py` | pass — canonical `1.0.0-rc.8`, RPM `1.0.0~rc.8`, preset binaries `sentinel-cli, sentinel-daemon, sentinel-desktop, sentinel-plugin-host` |
| Privacy scan | `python3 tools/analysis/check_personal_paths.py` | **0 flagged** (after §4.2 fix) |
| Whitespace | `git diff --check` | clean |
| QML lint (P6-03 files) | `qmllint …` | 0 errors |
| Secret scan | regex over full diff + all untracked files (`sk-…`, `AIza…`, PEM, `password=`/`secret=`) | none |
| Conflict markers | scan of diff + untracked files | none |

### 4.5 Environmental limitations (explicit)

- **Formatting gate not exactly reproduced.** The host has `clang-format` **23.1.3**; CI pins
  **18.1.8**. As an advisory check, `clang-format-diff.py` over the changed C++ hunks produced an
  **empty** diff (no reformatting needed), but the exact CI gate must be re-run with 18.1.8 before
  merge.
- **Fedora/RPM not executed.** No `rpmbuild`/`rpmspec`/`rpmlint`/`systemd-analyze`/`snapcraft` on
  macOS; the spec/unit changes remain static. `ci-fedora.yml` has never run.
- **Accessibility is programmatic only.** No native screen reader (Orca/VoiceOver/NVDA) and no native
  KDE/Wayland rendering were exercised.
- **Live provider certification** is out of scope and was not attempted.

---

## 5. Security / privacy findings

1. **Confirmed privacy defect (fixed):** personal absolute path in the committed P7-01 audit
   (`.md:17`), a public-repo privacy leak that also blocked Linux CI. Sanitized in one line (§4.2),
   no scanner suppression.
2. **Contributor e-mail in packaging metadata:** `cmake/SentinelCPack.cmake` and both RPM specs use
   `Sentinel Team <sopwith.osdev@gmail.com>`. This is an intentional maintainer contact already
   present on `main`; **not** changed. Flagged for maintainer confirmation only.
3. **No secrets** were introduced by any change set (targeted scan clean).
4. The `bubblewrap` change adds a `Recommends`/`BuildRequires`, not a `Requires` — the sandbox remains
   fail-closed when absent. The systemd unit intentionally does **not** set `ProtectHome`/
   `ProtectSystem=strict`/`RestrictNamespaces` (would break real daemon paths and bubblewrap) — a
   documented, correct refutation of a P7-01 suggestion.
5. **No permission/approval/sandbox boundary was weakened.** The P6-03 approval overlay pins default
   focus to *Deny*; approval is not made easier. P4-04 adds no enforcement path and never calls
   `evaluate`. Boundaries from `AGENTS.md` are preserved.

---

## 6. Part E — Safe Git integration plan (maintainer procedure)

**Nothing in this section was executed by INT-07.** No branch was switched and no commit created.

### 6.1 Which base commit is correct

**`cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb`** — current `HEAD`, local `main` and `origin/main`
(identical). All four change sets were authored against it, and it contains every cross-task
dependency (all verified as ancestors of HEAD): P2-M02 (`4fd2b212`), the pre-integrated first half of
P2-M03 (`bef410b1`, `d913e0a0`), P4-02 (`a8fce657`), P4-03 / P4-03.1 (`47ea5ee5`, `ee8b1547`),
P6-02 (`32e96a8c`), CI-05 / WIN-02 (`09d4d325`, `eaa5c26d`), INT-05 (`3d59682f`), INT-06 (`1cd2268f`).
**Do not rebase onto any other base and do not use an individual feature branch as the base** — they
are indistinguishable from `main`.

### 6.2 Does current HEAD contain all required dependencies?

**Yes.** See §6.1 and §3.2. No dependency is missing and no submodule/remote fetch is required.

### 6.3 Exact maintainer steps (copy/paste)

```bash
# 0. Serialise: confirm no other agent is editing this checkout.
cd <personal-home>/Desktop/Projects/Sentinel-sanitized
git status --porcelain=v2           # must equal the INT-07 preserved inventory (§1)
git rev-parse HEAD                  # must be cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb

# 1. Optional safety net: confirm the preserved patch still reproduces the tree.
git apply --check -R --binary \
  /private/var/folders/jn/rtnh3wpd52537ppg_tls1nm40000gn/T/opencode/int-07-recovery-20261017T215059/patches/unstaged.post-fix.patch

# 2. Create the integration branch at the SAME commit (tree is unchanged; no files move).
git switch -c integration/int-07-shared-checkout-consolidation   # from cd9ec8bc

# 3. Stage EXACTLY the intended files (see 6.4). Never `git add -A` here.
#    (commands in 6.4)

# 4. Inspect what will be committed.
git diff --cached --stat
git diff --cached --name-only        # must be the 42 paths in 6.4, nothing else

# 5. One combined integration commit (see 6.5 for rationale/message).
git commit   # use the message in 6.5

# 6. Clean-tree validation BEFORE pushing (6.6).
```

### 6.4 Files that must be staged / must NOT be staged

**Must be staged (42 paths):**

```bash
git add \
  .github/workflows/ci-fedora.yml \
  .github/workflows/ci-linux.yml \
  CMakePresets.json \
  apps/sentinel-desktop/CMakeLists.txt \
  cmake/SentinelCPack.cmake \
  core/CMakeLists.txt \
  core/include/sentinel/core/policy/ResourcePolicyTelemetryAdapter.h \
  core/src/policy/ResourcePolicyTelemetryAdapter.cpp \
  core/include/sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h \
  core/src/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.cpp \
  core/include/sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h \
  core/src/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.cpp \
  core/src/model/ModelService.cpp \
  tests/CMakeLists.txt \
  tests/core/test_anthropic_messages_protocol.cpp \
  tests/core/test_gemini_generate_content_protocol.cpp \
  tests/core/test_native_provider_contract.cpp \
  tests/core/test_resource_policy_adapter.cpp \
  tests/helpers/settings_ui_certification_probe.cpp \
  tests/helpers/accessibility_certification.qml \
  ui/qml/Main.qml \
  ui/qml/components/base/SentinelButton.qml \
  ui/qml/components/base/SentinelComboBox.qml \
  ui/qml/components/base/SentinelSlider.qml \
  ui/qml/components/base/SentinelSpinBox.qml \
  ui/qml/components/base/SentinelSwitch.qml \
  ui/qml/components/base/SentinelTextField.qml \
  ui/qml/components/dialogs/SentinelOverlayModal.qml \
  packaging/linux/dbus/dev.sentinel.Sentinel.service \
  packaging/linux/dev.sentinel.Sentinel.metainfo.xml \
  packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml \
  packaging/linux/fedora-kde/sentinel-desktop.spec \
  packaging/linux/rpm/sentinel-desktop.spec \
  packaging/linux/snap/snapcraft.yaml \
  packaging/linux/systemd/sentinel-desktop.service \
  tools/packaging/check_packaging_consistency.py \
  docs/audits/P2_M03_NATIVE_PROVIDER_PROTOCOL_EXTRACTION_RESULT.md \
  docs/audits/P4_04_RESOURCE_TELEMETRY_POLICY_ADAPTER_RESULT.md \
  docs/audits/P6_03_DESKTOP_ACCESSIBILITY_RESULT.md \
  docs/audits/P7_02A_FEDORA_PACKAGING_FOUNDATION_RESULT.md \
  docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md \
  docs/audits/INT_07_SHARED_CHECKOUT_RECOVERY_RESULT.md
```

`packaging/linux/dbus/dev.sentinel.Sentinel.service` is a **deletion** — `git add` on the path
records the removal (equivalent to `git rm --cached`, but keep the working-tree deletion from the
`git add <path>` form).

**Must NOT be staged:**

- `cfhead_check.cpp` — untracked, unreferenced scratch copy of the certification probe (no
  `--accessibility-smoke`); belongs to no change set. Recommend the maintainer delete it after
  confirming no one needs it (INT-07 did not delete it).
- Anything under `build/` — already gitignored; never add build artefacts.

### 6.5 One combined commit vs splitting — recommendation

**Use one combined integration commit.** Rationale:

1. The four change sets have **no per-task commit boundary**: all four branches point at the same
   commit, and their changes exist only as uncommitted working-tree edits. There is nothing to
   "replay" per branch.
2. `tests/CMakeLists.txt` holds hunks from **two** tasks, and `docs/audits/` mixes four tasks' reports
   — a lossless split requires `git add -p` and risks dropping a hunk.
3. A single commit is atomic and trivially verifiable; it matches the INT-05/INT-06 precedent.

If the maintainer still wants per-phase traceability **inside** one commit, use `git add -p` only for
`tests/CMakeLists.txt` / `core/CMakeLists.txt` and create at most a small number of sequenced commits
on the same branch — but the default and safest option is one commit.

**Suggested commit message (traceability of the four phases):**

```
feat(integration): consolidate P2-M03, P4-04, P6-03, P7-02A (INT-07)

Recovered from a shared checkout where four concurrent tasks were performed
without committing and HEAD was switched under them. All four branches pointed
at cd9ec8bc; the changes existed only as uncommitted working-tree edits.

- P2-M03: extract Anthropic/Gemini native request serialization + tool-role
  conversion into the protocol modules; golden request-byte tests.
  See docs/audits/P2_M03_NATIVE_PROVIDER_PROTOCOL_EXTRACTION_RESULT.md
- P4-04: read-only HostResourceSnapshot -> ResourcePolicyInputs adapter +
  diagnostics side-channel; 18 deterministic cases.
  See docs/audits/P4_04_RESOURCE_TELEMETRY_POLICY_ADAPTER_RESULT.md
- P6-03: base-control + overlay-dialog accessibility and keyboard contracts;
  offscreen certification.
  See docs/audits/P6_03_DESKTOP_ACCESSIBILITY_RESULT.md
- P7-02A: Fedora packaging foundation (version/parity, %files, systemd
  hardening, D-Bus removal, Fedora CI, consistency checker).
  See docs/audits/P7_02A_FEDORA_PACKAGING_FOUNDATION_RESULT.md
- INT-07: sanitize a pre-existing personal path in the P7-01 audit so the
  Linux quality job passes; recovery evidence in
  docs/audits/INT_07_SHARED_CHECKOUT_RECOVERY_RESULT.md

Combined-tree evidence: full CTest 150/150, accessibility smoke exit 0,
IPC + privacy + packaging-consistency checks pass.
```

### 6.6 Clean-tree validation that must precede merging into `main`

On the freshly committed integration branch, from a **clean tree**:

1. `cmake --preset tests && cmake --build --preset tests && ctest --preset tests --output-on-failure`
   → expect **150/150**.
2. Formatting gate with the **CI-pinned `clang-format==18.1.8`**: `git diff origin/main...HEAD -U0 --
   apps core integrations plugins tests | clang-format-diff.py -p1` → expect empty.
3. `python3 tools/ipc/generate.py --check` and `python3 tools/ipc/generate_desktop.py --check` → pass.
4. `python3 tools/analysis/check_personal_paths.py` → 0 flagged.
5. `python3 tools/packaging/check_packaging_consistency.py` → pass.
6. `git diff --check` → clean; `qmllint` → 0 errors.
7. Run the new Fedora workflow once (`workflow_dispatch` on `ci-fedora.yml`) and address
   `continue-on-error` fallout before removing the advisory markers.

### 6.7 How the maintainer verifies GitHub CI afterward

1. Push the integration branch (not `main`), open a PR to `main`.
2. Confirm the PR changed-file list equals the 42 staged paths (§6.4) — in particular that
   `cfhead_check.cpp` is **absent**.
3. Watch Actions: `ci-linux` (the **quality** job must now reach and pass the new packaging
   consistency step — this is the direct proof the P7-01 sanitization unblocked it), the Ubuntu
   build/test jobs, `ci-fedora.yml` (first run; new/advisory), the macOS/Windows/Rust workflows.
4. Only merge after a green CI run **with the pinned formatter** and after the Fedora workflow has
   been validated (or its advisory status explicitly accepted).

---

## 7. Confirmed defects

| # | Defect | Scope | Status |
| --- | --- | --- | --- |
| D1 | Personal absolute path in `docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md:17` breaks `check_personal_paths.py` and, therefore, the Linux `quality` job (and the packaging step after it) | pre-existing (commit `b257dce8`), not one of the four tasks | **fixed** (one-line sanitization, §4.2), reversible |
| D2 | `cfhead_check.cpp` — untracked, unreferenced 1091-line scratch copy of the certification probe, missing the P6-03 additions | belongs to no change set | **reported, not removed**; must not be staged |
| D3 | P7-02A ran its suite with `-E test_resource_policy_adapter` | transient concurrency artefact | **resolved** — fixture compiles and passes (§4.1) |
| D4 | `clang-format` gate not reproducible on the host (23.1.3 vs pinned 18.1.8) | environment | advisory empty diff; must be re-run in CI |

No defect was found in the four change sets themselves: all are present, internally consistent and
mutually compatible.

---

## 8. Readiness verdict

**Integration-ready, pending maintainer approval.**

- All four implementations are **physically present and mutually compatible**; no missing or
  conflicting change was found.
- The **combined** tree passes the complete suite (**150/150**), the accessibility smoke, IPC
  contracts, the packaging-consistency checker and the privacy scan; whitespace and conflict scans
  are clean.
- The two Part-D gaps were closed: the P4-04 adapter fixture **compiles and passes**
  (exclusion no longer justified), and the personal-path CI blocker is **fixed at the root**.
- Because the four change sets were never committed on separate branches, the evidence is a
  **combined-tree** result only. Individual-branch validation does not exist and must not be
  claimed.
- **Requires maintainer approval:** creating the integration branch and the single combined commit;
  optionally deleting `cfhead_check.cpp`; confirming the contributor e-mail in packaging metadata;
  and running the pinned-format/Fedora gates before merge. None of these was performed by INT-07.

---

## 9. Scope statement

INT-07 preserved the shared checkout, verified the four untracked change sets against their reports
and against the committed baseline `cd9ec8bc`, applied exactly one documented one-line privacy
correction, and produced this procedure. It performed **no** commit, push, merge, reset, clean,
stash, checkout, switch, rebase, cherry-pick or worktree operation. All original bytes are recoverable
from the recovery directory in §1.
