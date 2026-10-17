# INT-08 — Final Integration Verification & Commit Preparation (Result)

Audit date: 2026-10-17. Phase: cross-cutting integration verification.
Priority: Critical. Mode: single-repository verification and commit preparation.

Task: independently re-verify the INT-07 shared-checkout recovery, re-run the pinned
quality gates against the current tree, and produce an exact, safe procedure for a
maintainer to create one integration commit for the four recovered change sets:

- **P2-M03** — Anthropic/Gemini native protocol extraction.
- **P4-04** — resource telemetry → policy adapter.
- **P6-03** — Qt/QML desktop accessibility.
- **P7-02A** — Fedora packaging foundation.

**No repository mutation of the working change sets was committed, pushed, merged,
rebased, reset, stashed, cleaned or checked-out.** No branch was switched. No CI
workflow was dispatched. Two **documented, reversible, mechanical corrections** were
applied to the working tree so the proposed commit can pass the pinned CI gates (see
§3); originals are preserved in a recovery directory (§1.2).

---

## 0. Result at a glance

| Item | Value |
| --- | --- |
| Combined implementation ready for a maintainer-controlled commit? | **Yes — locally verified** |
| Hosted CI acceptance | **Not yet proven** (unpushed; `ci-fedora.yml` never run) |
| Full CTest (final tree) | **150 / 150 passed, 0 failed, 0 skipped** (167.01 s) |
| CI-pinned `clang-format==18.1.8` gate | **Clean after correction** (failed before — §3.2) |
| Privacy path scanner | **0 flagged after correction** (failed before — §3.1) |
| 42-path inventory | **All present, expected shapes** (§2) |
| Recovery backup | **Intact — 69/69 SHA-256 verified** (§1.2) |

Two blockers that INT-07 missed were found and closed; both are covered in §3 and §5.

---

## 1. Part A — Verified current state

### 1.1 Git identities and working tree

| Item | Value |
| --- | --- |
| Repository root | `<personal-home>/Desktop/Projects/Sentinel-sanitized` |
| `git rev-parse HEAD` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| HEAD subject | `test(cmake): drop duplicated P4-03 test registrations introduced by integration merge` |
| Checked-out branch | `feature/p4-04-resource-policy-adapter` (name is **not** authoritative — §1.1.1) |
| Local `main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| `origin/main` | `cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb` |
| `main` vs `origin/main` | `0 ahead / 0 behind` (`git rev-list --left-right --count main...origin/main` → `0  0`) |
| Git version | 2.56.0 |
| Staged | **0** |
| Unstaged modified | **30** |
| Unstaged deleted | **1** (`packaging/linux/dbus/dev.sentinel.Sentinel.service`) |
| Untracked (non-ignored) | **12** = 11 INT-07 manifest new files + `cfhead_check.cpp` (becomes **13** once this INT-08 deliverable is written; it is not part of the 42-path manifest) |
| Unmerged | **0** |
| `git diff --stat` | **31 files changed, 982 insertions(+), 110 deletions(−)** |
| Disk available | **91 GiB** (`df -h .` → 228 GiB / 112 GiB used / 91 GiB avail, 56%) |

`git status` is byte-for-byte consistent with the INT-07 §0 inventory (same 30 modified,
1 deleted, 0 staged, 0 unmerged). **No missing files, no unexpected concurrent edits, no
Git divergence.** Recovery is **not** uncertain; the tree may be prepared for commit.

#### 1.1.1 Branch naming is not authoritative

Every local branch still points at `cd9ec8bc` (reflog-documented concurrent switching from
INT-07 §0.1). The four change sets exist **only as uncommitted working-tree edits**, so the
current branch name carries no ownership or content information. The evidence in this report
is therefore a **combined-tree** result only.

### 1.2 Recovery backup verification (not relying on a single copy)

INT-07's recovery directory (under the approved opencode temp root,
`.../opencode/int-07-recovery-20261017T215059`) is **present and intact**:

- `shasum -a 256 -c SHA256SUMS` → **69/69 files OK, 0 FAILED** (includes the full `tree/`
  byte backup of 42 files, `deleted-originals/`, all patches and metadata).
- `tree/` contains 42 files; `deleted-originals/` contains the deleted D-Bus service.
- `patches/unstaged.final.patch` is byte-identical to `unstaged.post-fix.patch`.
- `SHA256SUMS.worktree` lists the **pre-fix (HEAD)** hash of the P7-01 audit
  (`8c834d0c…`); it no longer matches the working tree **by design** — that manifest
  recorded the file *as found*, i.e. before the INT-07 §8 sanitization. The `tree/` copy
  matches the sanitized working tree, and the main `SHA256SUMS` (69/69) verifies the whole
  archive.

**Caveat reported honestly:** the only recovery archive is the **temporary-directory**
backup. Temp directories are volatile. The maintainer should treat it as valuable and, if
desired, copy it somewhere durable before acting. (The INT-07 `unstaged.post-fix.patch`
still reproduces the INT-07-certified tree; after the two INT-08 corrections it now fails to
reverse-apply cleanly, which is expected — see §3.3.)

---

## 2. Part B — File inventory (exact INT-07 §6.4 manifest)

All **42** manifest paths were verified against the current working tree; **every path is
present in the expected state**. Full manifest is reproduced copy-ready in §7.1.

| Group | Paths | State verified |
| --- | --- | --- |
| CI workflows | `.github/workflows/ci-fedora.yml`, `.github/workflows/ci-linux.yml` | `??` new / ` M` +2 |
| Build/preset | `CMakePresets.json`, `apps/sentinel-desktop/CMakeLists.txt`, `cmake/SentinelCPack.cmake`, `core/CMakeLists.txt`, `tests/CMakeLists.txt` | modified as inventoried |
| P2-M03 | 4 protocol `.h/.cpp` + `core/src/model/ModelService.cpp` | modified |
| P2-M03 tests | `tests/core/test_anthropic_messages_protocol.cpp`, `test_gemini_generate_content_protocol.cpp`, `test_native_provider_contract.cpp` | modified |
| P4-04 | `ResourcePolicyTelemetryAdapter.h/.cpp`, `tests/core/test_resource_policy_adapter.cpp` | `??` new |
| P6-03 | `Main.qml` + 6 base controls + `SentinelOverlayModal.qml` + `settings_ui_certification_probe.cpp` + `accessibility_certification.qml` | modified / new |
| P7-02A packaging | 2 RPM specs, 2 metainfo, snapcraft, systemd unit, deleted D-Bus service, `tools/packaging/check_packaging_consistency.py` | modified / new / deleted |
| Audit reports | `P2_M03…`, `P4_04…`, `P6_03…`, `P7_02A…`, `P7_01…`, `INT_07…` | new / modified |

Cross-checks:

- **P4-04 integrity fingerprints match the report exactly** (independent proof of
  uncorrupted artifacts):
  `ResourcePolicyTelemetryAdapter.h` `eacd0d34…33ce9`, `.cpp` `604fcc8b…101cb`,
  `test_resource_policy_adapter.cpp` `9c551590…790fc`.
- **Intentional one-line privacy correction in the P7-01 report remains present**
  (`docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md:17` →
  `<personal-home>/.local/share/opencode/worktree/d272e3/audit/p7-01-roasd-integration`).
- **No API credentials, personal paths, build artifacts or temporary files** enter the
  proposed commit (credential scan clean; `build*/` ignored; only the 43 tracked/untracked
  entries above exist). One personal-path defect *was* present and is fixed — §3.1.

### 2.1 Excluded files

| File | Reason |
| --- | --- |
| `cfhead_check.cpp` | Untracked, unreferenced 1091-line scratch copy of the certification probe **without** the P6-03 additions. Belongs to no change set. **Not staged. Not deleted** (pending maintainer approval). |
| `build/`, `build-copr/`, any `*.o`/`*.pch`/logs | Already `.gitignore`d; never added. |
| `docs/audits/INT_08_FINAL_INTEGRATION_VERIFICATION_RESULT.md` | This INT-08 deliverable. It is **not** part of the 42-path manifest (INT-08 §7.1) and must be committed separately by the maintainer if wanted. |

---

## 3. INT-08 corrections (documented, reversible)

INT-07's §4.4 recorded a clean privacy scan and an advisory "empty" formatting diff.
Re-verification on the current tree found **both claims no longer hold**, and both would have
failed CI. Two mechanical corrections were applied and are backed up.

### 3.1 Correction C1 — personal paths in the INT-07 report (privacy gate blocker)

`tools/analysis/check_personal_paths.py` (the **second** step of the Linux `quality` job,
immediately after Checkout) **failed on the tree INT-07 certified**, because the INT-07
deliverable itself — one of the 42 manifest paths — contained three `/Users/<user>/…`
occurrences:

| Line | Before | After |
| --- | --- | --- |
| 27 | `<personal-home>/Desktop/Projects/Sentinel-sanitized` (literal home) | `<personal-home>/Desktop/Projects/Sentinel-sanitized` |
| 272 | quoted leaked worktree path (literal home) | quoted with `<personal-home>` |
| 363 | `cd` example (literal home) | `cd` example with `<personal-home>` |

This is the same class of defect INT-07 §8 fixed in the P7-01 report (INT-07 simply ran the
scanner before its own deliverable was final). The fix replaces only the home-directory
segment with a non-personal placeholder; it **does not** suppress the scanner or remove any
assertion. The original file is preserved at
`.../opencode/int08-recovery/INT_07_SHARED_CHECKOUT_RECOVERY_RESULT.md.orig`
(sha256 `f5583aa2…914c2`).

**After:** `check_personal_paths.py` → **0 files flagged, exit 0**.

### 3.2 Correction C2 — pinned-format violation in the P6-03 probe (format gate blocker)

The CI `quality` job formats the changed hunks with the **pinned `clang-format==18.1.8`**
(`.github/workflows/ci-linux.yml`: `git diff -U0 "$base_ref" HEAD -- apps core integrations
plugins tests | clang-format-diff.py -p1`, then fails on any non-empty output).

Running the **pinned 18.1.8** over the working-tree changes produced a **non-empty diff** on
`tests/helpers/settings_ui_certification_probe.cpp` — the `extensionsSmoke ? …` ternary
added by P6-03 was aligned with the spacing of a newer clang-format:

```
-        extensionsSmoke      ? QFileInfo(QStringLiteral(CERTIFICATION_QML_WRAPPER))
-                                   .dir()
-                                   .filePath("extensions_hub_certification.qml")
+        extensionsSmoke ? QFileInfo(QStringLiteral(CERTIFICATION_QML_WRAPPER))
+                              .dir()
+                              .filePath("extensions_hub_certification.qml")
```

**Root cause of INT-07's false negative:** INT-07 §4.5 ran the advisory check with the
host's **clang-format 23.1.3**. Confirmed here: 23.1.3 → **empty**, 18.1.8 → **non-empty**.
The pinned CI version therefore fails; the gate was **not** reproducible on the host in
INT-07 and is now run for real.

The correction was applied **only with the pinned binary** (`clang-format-diff.py -i
-binary <18.1.8>`), touching only the three changed lines (whitespace/wrapping). The
original file is preserved at `.../opencode/int08-recovery/settings_ui_certification_probe.cpp.orig`
(sha256 `5fe229af…9ae9f`).

**After:** `git diff -U0 … | clang-format-diff.py -p1` (pinned 18.1.8) → **empty**.

P6-03's own report (§5.6) already noted that the pinned formatter re-wraps this region; this
report converts that note into the actual applied, CI-matching correction.

### 3.3 Effect on INT-07's recovery patch

After C1/C2 the INT-07 `unstaged.post-fix.patch` no longer reverse-applies to the working
tree (`tests/helpers/settings_ui_certification_probe.cpp` differs). This is expected: C1/C2
are INT-08 changes layered on top of the INT-07-certified state. The INT-07 state remains
fully recoverable from the archive in §1.2.

---

## 4. Part C — Integration tests (actual numbers, re-run)

Environment: macOS (Darwin, arm64), Apple clang, CMake 4.4.4, Ninja, Qt 6.12.0 (Homebrew),
qmllint 6.12.0, Python 3.15.0. Existing `build/tests` reused; **no new worktree / no
duplicate build directory**.

| Check | Command | Result |
| --- | --- | --- |
| Configure | `cmake --preset tests` | pass (exit 0) |
| Build | `cmake --build --preset tests` | pass (exit 0) |
| **CTest discovered** | `ctest --preset tests -N` | **150** targets |
| **Full CTest (final tree)** | `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure` | **150 / 150 passed, 0 failed, 0 skipped, 0 blacklisted**, 167.01 s |
| Accessibility smoke | `settings_ui_certification_probe --accessibility-smoke` (offscreen) | exit 0 |

**Focused suites (run directly):**

| Suite | Passed |
| --- | --- |
| `test_anthropic_messages_protocol` | **14** |
| `test_gemini_generate_content_protocol` | **13** |
| `test_native_provider_contract` | **77** |
| `test_resource_policy_adapter` | **20** (18 cases + init/cleanup) |
| `test_resource_policy` | 25 |
| `test_resource_policy_review` | 13 |

The 20 resource-adapter cases and the Anthropic/Gemini characterization counts match the
INT-07 baseline; the full 150/150 was **reproduced twice** (pre-correction 166.36 s; final
tree 167.01 s).

**Rust (unaffected by the four C++ change sets; regression guard only), toolchain pinned
`1.94.0` as in `ci-rust.yml`:**

| Check | Command | Result |
| --- | --- | --- |
| Formatting | `cargo +1.94.0 fmt --all --check` | exit 0 |
| Clippy | `cargo +1.94.0 clippy --locked --all-targets -- -D warnings` | exit 0 |
| Tests | `cargo +1.94.0 test --locked` | exit 0 — **142 passed / 0 failed** |

---

## 5. Part D — Mandatory quality gates

| Gate | Command / tool | Result |
| --- | --- | --- |
| **CI-pinned clang-format** | `clang-format-diff.py -p1` with **18.1.8** | **FAIL before C2, CLEAN after C2** (§3.2) |
| Scoped clang-tidy | LLVM 23 over changed `core/**` files (Apple PCH stripped) | **exit 0** — no errors; only pre-existing `readability-braces-around-statements` and one `performance-unnecessary-value-param` on `ModelService.cpp`, matching the P2-M03 report. `.clang-tidy` sets `WarningsAsErrors: ''` and CI does not pass `-warnings-as-errors`. |
| IPC generated contracts | `tools/ipc/generate.py --check`, `generate_desktop.py --check` | pass (exit 0 both) |
| Privacy path scanner | `tools/analysis/check_personal_paths.py` | **0 flagged after C1** (§3.1) |
| Credential scanner | regex (`sk-…`, `AIza…`, PEM, `password=`/`secret=`/`token=`/`api_key=`) over full diff + all untracked files | **none** |
| Packaging consistency | `tools/packaging/check_packaging_consistency.py` | pass — canonical `1.0.0-rc.8`, RPM `1.0.0~rc.8`, preset binaries `sentinel-cli, sentinel-daemon, sentinel-desktop, sentinel-plugin-host` |
| AppStream metadata | `appstreamcli validate --no-net packaging/linux/dev.sentinel.Sentinel.metainfo.xml` | success (`infos: 2, pedantic: 1`) |
| Desktop entry | `desktop-file-validate packaging/linux/dev.sentinel.Sentinel.desktop` | pass (exit 0) |
| Whitespace | `git diff --check` (unstaged + staged) | clean |
| QML lint (baseline guard) | `qmllint -I ui/qml -I build/tests/apps/sentinel-desktop $(find ui/qml -name '*.qml')` | 0 errors, 113 warnings (identical to the P6-03 baseline) |

**Pinned-format statement (required):** the CI-pinned **clang-format 18.1.8** was used; it
was **not** substituted with the host's 23.1.3. The wheel was obtained and run from a
temporary directory (`clang-format 18.1.8` confirmed by `--version`), so the gate is
**verified**, not merely asserted. The gate was **failing** on the INT-07 tree and is clean
only after correction C2.

---

## 6. Part E — Fedora CI review (`ci-fedora.yml`)

`ci-fedora.yml` is new (P7-02A). YAML parses cleanly. Structure:

| Job | Runner | Container | Steps | Advisory steps |
| --- | --- | --- | --- | --- |
| `packaging-metadata` | ubuntu-latest | `registry.fedoraproject.org/fedora:44` | 10 | rpmlint lint; `systemd-analyze verify` |
| `build-package` | ubuntu-latest | fedora:44 | 7 | *none* (all blocking) |
| `test` | ubuntu-latest | fedora:44 | 6 | CTest run |
| `rust-feasibility` | ubuntu-latest | fedora:44 | 3 | toolchain probe |

- **Job dependencies:** there are **no** `needs:` edges — the four jobs are independent and
  run in parallel. This is internally consistent (each bootstraps its own container), but a
  maintainer should confirm that is the intent rather than an omission.
- **Package dependencies:** `packaging-metadata` installs `git tar gzip`, then `rpm-build
  desktop-file-utils appstream systemd` (matches the tools it invokes: `rpmspec`,
  `desktop-file-validate`, `appstreamcli`, `systemd-analyze`). `build-package` installs
  `cmake ninja-build gcc-c++` + Qt 6 dev packages + `mesa-libGL-devel vulkan-loader-devel`.
  `test` adds `bubblewrap google-noto-sans-fonts`.
- **Packaging-consistency step** runs `check_packaging_consistency.py` first — passing on
  the host here; it is version/parity/`%files`-binary driven and distro-independent.
- **Release-artifact expectations:** `build-package` stages `cmake --install
  build/package-ready --prefix "$PWD/dist"` and asserts the four binaries, desktop, metainfo,
  scalable SVG, systemd unit, config template, `share/sentinel/core-warden`,
  `share/licenses/sentinel`, and PNG icons at 16/22/24/32/48/64/128/256/512. These match the
  CMake install rules reviewed (`CMAKE_INSTALL_DOCDIR` = `share/doc/Sentinel` is also shipped
  in the RPM `%files` via the `LICENSE`/`README.md` install rules).
- **Source RPM:** `packaging/linux/fedora-kde/build_copr.sh` exists and is invoked by the
  non-advisory `Build source RPM` step; it builds a `git archive` tarball named to match
  `%autosetup -n %{name}-%{version}` and runs `rpmbuild -bs` (no `copr-cli` needed unless
  `--submit`). It does not perform a full `rpmbuild -bb`, so `%files` file-existence is not
  exercised in CI.
- **Privacy scanner no longer blocks Linux quality checks:** in `ci-linux.yml` the privacy
  step is the first validation step, *before* the newly added packaging-consistency step. On
  the *uncorrected* tree it exited 1 and stopped the job (so the packaging step never ran);
  after correction C1 it passes, unblocking the whole `quality` job. **Confirmed on the host.**
- **`ci-linux.yml` change** is a single +2: the packaging-consistency step inserted directly
  after the privacy step, exactly as documented.

**Not performed / not claimed:** no `workflow_dispatch`, no hosted run, no native Fedora 44
execution, no Ro-ASD image/repository/signing/acceptance. `ci-fedora.yml` has **never run**.

### 6.1 Unresolved packaging / release-policy decisions

1. `%files` completeness is only checked by SRPM creation; a full `rpmbuild -bb` /
   `rpmlint` pass on Fedora is still required.
2. `bubblewrap` is a `Recommends`/`BuildRequires`, not `Requires` — the sandbox stays
   fail-closed; confirm this matches product policy.
3. The systemd unit intentionally omits `ProtectHome` / `ProtectSystem=strict` /
   `RestrictNamespaces` (would break daemon paths and bubblewrap); treat as a documented
   decision.
4. The Rust feasibility job is advisory — no Rust RPM is produced and the Fedora
   distribution toolchain may be older than `cli/Cargo.toml`'s `rust-version = 1.94`.
5. Contributor contact `Sentinel Team <sopwith.osdev@gmail.com>` remains in CPack/spec
   metadata (pre-existing on `main`); maintainer confirmation only.

---

## 7. Part F — Safe Git integration preparation

### 7.1 Is branch creation at the current HEAD safe? — **Yes**

`HEAD = main = origin/main = cd9ec8bc`, working tree dirty, 0 staged, 0 unmerged. Creating a
new branch **at the current HEAD** does not move any file (the new branch points at the same
tree). **However, do not run `git switch` while the tree is dirty unless the switch cannot
move files** — the commands below create the branch from `cd9ec8bc`, which is exactly current
HEAD, so no working-tree file changes. **These commands were not executed by INT-08.**

`integration/int-07-shared-checkout-consolidation` does **not** exist yet.

### 7.2 Copy-ready maintainer commands

Run from the repository root (set the path without hard-coding a personal directory):

```bash
cd "$(git rev-parse --show-toplevel)"

# 0. Confirm no other agent is editing this checkout, and the base is unchanged.
git status --porcelain=v2                 # expect: 0 staged; 30 M + 1 D + 12 ?? (incl. cfhead_check.cpp)
git rev-parse HEAD                        # expect: cd9ec8bcbfd250eedb5e237eb33fde55ab91dddb
git rev-list --left-right --count main...origin/main   # expect: 0   0

# 0b. Confirm the two INT-08 corrections are present in the working tree.
python3 tools/analysis/check_personal_paths.py            # expect: 0 file(s) flagged
python3 tools/packaging/check_packaging_consistency.py    # expect: packaging metadata is consistent
#     (format gate, optional re-check)
git diff -U0 --no-color HEAD -- apps core integrations plugins tests \
  | clang-format-diff.py -p1    # with clang-format==18.1.8 on PATH; expect empty

# 1. Create the integration branch at the SAME commit (no files move).
git switch -c integration/int-07-shared-checkout-consolidation

# 2. Stage EXACTLY the 42 approved paths. NEVER `git add -A` here.
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

# 3. Inspect the staged set — MUST be exactly the 42 paths, cfhead_check.cpp ABSENT.
git diff --cached --name-only | sort | diff - <(printf '%s\n' \
  .github/workflows/ci-fedora.yml .github/workflows/ci-linux.yml CMakePresets.json \
  apps/sentinel-desktop/CMakeLists.txt cmake/SentinelCPack.cmake core/CMakeLists.txt \
  core/include/sentinel/core/policy/ResourcePolicyTelemetryAdapter.h \
  core/src/policy/ResourcePolicyTelemetryAdapter.cpp \
  core/include/sentinel/core/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.h \
  core/src/provider/protocols/anthropic_messages/AnthropicMessagesProtocol.cpp \
  core/include/sentinel/core/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.h \
  core/src/provider/protocols/gemini_generate_content/GeminiGenerateContentProtocol.cpp \
  core/src/model/ModelService.cpp tests/CMakeLists.txt \
  tests/core/test_anthropic_messages_protocol.cpp tests/core/test_gemini_generate_content_protocol.cpp \
  tests/core/test_native_provider_contract.cpp tests/core/test_resource_policy_adapter.cpp \
  tests/helpers/settings_ui_certification_probe.cpp tests/helpers/accessibility_certification.qml \
  ui/qml/Main.qml ui/qml/components/base/SentinelButton.qml ui/qml/components/base/SentinelComboBox.qml \
  ui/qml/components/base/SentinelSlider.qml ui/qml/components/base/SentinelSpinBox.qml \
  ui/qml/components/base/SentinelSwitch.qml ui/qml/components/base/SentinelTextField.qml \
  ui/qml/components/dialogs/SentinelOverlayModal.qml packaging/linux/dbus/dev.sentinel.Sentinel.service \
  packaging/linux/dev.sentinel.Sentinel.metainfo.xml packaging/linux/fedora-kde/dev.sentinel.Sentinel.metainfo.xml \
  packaging/linux/fedora-kde/sentinel-desktop.spec packaging/linux/rpm/sentinel-desktop.spec \
  packaging/linux/snap/snapcraft.yaml packaging/linux/systemd/sentinel-desktop.service \
  tools/packaging/check_packaging_consistency.py \
  docs/audits/P2_M03_NATIVE_PROVIDER_PROTOCOL_EXTRACTION_RESULT.md \
  docs/audits/P4_04_RESOURCE_TELEMETRY_POLICY_ADAPTER_RESULT.md \
  docs/audits/P6_03_DESKTOP_ACCESSIBILITY_RESULT.md \
  docs/audits/P7_02A_FEDORA_PACKAGING_FOUNDATION_RESULT.md \
  docs/audits/P7_01_ROASD_DISTRIBUTION_INTEGRATION_READINESS.md \
  docs/audits/INT_07_SHARED_CHECKOUT_RECOVERY_RESULT.md)   # expect: no output (identical)

git diff --cached --stat
git diff --cached --check                 # expect: clean

# 4. One combined integration commit (message in §7.3).
git commit    # paste the message from §7.3

# 5. (Optional) also commit this INT-08 report separately, if wanted:
#    git add docs/audits/INT_08_FINAL_INTEGRATION_VERIFICATION_RESULT.md && git commit -m "docs(int-08): final integration verification result"

# 6. Push the integration branch (NOT main) and open a PR to main.
git push -u origin integration/int-07-shared-checkout-consolidation
```

`packaging/linux/dbus/dev.sentinel.Sentinel.service` is a **deletion**: `git add` on the
path records the removal.

### 7.3 Suggested commit message

```
feat(integration): consolidate P2-M03, P4-04, P6-03, P7-02A (INT-08)

Recovered from a shared checkout where four concurrent tasks were performed
without committing and HEAD was switched under them. All branches pointed at
cd9ec8bc; the changes existed only as uncommitted working-tree edits.

- P2-M03: extract Anthropic/Gemini native request serialization + tool-role
  conversion into the protocol modules; golden request-byte tests.
- P4-04: read-only HostResourceSnapshot -> ResourcePolicyInputs adapter +
  diagnostics side-channel; 18 deterministic cases.
- P6-03: base-control + overlay-dialog accessibility and keyboard contracts;
  offscreen certification.
- P7-02A: Fedora packaging foundation (version/parity, %files, systemd
  hardening, D-Bus removal, Fedora CI, consistency checker).
- INT-07: sanitize a pre-existing personal path in the P7-01 audit.
- INT-08: sanitize personal paths introduced in the INT-07 report (privacy gate)
  and reformat the P6-03 probe with the CI-pinned clang-format 18.1.8
  (format gate). See docs/audits/INT_08_FINAL_INTEGRATION_VERIFICATION_RESULT.md

Combined-tree evidence: full CTest 150/150, accessibility smoke exit 0,
IPC + privacy + packaging-consistency + AppStream/desktop validation pass,
pinned clang-format 18.1.8 clean.
```

### 7.4 Verify hosted CI after pushing

1. Confirm the PR changed-file list equals the 42 staged paths — in particular
   `cfhead_check.cpp` is **absent**.
2. `ci-linux` → the `quality` job must now pass the privacy step **and** reach the new
   packaging-consistency step; the pinned-format step must be green.
3. Ubuntu `build-test` / `asan-build-test`; `ci-fedora` (first run — advisory steps may
   still surface fallout); macOS/Windows/`ci-rust` workflows.
4. Merge only after a green run **with the pinned formatter** and after the Fedora workflow
   is validated (or its advisory status explicitly accepted).

---

## 8. Security / privacy findings

1. **Fixed (C1):** the INT-07 deliverable contained three personal home-directory paths and
   would have failed the Linux `quality` job's privacy step — itself a public-repo privacy
   leak. Sanitized with `<personal-home>`; scanner not suppressed; original preserved.
2. **Fixed (C2):** the P6-03 probe did not satisfy the CI-pinned `clang-format 18.1.8` and
   would have failed the format step. Reformatting only; no logic change.
3. **No secrets/credentials** were introduced by any change set (targeted scan clean).
4. **No boundary weakened:** P6-03's approval overlay still pins default focus to *Deny*;
   P4-04 adds no enforcement path and never calls `evaluate`; the sandbox stays fail-closed
   (`bubblewrap` is `Recommends`, not `Requires`).
5. **`cfhead_check.cpp`** remains untracked and must not be committed; deletion only on
   maintainer approval.

---

## 9. Remaining Fedora / platform verification gaps

- **No native Fedora 44 run.** `ci-fedora.yml` has never executed; no `rpmbuild -bb`,
  `rpmlint`, `systemd-analyze`, `snapcraft` or `appstreamcli` on Fedora was performed here.
- **No Ro-ASD acceptance.** No Ro-ASD image, repository, signing or acceptance service was
  involved; a green Fedora container is not Ro-ASD compatibility.
- **%files completeness** is not exercised by the SRPM-only CI path.
- **Accessibility is programmatic only.** No Orca/VoiceOver/NVDA and no native KDE/Wayland
  rendering.
- **Live provider certification** (Anthropic/Gemini) was not attempted.
- **Hosted CI acceptance is pending** for all platforms.

---

## 10. Readiness verdict

**Ready for a maintainer-controlled integration commit — locally verified; hosted CI
acceptance still pending.**

- The recovered four change sets are **physically present and mutually compatible**; no
  missing or conflicting change was found, and the 42-path inventory is exact.
- The **combined final tree passes the full suite (150/150, 0 skipped)**, the accessibility
  smoke, IPC contracts, the packaging-consistency checker, the privacy scan (after C1),
  AppStream and desktop-file validation, `git diff --check`, and the **CI-pinned
  clang-format 18.1.8** gate (after C2). Rust fmt/clippy/tests are green on the pinned 1.94.0.
- **INT-07's headline results were stale in two places.** Its privacy scan predated its own
  deliverable (personal paths), and its "empty" format diff used clang-format 23.1.3 rather
  than the pinned 18.1.8. Both are corrected here with reversibility.
- Evidence is a **combined-tree** result only; per-branch validation does not exist and is
  not claimed. `ci-fedora.yml` and all hosted CI remain unverified until pushed.
- **Requires maintainer action:** review C1/C2; create the integration branch and the single
  combined commit; optionally delete `cfhead_check.cpp`; confirm the packaging contact
  e-mail and the Fedora advisory markers; run the Fedora workflow and the hosted CI before
  merging.

---

## 11. Scope statement

INT-08 independently re-verified the INT-07 shared checkout, confirmed the recovery archive
and the 42-path inventory, re-ran the full test and quality-gate matrix with the pinned
toolchain, applied two documented and reversible corrections so the proposed commit can pass
the pinned CI gates, reviewed `ci-fedora.yml`, and produced a reviewed staging manifest.

It performed **no** commit, push, merge, reset, clean, stash, checkout, switch, rebase,
cherry-pick, workflow dispatch or additional worktree. All original bytes are recoverable
from the INT-07 archive (§1.2) and the INT-08 backups (§3).
