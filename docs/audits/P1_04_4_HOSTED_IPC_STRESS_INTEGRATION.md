# P1-04.4 Hosted IPC Stress Test Integration

Date: 2026-10-16. AC-P1-04 remains **Ready for Validation / partial, not Passed**.

## Isolation and preflight

Dedicated worktree: `/private/tmp/sentinel-p1-04-ipc-stress`.
Branch: `ci/p1-04-ipc-stress`. Exact source baseline: `e71cca4e958109e0825b6277072bd1235e77e60a` (local main).
Cached origin/main: `e71cca4e958109e0825b6277072bd1235e77e60a`; no fetch or hosted run was performed.
IPC correction commit `16dea2c4` is an ancestor of both local main and cached origin/main.
The IPC source on both refs has SHA-256
`a1b969acbbce0b491681a3f679164311692a5a711f788412d3984bf9a1ec2508`, matching the investigation report.
The source contains 26 test functions. Rust workspace minimum is 1.94; ordinary CI explicitly uses 1.94.0 on ubuntu-latest and macos-14.

Personal-Brain was unavailable (`command -v brain` failed). Used the supplied checkout, verified its remote and refs, read AGENTS.md, the P1-04 handshake investigation report and ACCEPTANCE_MATRIX.md (especially its repeat policy and AC-P1-04 row). Only one P1-04 investigation report was present. No production code or tests were modified. The original checkout was clean at entry and was not edited. No commits, pushes, PRs, tooling installations or hosted executions were made.

## Implementation

The existing workflow gains workflow_dispatch and a dedicated manual-only ipc-stress matrix job. Ordinary push/PR CI remains intact. No label trigger is needed: manual dispatch is the selected explicit authorization mechanism.

Each OS runs `cargo +1.94.0 test --locked -p sentinel-ipc` 100 times sequentially, including the whole package suite and doc tests. There is no test filter or retry. A failed iteration is retained and subsequent distinct iterations continue. Each attempt has a combined stdout/stderr log and JSON exit code, timeout flag and elapsed time. Incremental results.json survives later failures. Metadata records the event SHA, actual checkout SHA, platform, exact compiler and command. outcome.json is written only after all iterations finish. Any failure yields exit 1; later successful iterations cannot overwrite it.

Each iteration has a 120-second deadline (including compilation); timeout kills its process group and records the actual wait return code with timed_out=true. The job has a 45-minute ceiling, leaving room for 100 bounded attempts and setup/artifacts. Matrix fail-fast is disabled. Always-run summary and artifact steps retain diagnostics on ordinary failure; missing/incomplete evidence reports FAIL / INCOMPLETE. Artifacts are separated by OS/run/attempt and retained 14 days. Setup failure produces an incomplete summary.

## Local validation

- Existing actionlint passed for the changed workflow; no tooling installed.
- Ruby YAML parsing and jobs structure assertion passed.
- Executed the actual embedded Python blocks with mocked subprocesses: all-pass, first-iteration failure followed by 99 passes, process timeout, and missing results. Expected exit statuses and summaries passed; each completed scenario produced 100 logs and 100 per-iteration JSON files.
- Ordinary rust job compared byte-for-byte with baseline and passed preservation check.
- git diff --check passed. Shell uses a literal quoted heredoc; command arguments are a Python list, with no shell interpolation or pipelines masking exit status. GitHub expressions are limited to runner/artifact configuration.
- No real Rust stress suite was run locally in this CI-only task; mocked driver checks are not hosted IPC acceptance evidence. CMake/product checks are outside this workflow/documentation scope.

## Trigger instructions

After separate maintainer review and authorized publication, select **Actions → Rust IPC clients → Run workflow**, choose the exact branch/ref containing this change and the merged IPC correction, then run it. workflow_dispatch normally requires the workflow to exist on the default branch before the UI permits dispatch. This task does not publish or dispatch it. Ordinary pushes and PRs skip ipc-stress. A maintainer may manually dispatch a reviewed PR branch; the run tests the selected ref rather than an implicit PR merge ref.

## Security and remaining evidence

contents: read is the only token permission. Checkout credentials are not persisted. No pull_request_target, secrets, privileged environment or write permissions are used. Upload action uses the repository's existing pinned v6 SHA. A manually selected ref executes code on an ephemeral hosted runner under these read-only permissions; maintainers should select the intended reviewed candidate.

Cancellation, runner loss or the hard overall timeout can prevent even always-run upload steps from finishing; such a run is incomplete and cannot satisfy acceptance. Cold compilation exceeding 120 seconds is retained as a failure, not retried automatically. Artifact expiration requires evidence archival within 14 days. Runner image labels can change; retain platform/compiler metadata and run URLs. Live remote main was not refreshed, so cached ancestry is not a claim about current GitHub state.

AC-P1-04 cannot be Passed until **both hosted matrix cells** provide 100 complete successful repetitions for the same reviewed candidate, with run URLs, exact checkout/source/workflow SHAs, runner/compiler identity, every log/exit/timeout, and review of original failure/regression evidence. Any unexplained failure or missing attempt keeps acceptance open. The acceptance matrix was not edited. This does not close P1-01 or P1-05.

## Relevant workflow diff against exact source baseline

```diff
diff --git a/.github/workflows/ci-rust.yml b/.github/workflows/ci-rust.yml
index 155cb885..d9bbd859 100644
--- a/.github/workflows/ci-rust.yml
+++ b/.github/workflows/ci-rust.yml
@@ -3,6 +3,7 @@ on:
   push:
     branches: [main]
   pull_request:
+  workflow_dispatch:
 permissions:
   contents: read
 concurrency:
@@ -27,3 +28,104 @@ jobs:
       - run: cargo +1.94.0 fmt --all --check
       - run: cargo +1.94.0 clippy --locked --all-targets -- -D warnings
       - run: cargo +1.94.0 test --locked
+  ipc-stress:
+    if: github.event_name == 'workflow_dispatch'
+    permissions:
+      contents: read
+    timeout-minutes: 45
+    strategy:
+      fail-fast: false
+      matrix:
+        os: [ubuntu-latest, macos-14]
+    runs-on: ${{ matrix.os }}
+    defaults:
+      run:
+        working-directory: cli
+    steps:
+      - uses: actions/checkout@3d3c42e5aac5ba805825da76410c181273ba90b1
+        with:
+          persist-credentials: false
+      - run: rustup toolchain install 1.94.0 --profile minimal
+      - name: Run 100 sequential IPC suites
+        shell: bash
+        run: |
+          python3 - <<'PYTHON'
+          import json
+          import os
+          from pathlib import Path
+          import platform
+          import signal
+          import subprocess
+          import sys
+          import time
+
+          output = Path("../ipc-stress-results")
+          output.mkdir(exist_ok=True)
+          command = ["cargo", "+1.94.0", "test", "--locked", "-p", "sentinel-ipc"]
+          metadata = {
+              "github_sha": os.environ["GITHUB_SHA"],
+              "checkout_sha": subprocess.check_output(["git", "rev-parse", "HEAD"], text=True).strip(),
+              "runner_os": os.environ["RUNNER_OS"],
+              "platform": platform.platform(),
+              "rustc": subprocess.check_output(["rustc", "+1.94.0", "-Vv"], text=True),
+              "command": command,
+              "iteration_timeout_seconds": 120,
+          }
+          (output / "metadata.json").write_text(json.dumps(metadata, indent=2) + "\n")
+          results = []
+          for iteration in range(1, 101):
+              started = time.monotonic()
+              timed_out = False
+              with (output / f"iteration-{iteration:03d}.log").open("w") as log:
+                  process = subprocess.Popen(command, stdout=log, stderr=subprocess.STDOUT,
+                                             start_new_session=True)
+                  try:
+                      code = process.wait(timeout=120)
+                  except subprocess.TimeoutExpired:
+                      timed_out = True
+                      os.killpg(process.pid, signal.SIGKILL)
+                      code = process.wait()
+              result = {"iteration": iteration, "exit_code": code, "timed_out": timed_out,
+                        "elapsed_seconds": round(time.monotonic() - started, 3)}
+              (output / f"iteration-{iteration:03d}.json").write_text(json.dumps(result) + "\n")
+              results.append(result)
+              (output / "results.json").write_text(json.dumps(results, indent=2) + "\n")
+              print(json.dumps(result), flush=True)
+          failed = sum(item["exit_code"] != 0 or item["timed_out"] for item in results)
+          (output / "outcome.json").write_text(json.dumps({"completed": len(results), "failed": failed}) + "\n")
+          sys.exit(1 if failed else 0)
+          PYTHON
+      - name: Summarize IPC stress evidence
+        if: always()
+        shell: bash
+        run: |
+          python3 - <<'PYTHON'
+          import json
+          import os
+          from pathlib import Path
+
+          output = Path("../ipc-stress-results")
+          output.mkdir(exist_ok=True)
+          results_path = output / "results.json"
+          results = json.loads(results_path.read_text()) if results_path.exists() else []
+          failed = sum(item["exit_code"] != 0 or item["timed_out"] for item in results)
+          outcome = output / "outcome.json"
+          success = len(results) == 100 and failed == 0 and outcome.exists()
+          summary = (f"### IPC stress: {os.environ['RUNNER_OS']}\n"
+                     f"Result: {'PASS' if success else 'FAIL / INCOMPLETE'}\n\n"
+                     f"Completed: {len(results)}/100; failed: {failed}; "
+                     f"missing: {100 - len(results)}. No retries.\n\n"
+                     f"Source: `{os.environ['GITHUB_SHA']}`; Rust: 1.94.0. "
+                     "See diagnostic artifact for every log and exit code.\n")
+          (output / "summary.md").write_text(summary)
+          with open(os.environ["GITHUB_STEP_SUMMARY"], "a") as handle:
+              handle.write(summary)
+          PYTHON
+      - name: Upload IPC stress diagnostics
+        if: always()
+        uses: actions/upload-artifact@b7c566a772e6b6bfb58ed0dc250532a479d7789f # v6
+        with:
+          name: ipc-stress-${{ matrix.os }}-${{ github.run_id }}-${{ github.run_attempt }}
+          path: ipc-stress-results/
+          if-no-files-found: error
+          retention-days: 14
```
