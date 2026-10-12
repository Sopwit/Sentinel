# GIT-01 — Current repository generated-artifact cleanup

Date: 2026-10-12. Status: **COMPLETE — current-index cleanup only**. The owner authorized index-only untracking of the exact SEC-P1.1 proposal. Exactly **2,767 generated files** are now staged as deletions, with **all local contents preserved**. No commit, push, tag, history rewrite, remote request/mutation, artwork edit or release publication occurred. GIT-02 has not begun.

The completed SEC-P1, SEC-P1.1, SEC-P1.1B and SEC-P1.1B-R1 reports were consulted. Personal-Brain remains unavailable (`brain` absent); the explicitly supplied Sentinel checkout and repository instructions were used. No broad security audit was repeated.

## 1. Exact approved scope and baseline reconciliation

Primary HEAD remains `c35a9aedff5832bd0704bbb164f4b25f9a7bff2d`. SEC-P1.1 originally proposed cleanup at `541b8db9…`; subsequent owner commits advanced HEAD. Fresh checks reconcile that drift: the approved Cargo target tree is still `7e8bfa3a3f71af5c1d59b460a4b867e90f70f397`, and every selected current index blob, size and SHA-256 matches the original private receipt.

| Scope | Files | Local bytes preserved |
| --- | ---: | ---: |
| `docs/reviews/core-warden-rust-terminal/target/` | 2,758 | 703,223,935 |
| Nine exact Python bytecode paths below | 9 | 131,416 |
| Total | **2,767** | **703,355,351** (703 MB decimal / 671 MiB) |

Nine bytecode paths:

- `docs/reviews/agent-v323-2026-10-10/reproduce/__pycache__/sentinel_v323_accept.cpython-315.pyc`
- `docs/reviews/agent-v324-2026-10-10/reproduce/__pycache__/live_readonly.cpython-315.pyc`
- `docs/reviews/core-warden-cw044/__pycache__/ab_pty.cpython-315.pyc`
- `docs/reviews/core-warden-cw044/__pycache__/production_pty.cpython-315.pyc`
- `docs/reviews/core-warden-cw045/__pycache__/production_pty.cpython-315.pyc`
- `docs/reviews/core-warden-rust-terminal/__pycache__/fragmented_probe_check.cpython-315.pyc`
- `docs/reviews/core-warden-rust-terminal/__pycache__/run.cpython-315.pyc`
- `docs/reviews/tui-v33b/__pycache__/pty_journeys.cpython-315.pyc`
- `docs/reviews/tui-v34/__pycache__/compatibility.cpython-315.pyc`

The [approved pathspec](sec-p1-1/untracking-paths.txt), target tree identity and private per-file receipt identify every removed path precisely. Private receipt SHA-256: `3ca23c5446e225448523f65e4ff6373c05e7aad97092bdb37236870a1a97130f`. The full sensitive/raw receipt remains outside the repository and in the recovery snapshot; it was not republished.

Before removal, there were 7,900 index entries, no staged or unstaged tracked changes, and twelve pre-existing untracked report/receipt files. Those twelve files were captured by hash and preserved. The cleanup reduced index entries to **5,133**, with no other index entry/mode/blob changes. Prior SEC report files remain untracked and untouched.

## 2. Index operation and local recovery

The original ten-line proposal was expanded to a private NUL-delimited list of exactly 2,767 already-tracked, receipt-verified files. Immediately before mutation, HEAD, the index file identity, empty tracked diffs and every selected local hash were checked again. The operation used literal path handling:

```text
git rm --cached --pathspec-from-file=<private-exact-file-list> --pathspec-file-nul
```

No `--force`, broad recursive cache removal or working-file deletion was used. All 2,767 paths remain present locally with original sizes and SHA-256 values. All 7,900 original tracked-file hashes remain unchanged after validation, including removed artifacts, source, Cargo manifests/lockfiles, diagnostic source, Core Warden PNGs, Blender masters and FA-3 branding. The nine Python sources remain tracked. All retained index entries match their baseline modes/blob IDs.

The existing persistent private recovery snapshot under `$HOME/.codex/private-backups/<backup_id>` was not overwritten. Its identity and prior complete restoration receipt are in [SEC-P1.1B recovery validation](sec-p1-1b/recovery-validation.json). This phase verified all selected files against both that snapshot and the existing separately restored copy, verified the backed-up private receipt, all twenty original report hashes and the unchanged historical manifest, and reran backup `git fsck --full` successfully. This is recoverable same-device local storage, sufficient for the reversible current-index operation; independent encrypted/off-device restoration remains a future shared-history gate.

A private pre-operation index copy, tracked/untracked hash baseline, expanded exact path list, operation log and test logs were retained outside the repository. Existing generated files were neither rebuilt in place nor deleted. No historical Git object-storage reclamation is claimed.

## 3. Evidence addendum and reference correction

Created the [generated-artifact evidence addendum](../reviews/agent-v323-2026-10-10/GIT_01_GENERATED_ARTIFACT_ADDENDUM.md). Original `artifact-sha256.json` remains byte-identical, SHA-256 `b9fc25c508f590d2c5d8314cdf1cde47fbb4dc4f768bf5f80f8e15e2da9aa4ec`.

Focused inspection corrects the earlier reported generated-log association. The selected `target/release/build/zmij-7c2145943554e178/stderr` is empty (zero bytes; SHA-256 `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`). The manifest does **not** name that log. Its fifteen empty-file entries share the same digest, which caused the prior digest-based candidate match. This was not an explicit path reference. The addendum documents the correction and original identities without silently altering previous reports, manifests, measurements or acceptance results.

No named manifest entry is invalidated by untracking that generated log. Local and backup originals remain available. Probe scripts referring to `target/release` still describe reproduction locations; callers must build the required profile or configure the executable location. New output is not a replacement historical measurement. HEAD and history still contain the originally tracked artifacts because no commit or rewrite was performed.

## 4. Validation results

[Sanitized validation receipt](git-01/validation.json) summarizes the bounded checks; raw inventories/logs remain private.

| Check | Result |
| --- | --- |
| Scope, baseline and receipt identity | PASS — 2,758 target files + nine exact bytecode files; original target tree/index blob identities, sizes and hashes match. |
| Recoverability and original evidence | PASS — selected snapshot/restoration hashes, private receipt, twenty original reports, historical manifest and backup Git integrity. |
| Local file preservation | PASS — all 7,900 baseline tracked files and twelve inherited untracked files remain byte-identical. |
| Index diff | PASS — exactly 2,767 staged deletions, no additional staged changes; 5,133 retained entries identical. |
| Source/artwork integrity | PASS — exhaustive baseline tracked-file hash equality includes source, manifests, locks, canonical artwork and masters; no behavioral edits. |
| Ignore coverage | PASS — all 2,767 removed files are ignored; rules `**/target/`, `__pycache__/` and `*.py[cod]` cover generated outputs. No `.gitignore` edit needed. |
| Retained inputs visible | PASS — `check-ignore --no-index` negative checks for probe source/manifest/lock/script and a canonical PNG; all nine bytecode source counterparts remain tracked. |
| Fresh probe build | PASS — Rust 1.94.0, macOS arm64, `--locked --offline`, new private external Cargo target. |
| Probe unit tests | PASS — 2/2: `base64_vectors`; `upload_and_placement_do_not_clear_or_invalidate_screen`. |
| Evidence references | PASS — original manifest unchanged; corrected empty-digest association documented; new report/addendum links resolve. |
| Whitespace and final preservation | PASS — staged/unstaged `git diff --check`; HEAD/public local refs preserved; no unstaged tracked changes. |

Commands used the retained `docs/reviews/core-warden-rust-terminal/Cargo.toml` with `CARGO_TARGET_DIR=<new-private-external-target> cargo +1.94.0 build/test --locked --offline`. This proves the scoped source rebuild/tests in the cached environment, not bit-identical historical binaries, physical terminal acceptance or binary distribution clearance. Full native/runtime suites were unnecessary because no behavior changed.

The checkout is intentionally not clean: exactly the approved deletions are staged; inherited SEC report files and the new report, addendum and sanitized receipt are untracked. No documentation was automatically staged and no commit was created. Local generated files are now ignored. Git HEAD, historical exposure, remote refs and release assets are unchanged.

## 5. Known GIT-02 decisions carried forward only

Use [SEC-P1.1B-R1](SEC_P1_1B_R1_FINAL_DECISION_REPORT.md) and its existing candidate identities; no new broad scan or GIT-02 implementation occurred.

- **Raw evidence and screenshots:** choose public retention versus explicit sanitized derivatives/private originals for the 374 known home-path-bearing raw-evidence blobs and screenshot/conversation context. Define per-evidence-set ownership and bounded context/pixel review. Preserve measurements and original acceptance assertions; a derivative needs its own digest/ledger.
- **Historical personal paths:** twenty historical report blobs and two operational/test source blobs require selective policy and compatibility decisions. Three Linuxbrew source variants are harmless standard prefixes and should be retained. Current index cleanup does not erase any historical path.
- **Fourteen canonical PNG metadata cases:** decide retain unchanged versus approved metadata-only derivatives, decoded-pixel equality and original/new hash/provenance ledgers. Exact files remain those in `cw02` and `cw02_1` named by R1. All PNGs and Blender masters remain unchanged now.
- **Private original retention:** appoint the custodian, access/encryption policy, retention duration and independently restored off-device backup before any shared-history change. Preserve the original reports, raw diagnostics, screenshots, manifests and mapping receipts privately.
- **Historical/public versus private refs:** 413 of the 414 residual home-matching blobs were traced to cached public-ref histories; the branding bytecode blob was private-checkpoint-only. Do not expand a public rewrite scope for that private blob or publish checkpoints. Historical rewrite scope, signatures, public refs, PR/release/fork coordination and administrator approval remain separate R1 decisions.

Existing dependency/licensing/release gates remain unchanged. GIT-01 ends here; no historical cleanup, remote coordination or publication follows automatically.
