# INT-02 — SEC-01 security-first integration candidate review

Date: 2026-10-16 (Europe/Istanbul). Read-only source inspection plus a documentation-only
integration candidate. No product build, CTest, commit, push, merge, reset, stash or PR was performed.

Absolute workstation paths are intentionally excluded from this document; worktrees are named by
their managed-worktree alias.

## 1. Objective and baseline identity

Goal: prepare and independently validate a minimal **main-relative** integration candidate that
contains **only** the completed SEC-01 audit-privacy remediation, so the current-tree personal-path
CI gate is unblocked without importing unrelated P1/P2/P3 development work.

| Item | Value |
| --- | --- |
| Repository | `sentinel-projects/sentinel` (`origin`) |
| Baseline branch | `main` |
| Baseline SHA (`main`, `origin/main`, `HEAD`) | `619e989c419956393201a749da1cede386e83ba3` |
| Candidate branch | `review/int-02-sec-01-integration` |
| Candidate worktree (alias) | `int-02-sec-01-integration/Sentinel-sanitized` |
| Source worktree (read only) | `sec-01-audit-privacy/Sentinel-sanitized` (branch `fix/sec-01-audit-privacy`) |
| Candidate HEAD | `619e989c419956393201a749da1cede386e83ba3` (no commit created) |
| Working tree | Two-file documentation delta; index untouched |

`main`, stored `origin/main` and the new worktree HEAD agreed on `619e989c…` at entry. The new
worktree was created from `main` (not from the SEC-01 branch), so the candidate is already
main-relative.

## 2. Preflight findings

1. **AGENTS.md / reports read.** Root `AGENTS.md`, the SEC-01 remediation report and the INT-01
   multi-worktree integration readiness audit were read before acting.
2. **Main identity.** Local `main` = stored `origin/main` = `619e989c419956393201a749da1cede386e83ba3`.
   No divergence; the candidate worktree was branched from this exact commit.
3. **SEC-01 already in main?** **No.** `docs/audits/SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md`
   is absent from `619e989c`, and the historical file still carries the unremediated path token.
   Therefore the correction was applied rather than skipped.
4. **Source worktree status (read only).** `fix/sec-01-audit-privacy` is at `619e989c` with:
   - modified: `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` (the redaction + editorial note),
   - untracked: `docs/audits/SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md` (the receipt).
   The source worktree was not modified, reset, stashed or committed.
5. **SHA-256 digests verified independently** (see §3). They match the SEC-01 report's recorded
   values exactly.
6. **Baseline privacy scan reproduced.** `python3 tools/analysis/check_personal_paths.py` on a clean
   `main` checkout exits **1** and flags exactly one file
   (`docs/audits/P1_04_2_LINUX_CI_VALIDATION.md`), matching the inherited finding recorded by
   INT-01 and SEC-01. The flagged token is a `/home/…/`-style absolute local directory path with a
   private local identifier; it is not reproduced here.

## 3. Integration candidate — imported files and hashes

The reviewed sanitized bytes were copied byte-for-byte from the read-only SEC-01 worktree, so the
candidate reproduces the reviewed remediation exactly.

| Repository-relative file | Change | SHA-256 in candidate |
| --- | --- | --- |
| `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` | One local-path token redacted + dated editorial note added | `0ab8f7405e3264acb4b0c20e5687c3cc00a3015886e9882dc823bca2a1fa1d63` |
| `docs/audits/SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md` | New remediation receipt (untracked → to be added) | `3260c7e88a75a798e61de643b28eb98b9265036cbf625865eb30ba9c8ff362a4` |

| Reference hash | Value |
| --- | --- |
| Original (main) `P1_04_2_LINUX_CI_VALIDATION.md` | `f00a8bcd35c4ad5d47d2ef4db070f4f9f3e0b85d86f4bad7209db34f3d67809d` |
| Candidate (sanitized) `P1_04_2_LINUX_CI_VALIDATION.md` | `0ab8f7405e3264acb4b0c20e5687c3cc00a3015886e9882dc823bca2a1fa1d63` |

Tracked diff versus `main`: **1 file changed, 3 insertions(+), 1 deletion(-)**.

The only content change is the redaction target `…=.local/tmp`-style absolute path replaced by the
literal placeholder `<REDACTED_LOCAL_PATH>`, plus a two-line dated editorial note under the title
(one blockquote plus one blank line). The surrounding command (`export TMPDIR=`), the
same-filesystem requirement and every other byte are unchanged.

## 4. Before / after privacy evidence

| Moment | Command | Result |
| --- | --- | --- |
| Baseline (`main` = `619e989c`) | `python3 tools/analysis/check_personal_paths.py` | **FAIL** — exit 1, 1 file flagged: `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` (token `/home/<id>/`) |
| Candidate | `python3 tools/analysis/check_personal_paths.py` | **PASS** — exit 0, 0 files flagged |
| Candidate changed files only (scanner pattern) | scoped scan of both files | 0 matches in each |
| Candidate `git diff --check` | tracked | clean (exit 0) |
| Candidate `SEC_01…RESULT.md` whitespace | `--no-index --check` | no whitespace diagnostics |

The flagging control is in three workflows — `.github/workflows/ci-linux.yml`,
`ci-macos.yml` and `ci-windows.yml` — each running the scanner as an early step on every
`pull_request`. Because those workflows have no path filter, a docs-only PR still runs the scan;
the inherited `main` failure is exactly what this candidate removes.

## 5. Historical evidence preservation

Verified by exact byte reconstruction (original = candidate minus the new note minus the
token/placeholder swap):

| Check | Result |
| --- | --- |
| Reconstructed original bytes identical to `main` version | **True** |
| Editorial note occurrences | 1 |
| Placeholder occurrences | 1 |
| Sensitive token occurrences in candidate file | 0 |
| Original / candidate line count | 157 / 159 |
| Changed lines (`+`/`-`, incl. blank line) | 4 (2 added note lines, 1 modified line, 2 counted as one +/- pair) |
| Unaffected original lines | **156** (matches the SEC-01 claim) |
| `git diff` on candidate prints any original path | No |

All historical dates, counts, commands, digests and conclusions are preserved:

| Historical claim | State |
| --- | --- |
| Audit date / OS (2026-10-16, Fedora Linux 44, KDE, x86_64) | Unchanged |
| Rust stable / minimum (1.99.1 / 1.94.0) | Unchanged |
| IPC suite 26 passed / 0 failed; doc-tests 0 | Unchanged |
| Stress 100/100 invocations, 2,600 executions, 0 failures | Unchanged |
| Workspace 15 + 26 + 22 + 78 = 141 passed, 0 failed | Unchanged |
| Initial EXDEV failure (code 18; devices 43 vs 55; mascot line 584) | Unchanged |
| Same-filesystem accommodation (mascot 22/22 after `TMPDIR` adjustment) | Unchanged |
| Historical macOS pre-fix failures 86/14 | Unchanged |
| Branding/privacy checks 5,226 sources / 69 branding files | Unchanged |
| Hosted CI / AC-P1-04 partial, pending | Unchanged |

The earlier "zero findings" scanner claim is not rewritten; the later inherited failure is recorded
separately rather than concealed.

## 6. Independent review results

| Review item | Result |
| --- | --- |
| Exactly the intended path token redacted | **PASS** — single `<REDACTED_LOCAL_PATH>`; single removed token |
| No sensitive original path in the candidate diff | **PASS** — diff prints only the placeholder |
| Historical dates/counts/commands/conclusions preserved | **PASS** — see §5 |
| Repository-wide personal-path scan passes | **PASS** — exit 0, 0 files |
| Markdown links valid | **PASS** — all three relative links in the receipt resolve; historical file has none |
| Markdown structure | **PASS** — 0 unbalanced fences, no trailing whitespace, no conflict markers, valid tables |
| No source/runtime/test/acceptance-matrix files change | **PASS** — only `docs/audits/*` in the delta |
| SEC-01 report distinguishes current-tree remediation vs history | **PASS** — §6 of the receipt states history survives and no rewrite is authorized |

## 7. Historical Git exposure — requires a separate maintainer decision

The unremediated token was introduced by commit
`d4704e39f21cc7fbd1c2ba9b0ec2b00e4a54b5b3` ("docs(audit): add P1-04.2 Linux CI validation report"),
which is reachable from `main`. That blob therefore remains in the history of `origin/main` and in
every clone/fork/cache derived from it. `git grep` over the `619e989c` tree confirms the token
appears in exactly one in-tree file.

**Current-tree redaction does not recall history.** No history rewrite, force-push, remote-object
deletion, secret rotation or publication clearance is performed or claimed here. Any decision about
historic exposure, distributed clones, remote caches or retained raw logs is a **separate
maintainer/security decision** and is explicitly out of scope for this documentation candidate.

## 8. Additional integration readiness (read-only Git inspection)

### 8.1 P0/P1/P2 corrections already present in `main`

| Workstream | Commits already reachable from `main` | Status |
| --- | --- | --- |
| P0 documentation reconciliation | `6ee75bb0` (merged `52a4e516`) | In main |
| P0-03 ownership/compatibility decisions | `fe82445b` (merged `375f2af1`) | In main (recommendations only) |
| P1-04.4 hosted IPC stress readiness | `d4230063` (merged `abac0d7b`) | In main (AC-P1-04 not Passed) |
| P1-01 through 1.5 agent reliability | `35cf03c3` (merged `fd99aaa2`, `7d311eea`) | In main |
| P2 cross-platform CI stabilization | `0245e5fa` | In main |
| P2 integration (sandbox/tray) | `ebad8152` (merged `8f6a8b87`) | In main |
| P2-01…P2-04 + discovery (contracts, redaction, vision, HTTP, catalog) | `f27f893e`, `868fa5c3`, `c9b99932`, `8e1612c7` (merged `e7457be5`, `619e989c`) | In main |

These merge commits record containment, not acceptance. Do not re-import them.

### 8.2 Still uncommitted (not in `main`) — must **not** be copied into this candidate

| Item | Evidence location (read only) | State |
| --- | --- | --- |
| P2-05 bounded streaming contract | planner + provider source worktrees: untracked `docs/audits/P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md`, `tests/core/test_bounded_streaming_contract.cpp`, interface edits | Uncommitted snapshot |
| P1-01.6 corpus expansion | agent source worktree: modified `docs/audits/P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md`, `tests/core/test_agent_reliability_harness.cpp` | Uncommitted |
| P1-01.7 bounded planner | planner worktree: untracked `docs/audits/P1_01_7_PLANNER_STREAMING_INTEGRATION_RESULT.md`; modified `core/src/agent/LlmAgentRuntime.cpp` | Uncommitted |
| P2-06 native/cloud provider | provider worktree: untracked `core/include/sentinel/core/runtime/LocalInferenceTransport.h`, `tests/core/test_native_provider_contract.cpp`, `tests/core/test_vendor_cloud_contract.cpp` | Incomplete |
| P3-01/03-02 lifecycle | p3 worktree: untracked lifecycle reports/tests; modified `LocalModelRuntime.*` | Active |

None of these were copied; they remain independent of INT-02.

### 8.3 Can this be an independent documentation PR?

**Yes.** The candidate is single-purpose, documentation-only, main-relative, and its only effect is
to remove the inherited `main` privacy-gate failure plus add its receipt. It has no build, runtime,
test or acceptance dependency and no overlap with P2-05/P1-01.6/1.7/P2-06/P3.

### 8.4 CI / reviewer requirements before approval

- **CI:** all four workflows trigger on `pull_request` with no path filter. The Linux/macOS/Windows
  jobs run the personal-path scan first (now passing); the Rust jobs run `tools/ipc/generate.py --check`
  and cargo checks. Docs-only changes do not need a product build to be reviewed, but the full PR
  checks will still execute.
- **Review:** `CODEOWNERS` (`* @Sopwit`, plus `/core/`, `/integrations/`, `/plugins/`,
  `/packaging/`, `/CMakeLists.txt`, `/CMakePresets.json`, `/.github/`) names the single maintainer as
  owner; `docs/audits/` is covered by the catch-all `*` rule. INT-01 recorded the branch as
  unprotected with empty rulesets, so a green run plus maintainer approval is the practical gate.
  No separate security/independent reviewer is currently assigned (PD-D11 open).
- **Publication:** the maintainer must separately authorize commit/push/PR creation; none of that
  was performed here.

## 9. Validation results

| Check | Result |
| --- | --- |
| Full repository personal-path scanner | **PASS** — exit 0, 0 files |
| Changed-file privacy check | **PASS** — 0 matches in both files |
| `git diff --check` (tracked) | **PASS** |
| Untracked report whitespace (`--no-index --check`) | **PASS** — no diagnostics |
| Markdown link validation | **PASS** — 3/3 relative links resolve |
| Markdown formatting | **PASS** — balanced fences, valid tables, no trailing whitespace/conflict markers |
| Historical evidence consistency | **PASS** — exact reconstruction, 156 unaffected lines |
| Main-relative allowlist | **PASS** — only the two intended `docs/audits` files |
| Production build / CTest | Not run (unnecessary for a documentation-only candidate) |

## 10. Exact proposed PR scope

- **Branch to publish:** `review/int-02-sec-01-integration` (currently local only; not pushed).
- **Base:** `main` at `619e989c419956393201a749da1cede386e83ba3`.
- **Allowlist (exactly two files):**
  1. `docs/audits/P1_04_2_LINUX_CI_VALIDATION.md` — one token redaction + dated editorial note
     (candidate SHA-256 `0ab8f7405e3264acb4b0c20e5687c3cc00a3015886e9882dc823bca2a1fa1d63`).
  2. `docs/audits/SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md` — new receipt
     (SHA-256 `3260c7e88a75a798e61de643b28eb98b9265036cbf625865eb30ba9c8ff362a4`).
- **Explicitly excluded:** this review document (an audit artifact), all P1/P2/P3 source, tests,
  CMake, acceptance matrices, scanner rules, ignore lists and CI workflows. If the maintainer wants
  the review merged too, it must be a separate decision.
- **Suggested commit/PR title:** `docs(privacy): redact historical local path in P1-04.2 audit and
  record SEC-01 remediation`.

## 11. Remaining risks and required approvals

- **History exposure** (commit `d4704e39…`) persists in `origin/main` history and clones — needs a
  separate maintainer/security decision; not addressed here.
- **Scanner is bounded:** it detects a home-path pattern only; zero matches do not prove absence of
  credentials, encoded identifiers, screenshot text or metadata.
- **Historical limits unchanged:** hosted CI was never observed and AC-P1-04 remains partial.
- **Approvals:** maintainer authorization is required to commit, push and open the PR, and to make
  any history-exposure decision. No independent/security reviewer is yet assigned.

## 12. Readiness

The candidate is **ready for maintainer-authorized PR creation**. It is minimal, main-relative,
documentation-only, and passes every scoped validation. It does **not** claim publication
clearance, and it does not resolve historical Git exposure. No commit, push, merge or PR was
performed.
