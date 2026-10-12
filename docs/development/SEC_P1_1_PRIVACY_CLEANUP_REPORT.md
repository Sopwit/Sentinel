# SEC-P1.1 — Public repository privacy and tracked artifact cleanup

Date: 2026-10-12. Baseline local HEAD: `541b8db91902066840093425109e9d9d93ae3b5c`.

Status: **safe report corrections applied to the working tree; generated-artifact removal is a proposal awaiting maintainer approval.** No files were untracked or deleted, no history was rewritten, and no commit, push, fetch or publication was performed. Public binary distribution remains uncleared.

This phase follows [SEC-P1](SEC_P1_PUBLIC_REPOSITORY_AUDIT.md). Existing SEC-P1 changes and other development work were preserved. Personal-Brain was unavailable (`brain` executable absent); the explicitly supplied checkout and repository instructions were used. Architecture, runtime, security, building and testing guidance were reviewed in SEC-P1; this phase changes documentation only.

## 1. Confirmed exposure categories and retention classification

| Class | Findings | Disposition |
| --- | --- | --- |
| Required production source | C++/QML/Rust source, runtime contracts and project manifests. A production Homebrew fallback contains a developer-specific home path. | Retain. No runtime edits in this phase; separately approve a portable fallback change. |
| Required reproducible tests | Test sources, Python reproduction scripts and probe source. One provider certification test contains a synthetic developer-root literal. | Retain sources and test semantics. Decide separately whether the literal can become a portable fixture. |
| Canonical approved assets | Core Warden PNGs and Blender masters. Fourteen PNGs contain home-directory strings in `tEXt` metadata. | All asset bytes preserved. Metadata sanitation needs artwork-owner approval and a digest/provenance plan. |
| Historical review evidence | Diagnostic JSON/JSONL, transcripts, screenshots, reproduction harnesses, acceptance reports and hash manifests. | Preserve originals. Approve a public sanitized derivative or restricted retention policy per evidence set. |
| Reproducible generated outputs | Cargo probe target: 2,758 tracked files, 703,223,935 bytes; nine tracked Python bytecode files, 131,416 bytes. | Precise proposal only; index-only removal after approval. |
| Sensitive/personal information | Home-directory identifiers, machine paths, temporary-profile locations, model-volume paths and embedded image metadata. | Twenty report files corrected safely; raw evidence and protected assets retained. No confirmed genuine credential found. |
| Owner retention decisions | Raw review archives, screenshots, binary metadata, personal-path source literals and generated artifacts referenced by evidence manifests. | Explicit decisions required; no silent deletion, acceptance rewrite or replacement evidence. |

The prior audit's **1,803 home-path files** is its own baseline and scan definition. The focused current-working-tree scan here matches Unix home directories, mounted-volume paths and platform temporary-profile directories across text and binary bytes. It finds **1,900 tracked files**: 1,389 target outputs, 491 historical-review files, four bytecode files and 16 source/asset files. These are file matches, not unique people or confirmed secrets. Scanner definitions differ; these figures must not be presented as a reduction from 1,803. Binary encodings, compressed metadata and screenshot pixels can evade byte-pattern scans.

The 16 source/asset matches are the 14 PNGs plus `apps/sentinel-daemon/service/DaemonModelHelpers.cpp` and `tests/helpers/provider_certification_probe.cpp`. Redacted locations are sufficient for triage; personal values are intentionally omitted here.

Screenshot inspection was sampled, not exhaustive. The inspected `docs/reviews/agent-v325-2026-10-10/nemotron/A-single-1-final.ansi.png` contains workspace/model and conversation context; no credential was observed in that image. It was preserved. The SEC-P1 screenshot concerns and remaining screenshot collection still require owner review; no claim of complete pixel-level clearance is made.

## 2. Safe corrections applied

Twenty Markdown reports now use role placeholders for machine-specific paths and quoted `SENTINEL_REPO` command examples. Each edited report explicitly identifies the privacy edit and its changed file digest. Commands document initializing the checkout root with `git rev-parse --show-toplevel`; Python executable examples use `python3` instead of an installation-specific absolute path.

The exact file list and before/after SHA-256 values are in [report-redactions.json](sec-p1-1/report-redactions.json). No recorded timing, acceptance outcome, diagnostic payload, screenshot, source file, canonical asset or historical raw archive was changed. Verification confirmed that every original report line without a machine-specific path remains present verbatim.

Byte-for-byte originals were saved outside the repository under `/tmp/sentinel-sec-p1-1-originals/`, with private directory/file permissions. They are temporary local retention copies, not a durable archive. The owner should move them to approved private durable storage before temporary storage is reclaimed. Do not add those originals or unredacted scanner output to Git.

## 3. Generated artifacts proposed for untracking

[untracking-proposal.json](sec-p1-1/untracking-proposal.json) records the exact baseline, target tree identity, counts, byte sizes and nine individual bytecode paths/digests. [untracking-paths.txt](sec-p1-1/untracking-paths.txt) is the precise future Git pathspec: one target directory and nine bytecode files. Total: **2,767 files, 703,355,351 bytes** (approximately 703 MB decimal / 671 MiB).

Target tree object: `7e8bfa3a3f71af5c1d59b460a4b867e90f70f397`.

A per-file receipt containing relative paths, byte sizes, index blob IDs and SHA-256 values is retained privately at `/tmp/sentinel-sec-p1-1-untracking-receipt.json`; its digest is recorded in the proposal. This allows a pre-removal comparison and local-file preservation check. Preserve that receipt privately with the originals before temporary storage expires.

The probe retains `Cargo.toml`, `Cargo.lock`, `src/main.rs`, its local `sentinel-mascot` dependency, reproduction scripts, report, proposed patch and physical diagnostic records. All nine bytecode candidates have corresponding retained Python source. Bytecode is interpreter-version-specific generated output and is unnecessary source evidence.

A clean, separate output directory successfully rebuilt the Rust probe offline with the retained lockfile:

```sh
CARGO_TARGET_DIR=/tmp/sentinel-sec-p1-1-probe-target \
  cargo +1.94.0 build \
  --manifest-path docs/reviews/core-warden-rust-terminal/Cargo.toml --locked --offline
CARGO_TARGET_DIR=/tmp/sentinel-sec-p1-1-probe-target \
  cargo +1.94.0 test \
  --manifest-path docs/reviews/core-warden-rust-terminal/Cargo.toml --locked --offline
```

Both unit tests passed. This demonstrates source rebuildability in the available cached toolchain, not byte-identical reproduction of historical binaries or physical terminal measurements. A clean machine also needs the Rust toolchain and dependency cache/network access. Scripts expecting `target/release` require an explicit release rebuild or a separately configured executable location; do not replace historical diagnostic outputs while rebuilding.

Existing ignore rules correctly ignore a new nested Cargo target output and Python `__pycache__` bytecode. They do not ignore the checked canonical PNG, probe source, manifest or lockfile. Ignore rules alone do not remove already tracked files.

Removing these paths from future tracking would reduce future checkout content. **It would not remove their blobs from existing Git history or immediately reclaim the historical Git object database.** No history cleanup is proposed in this phase.

## 4. Files requiring owner approval

1. Approve or reject the exact 2,767-file untracking proposal; preserve the local copies and private receipt.
2. Decide public retention versus sanitized derivatives for historical diagnostics, transcripts, screenshots and reproduction scripts. Preserve original evidence privately and label derivatives explicitly.
3. Decide whether to sanitize the 14 canonical PNG metadata chunks. Their pixels were not edited; metadata changes would still change whole-file hashes. Blender masters remain untouched and are not certified free of encoded personal metadata.
4. Approve a separate scoped production-path correction and a test-fixture decision for the two source literals. Runtime/test behavior was outside this documentation cleanup.
5. Decide how long to retain private originals and receipts, and where to store them durably.

The canonical PNG metadata matches are eight files under `assets/core-warden/renders/cw02/` (`back`, `front`, `hero`, `side`, `size_128`, `size_192`, `size_96`, `three_quarter`) and six under `cw02_1/` (`front`, `hero`, `side`, `size_128`, `size_96`, `three_quarter`), all `.png`.

## 5. Review evidence and hash impact

The 20 report digests necessarily changed. Their original and revised digests are explicit in the redaction ledger. No explicit references to the original report SHA-256 values were found in the scanned current text files (under 10 MB, excluding generated target output). External attestations and unscanned/binary references remain unknown. Do not silently replace an externally attested digest; publish a redaction addendum linking the original digest to the revised digest.

A candidate-digest cross-check found one existing evidence manifest referencing `docs/reviews/core-warden-rust-terminal/target/release/build/zmij-7c2145943554e178/stderr`, a proposed target build-log candidate: `docs/reviews/agent-v323-2026-10-10/artifact-sha256.json`. Details are in [candidate-evidence-references.json](sec-p1-1/candidate-evidence-references.json); the new proposal's self-references are also included. If untracking is approved, preserve the old manifest as historical evidence and create a documented inventory/addendum explaining the removed tracked build log. Do not fabricate a replacement measurement or silently recompute acceptance hashes.

Probe scripts and reports also refer operationally to generated executable paths. Those references remain meaningful as reproduction instructions, but are not a promise that the binary remains tracked. Physical terminal traces, screenshots, archived evidence and acceptance results remain unchanged. Any future raw-evidence sanitization must retain the original privately and identify both digests, the transformation and the effect on verification.

## 6. Local versus remote exposure confidence

| Surface | Confidence and limits |
| --- | --- |
| Current local HEAD/index | Confirmed locally: baseline commit and 7,862 tracked files; target outputs remain tracked. Documentation corrections exist only in the working tree. |
| Local Git history | SEC-P1 inspected 487 reachable commits and 11,507 blobs. Current redaction/untracking cannot remove historical exposure. This phase did not rewrite or re-scan every historical blob. |
| Remote-tracking references | Cached `origin/main` and `origin/HEAD` resolve to the baseline and contain the target tree. Other cached branch/tag snapshots were checked locally; see [local-ref-exposure.json](sec-p1-1/local-ref-exposure.json). These are local snapshots, not live GitHub state. |
| Confirmed GitHub remote exposure | User identifies the repository as public; the configured origin matches the expected GitHub repository. Live remote file/ref availability was not verified in this phase. No fetch or network request was performed. Cached refs are evidence of a prior remote snapshot, not proof of current accessibility. |
| Forks, clones, downloads, caches | Unknown. No claim that current edits, later untracking or even future history changes recall copies already distributed. |

## 7. Remaining privacy risks

Raw diagnostics and binary outputs still contain machine-specific paths. Canonical image metadata and source/test literals remain. Screenshots and transcripts can expose context beyond detectable path patterns. Public history and unknown copies remain separate exposure surfaces. The five redacted secret-scanner findings are the same four synthetic fixture tokens and one digest false positive documented in SEC-P1; no genuine credential was confirmed. This is not a guarantee that all encoded or historical secrets are absent.

## 8. Validation results

- Baseline preservation: 7,871 existing tracked/nonignored working-tree files checked by digest; only the 20 intended reports changed. Existing SEC-P1 work, assets, source, manifests and raw evidence preserved.
- Redaction ledger: private originals and edited report digests verified; original non-path lines preserved verbatim.
- Git: `git diff --check` passed; `git diff --cached --quiet` passed; 7,862 tracked files remain. No staged removals or index changes.
- Focused privacy scan: current tracked bytes classified as above; no matched personal values printed or added to this report.
- Gitleaks 8.30.1: completed against a fresh tracked text snapshot outside the repository, with redaction enabled; five known false positives, no new confirmed credential. Scanner finding status is not a clean zero-findings exit. Sensitive scanner logs/reports remain outside the repository.
- Ignore behavior: generated-output positive checks and retained-source/asset negative checks passed.
- Fresh offline Rust probe build: passed. Two unit tests passed (`base64_vectors`, `upload_and_placement_do_not_clear_or_invalidate_screen`). No physical terminal acceptance rerun or historical evidence overwrite.
- Full application suites were not repeated for these documentation-only changes; prior SEC-P1 test results remain historical, not newly executed SEC-P1.1 results.

Machine-readable scoped validation is recorded in [validation.json](sec-p1-1/validation.json). The checkout is intentionally dirty with inherited work, these report corrections and new SEC-P1.1 records; it is not described as clean. No newly generated build output was placed in the repository.

## 9. Exact next actions

1. Review the redaction ledger and report diffs; retain the originals and receipt in owner-approved private storage.
2. Obtain maintainer approval for the exact untracking proposal and the associated evidence-manifest addendum. Revalidate HEAD, tree identity, file counts, local digests and concurrent work immediately before executing it.
3. Only after that approval, use index-only removal without `--force` (commands below are **not executed**):

```sh
git status --short
git diff --cached --quiet
git rev-parse HEAD
git rev-parse HEAD:docs/reviews/core-warden-rust-terminal/target
git rm -r --cached --pathspec-from-file=docs/development/sec-p1-1/untracking-paths.txt
git diff --cached --stat
git diff --cached --check
git ls-files docs/reviews/core-warden-rust-terminal/target
```

4. Confirm all proposed local files remain present and match the private receipt. Verify the nine bytecode paths are no longer tracked, retained source/manifests/assets remain tracked and ignore rules cover regenerated files. Stop on any unexpected change; do not force removal over concurrent work.
5. Add the approved evidence inventory/addendum, review the staged diff and obtain separate authorization for any commit or push. No commit/push is authorized by this proposal.
6. Resolve raw-evidence and asset metadata retention decisions separately. Keep binary distribution blocked pending the remaining SEC-P1 gates. Do not begin dependency migration or Git history rewriting from this report.
