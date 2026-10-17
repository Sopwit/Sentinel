# P1-05.1 — Portable Legacy CLI Test Isolation Remediation Result

Audit date: 2026-10-17 (Europe/Istanbul).
Review worktree: `<REDACTED_LOCAL_PATH>/Sentinel-sanitized`
Branch: `review/p1-05-current-main`
HEAD: `29fdeab9b1326afad4072618272ff9fe93f4c567` (`origin/review/p1-05-current-main`)
Related acceptance: AC-P1-07
Related pending decision: PD-D05

---

## 1. Executive Summary

This remediation resolves the cross-platform test-environment isolation failure in `tests/integration/test_legacy_cli_exit_codes.cpp` that previously prevented the 20-case Legacy C++ CLI exit-code suite from executing under the normal macOS CTest environment without external workarounds.

Prior to this remediation, running `ctest -R test_legacy_cli_exit_codes` on macOS failed immediately in `initTestCase()` before executing any test cases, because the test suite's bootstrap assumed Linux-specific XDG environment behavior (`XDG_CONFIG_HOME`) and lacked macOS CoreFoundation home-directory routing (`CFFIXED_USER_HOME`). The suite previously required an external environment override (`CFFIXED_USER_HOME=/... ctest`) to run.

With this remediation:
- The suite executes all 20 test cases out-of-the-box under normal CTest without any external environment variables or wrappers.
- The parent process global environment (`environ`) is never modified; all overrides are strictly scoped to child `QProcessEnvironment` instances.
- Profile and settings isolation are enforced portably across macOS, Linux, and Windows.
- No production CLI code was modified; all 20 exit-code contract assertions remain intact without skips, retries, or weakened verifications.
- The complete Sentinel test suite passes at **137/137 tests (100%)**.

---

## 2. Reproduced Bootstrap Failure

### Invocations and Output
Invoking the test suite under the normal macOS CTest environment (`ctest --test-dir build/tests -R test_legacy_cli_exit_codes --output-on-failure`) reproduced the bootstrap failure:

```text
Test project <REDACTED_LOCAL_PATH>/Sentinel-sanitized/build/tests
    Start 118: test_legacy_cli_exit_codes
1/1 Test #118: test_legacy_cli_exit_codes .......***Failed    0.31 sec
********* Start testing of LegacyCliExitCodesTest *********
Config: Using QtTest library 6.12.0, Qt 6.12.0 (arm64-little_endian-lp64 shared (dynamic) release build; by Apple LLVM 21.0.0 (clang-2100.3.34.2)), macos 27.0.1
FAIL!  : LegacyCliExitCodesTest::initTestCase() 'settingsFile.startsWith(configRoot)' returned FALSE.
        (settings file is not rooted at XDG_CONFIG_HOME; test isolation would be invalid)
   Loc: [<REDACTED_LOCAL_PATH>/Sentinel-sanitized/tests/integration/test_legacy_cli_exit_codes.cpp(287)]
PASS   : LegacyCliExitCodesTest::cleanupTestCase()
Totals: 1 passed, 1 failed, 0 skipped, 0 blacklisted, 160ms
********* Finished testing of LegacyCliExitCodesTest *********

0% tests passed, 1 tests failed out of 1
```

### Analysis of the Failure
1. **0 of 20 test cases executed**: The failure occurred during fixture initialization (`initTestCase()`), aborting test case execution.
2. **Real user profile exposure risk**: Because `probe` ran without macOS CoreFoundation home isolation, `sentinel-cli status` printed:
   `Settings File: <REDACTED_USER_HOME>/Library/Preferences/sentinel-cli/settings.json`
   pointing directly to the real user's preference folder rather than the temporary `probeDir`.
3. **Fixture bootstrap defect**: The CLI binary functioned as designed, but the test harness fixture failed to provide portable environment isolation on macOS.

---

## 3. Confirmed Root Cause

1. **Platform Standard Path Discrepancy**:
   - On **Linux**, `QStandardPaths` conforms to the XDG Base Directory Specification. When `XDG_CONFIG_HOME` is set, `QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)` returns `$XDG_CONFIG_HOME/<appName>`.
   - On **macOS**, `QStandardPaths` delegates to Apple's CoreFoundation / Cocoa API (`NSSearchPathForDirectoriesInDomains` / `CFCopyHomeDirectoryURL`). CoreFoundation ignores `XDG_*` variables and determines the user's home directory via system user database APIs unless overridden by `CFFIXED_USER_HOME`.
   - On **Windows**, `QStandardPaths` uses Known Folders mapped to `%USERPROFILE%`, `%APPDATA%`, and `%LOCALAPPDATA%`.
2. **Invalid Fixture Assertion**:
   - The test fixture asserted:
     `const auto configRoot = env.value(QStringLiteral("XDG_CONFIG_HOME"));`
     `QVERIFY2(settingsFile.startsWith(configRoot), "settings file is not rooted at XDG_CONFIG_HOME; test isolation would be invalid");`
   - Even if macOS redirected home to a temporary directory, `settingsFile` on macOS is located at `<home>/Library/Preferences/sentinel-cli/settings.json`, which does not start with `<home>/config` (`XDG_CONFIG_HOME`).
3. **Symmetric Path Derivation Asymmetry**:
   - In `initTestCase()`, `HOME` was set to `probeDir.path()`.
   - In `init()`, `HOME` was set to `root + "/home"`, but `root + "/Documents"` was created, and `settingsPath()` derived the settings file by appending `settingsSuffix_` to `XDG_CONFIG_HOME`.
   - This made `settingsPath()` unable to locate or write the settings file on non-XDG platforms.

---

## 4. Exact Fixture Correction

The smallest, portable, assertion-preserving correction was implemented in `tests/integration/test_legacy_cli_exit_codes.cpp`:

### 4.1 Symmetric Child Environment Helper
A helper function `applyTestEnvironment` was introduced to configure the child process environment uniformly in both `initTestCase()` and `init()`:

```cpp
namespace {

void applyTestEnvironment(QProcessEnvironment& env, const QString& root) {
    env.insert(QStringLiteral("HOME"), root);
    env.insert(QStringLiteral("CFFIXED_USER_HOME"), root);
    env.insert(QStringLiteral("XDG_CONFIG_HOME"), root + QStringLiteral("/config"));
    env.insert(QStringLiteral("XDG_DATA_HOME"), root + QStringLiteral("/share"));
    env.insert(QStringLiteral("XDG_STATE_HOME"), root + QStringLiteral("/state"));
    env.insert(QStringLiteral("XDG_CACHE_HOME"), root + QStringLiteral("/cache"));
    env.insert(QStringLiteral("XDG_DOCUMENTS_HOME"), root + QStringLiteral("/Documents"));
#if defined(Q_OS_WIN)
    env.insert(QStringLiteral("USERPROFILE"), root);
    env.insert(QStringLiteral("APPDATA"), root + QStringLiteral("/AppData/Roaming"));
    env.insert(QStringLiteral("LOCALAPPDATA"), root + QStringLiteral("/AppData/Local"));
#endif
}

} // namespace
```

### 4.2 Portable Root Derivation in `initTestCase()`
Instead of asserting that the settings path is rooted at `XDG_CONFIG_HOME`, the test validates that the resolved settings file is strictly rooted within the disposable temporary probe root across native path separators:

```cpp
    const auto cleanProbeRoot = QDir::fromNativeSeparators(probeRoot);
    const auto cleanSettingsFile = QDir::fromNativeSeparators(settingsFile);
    QVERIFY2(
        cleanSettingsFile.startsWith(cleanProbeRoot),
        "settings file is not rooted in isolated test directory; test isolation would be invalid");
    settingsSuffix_ = cleanSettingsFile.mid(cleanProbeRoot.size() + 1);
    QVERIFY2(QFileInfo(settingsSuffix_).isRelative(), "settings suffix must be relative");
```

### 4.3 Portable `settingsPath()` Resolution
`settingsPath()` computes the settings path relative to the test case's disposable temporary root directory (`tempDir_`):

```cpp
QString LegacyCliExitCodesTest::settingsPath() const {
    return tempDir_ ? QDir(tempDir_->path()).filePath(settingsSuffix_) : QString{};
}
```

### 4.4 Multi-Platform Document Root Blocking
In `chatRefusedWithoutConversationStoreSetsFailure()`, Windows `USERPROFILE/Documents` was added alongside `XDG_DOCUMENTS_HOME` and `HOME/Documents` with non-empty path verification:

```cpp
    const QStringList documentRoots{
        baseEnv_.value(QStringLiteral("XDG_DOCUMENTS_HOME")),
        baseEnv_.value(QStringLiteral("HOME")) + QStringLiteral("/Documents"),
#if defined(Q_OS_WIN)
        baseEnv_.value(QStringLiteral("USERPROFILE")) + QStringLiteral("/Documents"),
#endif
    };
    for (const auto& root : documentRoots) {
        if (!root.isEmpty()) {
            QVERIFY(QDir().mkpath(root + QStringLiteral("/Sentinel/conversations.sqlite3")));
        }
    }
```

---

## 5. Environment Isolation Guarantees

1. **Zero Global Environment Mutation**:
   - `::setenv`, `putenv`, and `qputenv` are never called.
   - The parent test process environment remains untouched.
2. **Child-Scoped Overrides**:
   - Every `sentinel-cli` child process is executed with an explicit `QProcessEnvironment` instance (`probe.setProcessEnvironment(env)`, `process.setProcessEnvironment(env)`).
3. **Guaranteed Disposable Lifecycle**:
   - Each test case creates a fresh `std::unique_ptr<QTemporaryDir> tempDir_` in `init()` and resets it in `cleanup()`.
   - Intermediate files (`settings.json`, databases, logs) are deleted automatically upon test case completion.
4. **Strict Isolation from Real User Profile**:
   - `statusAndConfigSucceedWithoutConfiguration` asserts `!QFile::exists(settingsPath())` on the clean profile.
   - On macOS: `CFFIXED_USER_HOME` routes settings to `<tempDir>/Library/Preferences/sentinel-cli/settings.json`, databases to `<tempDir>/Documents/Sentinel/`, and logs to `<tempDir>/Library/Application Support/sentinel-cli/Logs/`.
   - On Linux: `XDG_CONFIG_HOME`, `XDG_DATA_HOME`, and `HOME` route all paths to `<tempDir>/config`, `<tempDir>/share`, and `<tempDir>/Documents`.
   - On Windows: `USERPROFILE`, `APPDATA`, and `LOCALAPPDATA` route paths to `<tempDir>/AppData/Roaming`, `<tempDir>/AppData/Local`, and `<tempDir>/Documents`.
   - Host settings, credentials, conversations, and models are completely protected from child process access.

---

## 6. Original Versus Corrected Results

| Metric | Original Fixture | Corrected Fixture |
| --- | --- | --- |
| External workaround required | `CFFIXED_USER_HOME=/...` | **None** (pure `ctest`) |
| `initTestCase()` | FAILED (assertion) | **PASS** |
| `usageWithoutArgsReturnsSuccess` | Not executed | **PASS** (exit 0) |
| `unknownCommandReturnsUsageError` | Not executed | **PASS** (exit 1) |
| `chatWithoutPromptReturnsUsageError` | Not executed | **PASS** (exit 1) |
| `configUnknownKeyReturnsUsageError` | Not executed | **PASS** (exit 1) |
| `modelPullBlankNameReturnsUsageError` | Not executed | **PASS** (exit 1) |
| `statusAndConfigSucceedWithoutConfiguration` | Not executed | **PASS** (exit 0) |
| `modelListSucceedsAgainstFixture` | Not executed | **PASS** (exit 0) |
| `modelListFailsWhenEndpointIsRefused` | Not executed | **PASS** (exit 2) |
| `modelPullSucceedsAgainstFixture` | Not executed | **PASS** (exit 0) |
| `modelPullFailureReturnsNonzero` | Not executed | **PASS** (exit 2) |
| `modelPullHttpErrorReturnsNonzero` | Not executed | **PASS** (exit 2) |
| `modelPullStallReturnsTimeout` | Not executed | **PASS** (exit 3) |
| `chatWithoutConfigurationReturnsNonzero` | Not executed | **PASS** (exit 2) |
| `chatProviderFailureReturnsNonzero` | Not executed | **PASS** (exit 2) |
| `chatSucceedsAgainstFixture` | Not executed | **PASS** (exit 0) |
| `chatStallReturnsTimeout` | Not executed | **PASS** (exit 3) |
| `chatRefusedWithoutConversationStoreSetsFailure` | Not executed | **PASS** (exit 2) |
| `interruptedPullExitsAbnormally` | Not executed | **PASS** (signal exit) |
| `cleanupTestCase()` | Passed | **PASS** |
| **Suite Totals** | 1 passed, 1 failed (0/18 cases) | **20 passed, 0 failed (18/18 cases)** |
| **Execution Time** | 160 ms (aborted) | 13.54 s (QtTest) / 15.26 s (CTest) |

---

## 7. Full CTest Outcome

The complete Sentinel test suite was executed under normal offscreen configuration:
```bash
QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure
```

Result:
- **137 / 137 tests passed (100% pass rate)**
- Total execution time: 162.18 seconds
- Zero failures, zero skips, zero timeouts

Focused Agent, LLM, and provider test run:
- **12 / 12 tests passed (100% pass rate)**
- Execution time: 34.33 seconds
- Including: `test_llm_agent_runtime`, `test_agent_reliability_harness`, `test_agent_reliability_ipc`, `test_agent_loop`, `test_agent_runtime`, `test_observation_policy`, `test_real_tool_executor_tools`, `test_provider_contracts`, `test_bounded_streaming_contract`, `test_local_inference`.

---

## 8. Verification Checks

1. **Rust CLI Compatibility**:
   - `cargo fmt --all --check` in `cli/`: **Clean (code 0)**.
   - No Rust source or CLI reference files were modified.
2. **Formatting**:
   - `clang-format --dry-run --Werror` on `tests/integration/test_legacy_cli_exit_codes.cpp` and all CLI source files: **Clean (code 0)**.
3. **Static Analysis & Compilation**:
   - Full CMake build (`cmake --build --preset tests`): **Clean (code 0)**.
4. **Generated IPC Checks**:
   - `python3 tools/ipc/generate.py --check`: **Clean (code 0)**.
   - `python3 tools/ipc/generate_desktop.py --check`: **Clean (code 0)**.
5. **Repository Privacy Scanner**:
   - `python3 tools/analysis/check_personal_paths.py`: **0 file(s) flagged (code 0)**.
6. **Git Diff Validation**:
   - `git diff --check`: **Clean (code 0)**.
   - Exactly one file modified: `tests/integration/test_legacy_cli_exit_codes.cpp`.

---

## 9. Cross-Platform Limitations & Verification Gaps

1. **macOS Host**:
   - Native verification performed on macOS Darwin 27.0.1 (arm64), AppleClang 21.0.0, Qt 6.12.0.
   - Confirmed full 20/20 and 137/137 passing suites.
2. **Linux Target (Primary Target - Fedora KDE Plasma)**:
   - Linux XDG environment variables (`XDG_CONFIG_HOME`, `XDG_DATA_HOME`, `XDG_STATE_HOME`, `XDG_CACHE_HOME`, `XDG_DOCUMENTS_HOME`, `HOME`) remain active and identical to original behavior.
   - Native Linux host was not directly available during this review; verification on Linux must be confirmed in CI.
3. **Windows Target**:
   - `USERPROFILE`, `APPDATA`, and `LOCALAPPDATA` variables and native path separator normalization were added for portability.
   - Native Windows host was not directly available during this review; verification on Windows must be confirmed in CI.

---

## 10. Remaining Blockers & Acceptance Matrix (AC-P1-07 / PD-D05)

1. **PD-D05 (Legacy CLI Disposition)**:
   - Remains an open Pending Decision regarding whether the Legacy C++ in-process CLI (`apps/sentinel-cli`) should be retained alongside or retired in favor of the canonical daemon-backed Rust CLI (`cli/`).
   - This remediation ensures that as long as the Legacy CLI is retained and tested in the tree, its test suite executes reliably across platforms.
2. **AC-P1-07 (Acceptance Status)**:
   - The test environment isolation failure preventing normal macOS CTest execution is resolved.
   - AC-P1-07 remains **Blocked** at the project level solely due to the open PD-D05 decision and the requirement for maintainer-authorized CI verification across native Linux/Windows runners.

---

## 11. Recommended Integration Action

1. Maintain the single-file fixture correction in `tests/integration/test_legacy_cli_exit_codes.cpp`.
2. Keep the nine-file P1-05 change set intact along with `P1_05_CURRENT_MAIN_INTEGRATION_VALIDATION.md` and this report.
3. Submit candidate for maintainer review and authorized multi-platform CI execution.
