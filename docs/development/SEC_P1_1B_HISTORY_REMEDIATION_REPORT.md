# SEC-P1.1B — Git history and privacy remediation simulation

Date: 2026-10-12. Primary baseline: `c35a9aedff5832bd0704bbb164f4b25f9a7bff2d` on `main`.

Final recommendation: **NEEDS REVISION**. The exact generated-artifact removal passed isolated history/source/recovery checks, but it is not a complete privacy remedy. Remaining historical paths, raw evidence, canonical image metadata, published artifacts, six open PRs and independent-backup/owner decisions prevent recommending an immediate shared-history migration. **No cleanup was applied to the primary checkout or GitHub. Public binary distribution remains BLOCKED.**

All prior SEC reports and repository instructions were consulted, especially SEC-P1.1/P1.4. Personal-Brain was unavailable (`brain` absent); the explicitly supplied checkout was used. No AgentRuntime, daemon, Desktop, TUI, dependency or artwork changes were made. Only this report and sanitized support receipts are added to the primary working tree. Experimental Git objects/refs exist only in isolated local copies.

## 1. Current repository and remote inventory

The primary checkout initially had a clean working tree/index, **7,900 tracked files**, one registered worktree and **497 commits** reachable from local refs. Prior SEC-P1.4 changes are already committed by the owner as `c35a9aed`; this phase did not make that commit. Local main, cached origin/main and live GitHub main matched at assessment.

The initial local inventory contains **28 refs**: one local branch, eleven remote-tracking entries (ten branches plus symbolic origin/HEAD), three tags and thirteen Codex checkpoint/capture refs. The thirteen checkpoint refs point directly to **tree objects**, not commits. The complete local ref/object snapshot is backed up privately. Application-managed checkpoint names changed during this active chat: two names disappeared and two appeared, with no changed value for surviving refs. No ref deletion/update was executed against the primary repository by this agent. This churn is a concrete reason to coordinate/pause writers and checkpoint creation before any real migration.

Fresh read-only GitHub requests returned:

| Surface | Reconfirmed state |
| --- | --- |
| Visibility/default branch | Public / main; live main at baseline SHA. |
| Branches | **10**, all reported unprotected; cached branch tip identities were included in the snapshot. |
| Tags | **3**: v1.0.0-rc.1, rc.7, rc.8; lightweight tags, not signed annotated tags. |
| Open PRs | **6**, increased from SEC-P1.4's two: #8 Actions group, #12 rustix, #13 ratatui-image, #14 signal-hook, #15 Crossterm, #16 Ratatui. |
| Forks | **1** returned; fork contents, private clones and downloads not inspected. |
| Historical releases | **3 published prereleases**, not drafts: rc.1, rc.7, rc.8, each with nine uploaded assets. |

[Remote inventory](sec-p1-1b/remote-inventory.json) records sanitized branch/tag/PR/release identities, asset names/sizes and GitHub-provided digests. Pagination was checked: no next page was advertised for the queried lists. PR merge/internal refs and fork object universes were not fetched or exhaustively scanned. No Sentinel fetch, remote setting change, workflow dispatch or push was performed.

Published release metadata uses `target_commitish: main`, which is mutable and does not prove exact binary source. Current tag commits are:

- rc.1: `aa8987dadaa24ceb4feb4643417bb80a180aa8d0`
- rc.7: `1a95a5516377905477bf13d857685b5f275508a3`
- rc.8: `cec95419a63c0eda1bd5c969a0f0b0af5e219cbe`

The rc.8 release includes eight asset filenames labelled rc.7 plus a numeric 1.0.0 tarball. That metadata mismatch needs owner/source identity review; filenames alone do not establish which code is inside. Assets were not downloaded, replaced or cleared. Existing published releases are historical exposure surfaces despite the current draft-only workflow and current publication prohibition.

## 2. Private backup and verified recovery

A persistent private backup was created **outside the public repository**, under `$HOME/.codex/private-backups/<backup_id>`, with a private parent directory. The exact ID is in [recovery-validation.json](sec-p1-1b/recovery-validation.json); the local location pointer is `/tmp/sec-p1-1b-backup-location.txt`. Do not add the backup or its raw receipts to Git.

The snapshot includes the complete checkout: `.git`, tracked files, uncommitted/untracked state and ignored files, including build trees, protected binaries and local evidence. APFS copy-on-write copied files into a distinct snapshot; no hardlinked mutable Git clone is used as the recovery backup. **125,439 files/symlinks and 16,185,875,964 logical bytes** were inventoried. Snapshot files outside `.git` were compared to the source; a full restoration to a separate temporary directory matched every snapshot file/symlink receipt. Snapshot and restored Git `fsck --full` both passed.

Recoverability includes more than Git objects:

- All twenty SEC-P1.1 original-report hashes match the saved redaction ledger; the original private generated-file receipt matches its published SHA-256.
- SEC-P1.2 original/rejected source and surviving raw regression/log evidence were copied privately. An additional **2,355 files** passed separate restoration hashes. Reproducible temporary Cargo target trees were excluded from this additional evidence copy; the complete primary ignored tree was already backed up. See the receipt for scope/counts.
- Source, locks/manifests, approved art, original diagnostic explanations and the currently incompatible historical release binary remain preserved. Copies did not mutate originals.

**Limitation:** this is persistent same-device storage, not an independently verified off-device disaster-recovery backup. That is adequate for this disposable-copy experiment; a real migration needs an owner-approved independent copy and another recovery check. Backup confidentiality, retention and future storage access are owner responsibilities. Temporary restoration/test outputs are not the durable backup.

Verified local recovery procedure: leave the original snapshot untouched; copy its `workspace` into a new isolated directory, compare the private file receipt, run `git fsck --full`, verify refs/HEAD/index and restore the separately archived prior evidence. This exact complete-copy restoration was exercised. Do not overwrite the real checkout, or roll back GitHub by republishing sensitive old objects, without a separate coordinated recovery decision.

## 3. Original versus simulated history

The only removal scope is the existing [ten-line SEC-P1.1 pathspec](sec-p1-1/untracking-paths.txt): one exact Cargo target directory and nine exact bytecode files. No generic `target`, `__pycache__`, home-path or secret-string replacement was used. No canonical image metadata, transcript content, acceptance value, author/committer identity or commit message was rewritten.

Installed `git-filter-repo` identifies as `a40bce548d2c`. Fresh local mirrors were made from the verified backup, never from GitHub. A first ordinary local clone was rejected by the tool's fresh-pack safeguard; it was retained and replaced by a new `--no-local` mirror. No `--force` override was used.

A normal filter run removed the selected paths from main but left all original blob exposure reachable through direct tree checkpoints, and remapped remote-tracking namespaces. Its results were rejected as insufficient. The final experiment used an explicitly enumerated **28-ref** scope with:

```text
git filter-repo --invert-paths --paths-from-file <exact-approved-pathspec>
  --prune-empty never --prune-degenerate never --preserve-commit-hashes
  --refs <every snapshotted local ref>
```

`--preserve-commit-hashes` preserves old commit references in messages; it does not preserve changed commit IDs. The explicit scope implies partial filtering and prevents automatic remote-ref namespace conversion/pruning. Old unreachable objects can remain in that experimental store.

`git fast-export` explicitly skips direct tree refs. Therefore a separate isolated helper rebuilt those checkpoint trees using `git ls-tree -z` / `git mktree -z`, omitting only the same approved path set, then updated **only the simulation's** tree refs with expected-old-ID checks. It processed thirteen direct tree refs; six changed. It authored no new artwork, source blob, tag or measurement. All retained tree-file entries were compared with the originals. This helper and full tree/ref/commit mappings are preserved privately for owner review; it is not permission to alter application checkpoint refs in the real checkout.

Final [simulation verification](sec-p1-1b/simulation-validation.json):

| Property | Original → simulation |
| --- | --- |
| Reachable commits | 497 → 497; no pruning. |
| Commit IDs changed | 101; full old/new mapping retained privately. |
| Retained file paths/modes/blobs | Identical across every mapped commit; only approved-path deletions. |
| Commit messages / author and committer metadata | Preserved for all 497 commits. |
| Original signed commits | Seven commits contain signature headers; their cryptographic validity was not checked. No signature-bearing commits remain reachable in the simulated history. Signature loss is a migration consequence, not approval to re-sign original authors' work. |
| Ref names | All 28 preserved in the final simulation; fifteen values changed. |
| Main | `c35a9aed…` → `f4f80319e246fcafa6fe45e3349713dcfa6cd198`. |
| Current tracked files | 7,900 → 5,133; exactly 2,767 selected files removed. |
| Selected historical paths | No longer present in simulated commit history or retained checkpoint trees. |
| Release tags | All three name/commit identities unchanged in this narrow experiment. |

The private commit-map SHA-256 is `ef300e6d792e595a223adb7698319d34d98702cb67086ebf4f87fe4abb140533`. A broader privacy transformation would require new mappings, tests and approvals; these numbers must not be reused for it.

## 4. Size and privacy comparison

[Comparison receipts](sec-p1-1b/size-privacy-comparison.json) use identical byte patterns against **all objects reachable from the snapshotted local refs**, including tree-only checkpoints. Counts are unique blobs, not current files or unique people. A representative path does not enumerate every alias for identical content.

| Metric | Original | Final simulation |
| --- | ---: | ---: |
| Reachable objects | 18,789 | 16,543 |
| Reachable blobs | 11,607 | 9,762 |
| Uncompressed reachable blob bytes | 1,264,295,277 | 587,268,906 |
| Home-path matching blobs / occurrences | 1,560 / 44,437 | 414 / 3,106 |
| Temporary-path matching blobs / occurrences | 249 / 1,878 | 247 / 1,876 |
| External-volume matching blobs / occurrences | 10 / 57 | 10 / 57 |
| Private-key headers / high-signal provider-token forms | 0 / 0 | 0 / 0 |

Reachable blob data decreases by **677,026,371 bytes (53.55%)**. This differs from current-tree file bytes because history and shared blob identities differ. The first default-ref experiment had no blob-byte reduction, illustrating why checkpoints matter.

A fresh reachable-only mirror of the final simulation contains 16,543 packed objects and reports **261,122 KiB** in Git pack storage. The intermediate partial-filter store retains old unreachable objects; its disk usage is not a reclaimed-history result. Original storage includes loose objects and differently compressed packs. No equivalent immediate saving is promised for GitHub, the primary object database, reflogs, caches or clones; no primary garbage collection was performed.

Gitleaks 8.30.1 ran against original and simulated reachable commit histories with `--redact`; both returned the same five known synthetic fixture-token/digest false positives documented in SEC-P1. Finding exit 1 is not a clean zero-findings pass. The independent all-reachable byte scan includes binary blobs and commit/tag metadata and found no high-signal key/token forms. No genuine credential was confirmed; compressed/encoded data, arbitrary assignments and screenshot pixels are not exhaustively cleared by these checks.

One home-path-bearing historical bytecode blob remains under `tools/branding/__pycache__/check.cpython-314.pyc`, outside the nine current-file proposal paths. It was not silently added to scope. Recorded real machine roots, synthetic examples and harmless placeholders require contextual classification; pattern totals alone do not establish that every match is personal information.

## 5. Current-tree untracking proposal — separate approval

Every file in the private SEC-P1.1 receipt was revalidated against current size, SHA-256 and index blob identity:

- Cargo target: **2,758 files / 703,223,935 bytes**; current tree object still `7e8bfa3a3f71af5c1d59b460a4b867e90f70f397`.
- Bytecode: **nine exact files / 131,416 bytes**.
- Total: **2,767 files / 703,355,351 bytes**.

The original probe manifest/lock/source, shared mascot source and reproduction scripts are retained. A fresh build/tests from simulated source succeeded, as detailed below. Current-tree index-only untracking and historical filtering are separate operations with separate effects: untracking preserves local files and does not remove historical blobs. No `git rm`, index removal or untracking was run against the primary repository, and no current-tree cleanup commit was created.

The future index-only operation remains the exact SEC-P1.1 proposal, without force removal, after owner approval and fresh baseline/local-digest checks. Do not expand it to all bytecode or nested targets automatically. Historical-only bytecode needs its own decision.

The existing evidence manifest `docs/reviews/agent-v323-2026-10-10/artifact-sha256.json` references the selected generated `zmij-…/stderr` build log. Preserve the original manifest/log privately and retain a public addendum explaining the intentional omission and original digest. Do not silently recalculate acceptance values or treat the now-absent log as reproducible historical measurement evidence.

## 6. Source, artwork and evidence integrity

All 497 mapped commits were compared in a combined isolated object store: every file-level diff was deletion at one of the exact selected paths. All retained source, required manifests/locks, assets, documentation and historical diagnostic content keep their blob identities and modes. The same file-entry check passed for transformed direct tree refs.

**651 Core Warden/FA-3 input hashes** match simulated checkout copies, including all Blender masters, reference artwork and canonical animation/render assets. The branding guard passes all **69 FA-3 files**. All **14 canonical PNG home-path metadata cases remain byte-identical**. Removing their metadata would change whole-file hashes despite unchanged pixels and requires a separate artwork/evidence decision. No image re-export or blind text substitution was performed.

A comparative Markdown check examined **383 local targets** and found **zero newly missing tracked targets**. Four existing links in TUI reports resolve only to ignored local test/Clippy logs, absent from original Git HEAD as well as the simulated checkout. Their current local copies were backed up; decide whether approved sanitized evidence should be published or document private retention. Do not publish raw logs merely to fix links.

The reference scan also found **29 document/old-commit-abbreviation pairs** referring to commits changed by the simulation. Messages/reports were deliberately preserved; publish a mapping/addendum if cleanup is eventually applied. [Reference receipt](sec-p1-1b/reference-check.json) identifies the documents without sensitive match values. Binary manifests, external attestations and every commit mention in all historical text were not exhaustively repaired.

## 7. Tests and validation

| Check | Result / scope |
| --- | --- |
| Backup snapshot and complete restoration | PASS — all 125,439 file/symlink receipts; two full Git integrity checks; prior originals/raw candidate restoration verified separately. |
| Original reachable-object scan | PASS with privacy findings — all 18,789 reachable objects, including binary blobs and commit/tag metadata. Not a full semantic privacy certification. |
| Exact artifact proposal | PASS — 2,767 current files match recorded sizes, SHA-256 and index identities. |
| Isolated final history / direct trees | PASS — 497 commit mappings, exact retained entries, 28 ref names, no selected-path residue, seven signature losses identified. |
| Simulated Git integrity | PASS — `git fsck --full` on filtered store and fresh reachable-only mirror. Dangling old objects in partial-filter storage are expected, not public clearance. |
| Fresh Rust probe build | PASS — Rust 1.94.0, `aarch64-apple-darwin`, `--locked --offline`, external fresh Cargo target; no tracked target refresh. |
| Fresh Rust probe tests | PASS — 2/2: base64 vectors and upload/placement does not clear/invalidate screen. Not physical terminal or bit-for-bit historical binary reproduction. |
| FA-3 / Core Warden hashes | PASS — guard plus 651 unchanged asset/source hashes; 14 privacy metadata cases intentionally retained. |
| Original/simulated Gitleaks | PASS after known-candidate triage, not zero findings — same five false-positive locations; raw redacted reports remain private. |
| Documentation/evidence reference comparison | PASS for no new tracked Markdown target breakage; existing ignored-log and old-commit references need decisions/addenda. |
| Primary preservation / whitespace | PASS — tracked baseline digests, index, main and public branch/tag values checked; only new report/receipts. Application checkpoint-name churn explicitly recorded, not reverted. |
| Full runtime/native/release suites | NOT VERIFIED in this phase; no behavior change or release acceptance claim. |
| Complete historical privacy clearance | BLOCKED — retained findings, private-context/pixel review and unknown external copies remain. |

Raw scans, tool streams, full commit/tree/private-ref maps and backup file inventories remain outside the public repository. Sanitized comparison, recovery, remote and simulation summaries accompany this report. Tool outputs and exact helper scripts are preserved privately for reproducibility.

## 8. GitHub migration consequences and remaining public exposure

- **PRs/Dependabot:** all six open PR branches need coordinated rebase/recreation onto an approved new base. Do not merge security-incompatible Ratatui/image/Crossterm bot updates to simplify migration. Live PR/head identities can change; requery before an operation. Internal PR refs and cached diffs can retain old objects.
- **Signatures/tags:** seven commit signatures are lost in this simulation. Do not impersonate original signers or claim equivalent verification. The three existing lightweight release tags happen to remain unchanged here; a broader privacy rewrite may change them and auto-source archive identities. Tag updates can trigger release automation, even when the result is a draft.
- **Published releases:** twenty-seven uploaded binary assets remain untouched and are not rewritten by filtering Git. Their embedded paths, actual sources and licensing/security acceptance are NOT VERIFIED here. Auto-generated source archives follow tag identities; preserving old tags may preserve old sensitive history in a broader remedy.
- **Forks/clones:** one fork is confirmed by metadata, and unknown downloads/clones may retain everything. Force-pushing cannot recall them. Contributors must avoid merging/pushing old ancestry back into cleaned branches.
- **Actions/caches:** historical runs, logs, artifacts and caches are separate retention surfaces. No cache/run/artifact deletion was authorized or performed. Old workflow versions at historical tags are not changed by the current draft-only policy.
- **Local checkpoints:** direct tree refs can retain content invisible to ordinary branch-only filtering. Application-generated refs can churn during a session. Decide private retention versus scoped transformation separately, and never mirror-push private checkpoint refs to GitHub.

No confirmed genuine credential was established. GitHub Support coordination is a conditional next step if the owner confirms qualifying sensitive data that remains in server PR refs/cached views and cannot be adequately mitigated otherwise; it is not an automatic service for repository-size cleanup. No support request or contributor message was sent. [GitHub's sensitive-data removal guidance](https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/removing-sensitive-data-from-a-repository) explains limits on clones, PRs, signatures and support eligibility.

## 9. Administrator and owner approvals required

Live rechecks carry forward SEC-P1.4: Dependabot alerts/security updates and private reporting are disabled; main is unprotected; rulesets are empty; no release environments exist. Scanning and push protection are enabled. No remote settings were changed. Administrator action should establish private intake, dependency alerting, practical branch/review/status-check policies, protected release tags and signing isolation.

Before any real operation, obtain explicit approval for:

1. The exact current-tree index-only pathspec, preservation receipts and evidence addendum, independently of rewriting.
2. Whether size cleanup alone warrants rewriting public history; the residual privacy scope, historical-only bytecode, raw evidence and 14 PNG metadata decisions.
3. Exact branch/tag/PR/checkpoint scopes, before/after mappings, signature loss and handling of historical releases/artifacts/source identities.
4. Private durable/off-device backup location, confidentiality/retention, verified restore and owner recovery responsibility.
5. Contributor/PR/bot freeze and migration communications; no automatic messages or bot merges.
6. Any narrowly timed protection exception and expected-old-ref checks for an approved remote migration, followed immediately by restored/enforced protection. No blanket permanent bypass or unattended mirror force-push.
7. Any GitHub Support/cache/artifact deletion request, commit, push, tag change or publication. None is authorized by this simulation request.

## 10. Safe implementation and contributor recovery plan

1. Review this simulation and decide the remaining scope. Keep the backup and raw receipts private. Obtain an independent backup and test the same restoration procedure before shared-state changes.
2. Coordinate a maintenance window and stop writers/bot refreshes/checkpoint creation as applicable. Reinventory HEAD, diffs/index/ignored evidence, live branches/tags/PRs/forks/releases and protections. Any drift invalidates these exact mappings; preserve new work and rerun the proposal.
3. If separately approved, perform current-tree untracking using only the original exact pathspec and index-only removal. Verify every local file remains, source/art/locks are intact, ignores work and evidence addenda are explicit. Review that diff before separately approving a commit.
4. If separately approved, rerun the selected historical transformation in another new clone with current refs, tool identity and exact scopes. Keep retained art/evidence immutable, compare every mapped commit/tree/ref, rerun scans/probe tests and review signature/source-link consequences. Do not copy this simulation's private refs into a public push plan.
5. Present an exact per-public-ref migration plan with expected old IDs, new IDs, tag/release/PR disposition and rollback criteria for final owner approval. Do not use an unattended `push --mirror`; internal/private refs and deletion semantics require deliberate review.
6. After a separately approved migration, verify live refs, retained files, source downloads and protections. Quarantine old clones. Contributors should clone the approved new history, preserve their old local work privately, and reapply only reviewed changes by patches or reviewed cherry-picks; never merge old ancestry or push stale tags wholesale. Recreate/rebase PRs and bot updates deliberately.
7. If validation fails, freeze shared writes and recover into an isolated directory from the verified snapshot first. Decide with the owner whether to repair forward or perform a scoped remote rollback; republishing old sensitive history is itself a risk. Do not overwrite concurrent work or use recovery as automatic force-push authorization.
8. Review residual external objects, releases, PR caches and private backup retention separately. Keep SEC-P1.2/P1.3 release gates open. Returning to Desktop development is an owner scheduling decision, not a release clearance.

Final recommendation: **NEEDS REVISION**. The narrow generated-file simulation is technically validated and recoverable; full privacy cleanup and shared migration still require explicit scope, backup and coordination decisions. Stop here. No primary cleanup, untracking, commit, push, tag, settings mutation, dependency migration or Desktop work begins automatically.
