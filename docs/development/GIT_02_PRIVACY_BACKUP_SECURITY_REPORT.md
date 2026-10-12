# GIT-02 — Privacy, backup and GitHub security

Date: 2026-10-12. Status: **PARTIALLY COMPLETE**. GIT-03 remains **BLOCKED**. The owner cancelled the encrypted disk-image workflow and authorized a plain external directory copy instead. Neither a successful plain copy nor prepared privacy transformations authorize historical rewriting or remote changes.

## 1. Git preflight and GIT-01 reconciliation

Local main is `f920c8abc5aa1de49fe7df372a1bca6a5b4f35fd`, parent `c35a9aedff5832bd0704bbb164f4b25f9a7bff2d`. The owner-created GIT-01-named commit exists, but its actual diff adds only the GIT-01 report, sanitized receipt and evidence addendum. It contains **zero generated-file deletions**. All 2,767 generated files were tracked again when this phase began; there were no staged or unstaged tracked changes, and twelve inherited untracked report/receipt files.

After verifying the original private receipt, all selected local/backup hashes and current index identities, the previously authorized exact **2,767 index-only removals were restored**. Local files remain unchanged; no commit was created. The resulting index has 5,136 entries, rather than 7,903. The owner must review the staged diff and separately authorize any later commit; the existing commit message alone must not be treated as proof of cleanup.

## 2. Privacy preparation

Prepared exact original-to-derivative transformations for **394 known historical blobs**: 374 raw review-evidence blobs and twenty historical report blobs. The [transformation ledger](git-02/historical-transformations.json) records original/new Git blob IDs, SHA-256 values, byte ranges and replacement counts. Only listed personal-home-root spans become `<LOCAL_HOME>`; all other bytes are preserved. The 362 JSON/JSONL blobs parse before and after, with structure and non-path values unchanged; 32 other text blobs are preserved outside those ranges. There are 3,085 replacements.

Original and derivative bodies remain private, outside the repository. Derivatives are prepared candidates, not approved public conversation transcripts. Root replacement does not clear conversation context. The twenty corresponding current report versions already have zero matches for the known personal root, so no additional speculative current-document redaction was applied. Test/production executable literals were not edited. Existing canonical source and measurements remain unchanged.

## 3. Residual historical privacy

R1's 414 home-matching blob identities remain the scoped evidence: the 394 prepared document/evidence transformations, fourteen canonical PNGs, five source blobs and one historical bytecode blob. Three source variants contain the harmless standard Linuxbrew prefix. The two remaining operational/test literals require a reviewed portability/provenance decision. The bytecode finding is private-checkpoint-only, not justification for expanding a public removal scope. Screenshots and recorded conversations require explicit context/retention decisions. No new broad audit was run.

## 4. Canonical artwork policy

Read-only PNG chunk parsing found the known personal root in a `tEXt` chunk in each of the fourteen PNGs; chunk CRCs were checked. [Metadata policy](git-02/png-metadata-policy.json) identifies exact paths, original hashes and affected chunk positions. Originals are retained privately; all canonical images and Blender masters remain unchanged.

Default: retain originals. A separately approved metadata-only derivative must alter only the identified ancillary text, preserve all rendering-relevant chunks, compare decoded dimensions/channels/pixels exactly, and record original/new whole-file hashes, affected manifests and evidence addenda. No derivative artwork was created. Core Warden reference, FA-3 and sound redistribution rights remain unresolved; privacy preparation does not clear copyright or distribution obligations.

## 5. Changed backup decision and rollback

The owner explicitly cancelled image encryption and authorized this destination:

`/Volumes/NeuralSilicon/Sentinel_Backup_2026-10-12/Sentinel`

This is a **plain directory copy**, not an independently encrypted backup. Read-only device checks identify a writable external USB APFS SSD, with 373,762,306,048 bytes of available container space at preflight. The source and destination volumes are distinct. The destination did not exist; no existing backup directory is overwritten.

Rollback inspection found no disk image at either attempted image filename, no attempt-created mount and no remaining identified password process. Disk Utility has no pending creation sheet. Two attempt-only, non-secret dialog scripts were moved into private quarantine, rather than deleted. Unique privacy analyses, original/derivative evidence and logs were retained. The existing SEC-P1.1B private recovery snapshot was untouched. The unrelated iOS Simulator image/mount was left alone. No new encryption workflow will be started.

The copy includes the complete project directory, `.git`, hidden files, inherited/new untracked reports, ignored build outputs, Blender masters, Core Warden and local evidence inside the project. Evidence stored outside that source directory remains in its existing private locations; this project-only copy is not a claim that every external temporary evidence directory or the internal snapshot has been independently backed up.

<!-- BACKUP_VERIFICATION_START -->
Plain-directory copy completed; **content and Git checks PASS, full metadata equality NOT ACHIEVED**. The final project copy contains **125,513 regular files**, **6,287 directories** and **0 symbolic links** (131,800 descendant entries), totaling **16,186,361,160 logical regular-file bytes**. Counts exclude the separate verification records stored alongside `Sentinel`.

Native `ditto --rsrc --extattr --acl` completed successfully. All source/destination relative paths and types, every regular-file SHA-256, permission modes, owner/group IDs, modification/creation times and file flags matched. There were no symbolic links in this snapshot. All compared extended attributes match except seven OS-managed `com.apple.provenance` cases (four destination-only, three differing values); macOS regenerated these after source values were restored. Original and destination values are retained privately in verification records. This exception is explicit and is not described as full metadata verification. The first comparison detected creation-time and macOS attribute differences; native destination-only metadata corrections restored 129,159 creation times and 78 original attributes. Four copy-generated provenance values were retained privately before removing those extra attributes from the new copy. The subsequent comparison confirmed the timestamp/quarantine corrections but retained the seven provenance exceptions. Source inventory was unchanged across the copy. Both Git repositories passed `git fsck --full`; HEAD, all branch/tag/checkpoint refs and porcelain status matched. File devices differ, so the backup is independent of the source and contains no cross-device hard links. The original internal snapshot remains untouched.

This report was finalized after the initial successful comparison and synchronized into this newly created backup; final inventories rechecked all other entries unchanged and independently verified the updated report hash and metadata. Private complete inventories, comparison logs and Git receipts are in `Sentinel_Backup_2026-10-12/verification/`, outside the public project. No existing backup or unrelated SSD content was overwritten. The copy is not an isolated restore test and is **not encrypted**; no encrypted recovery readiness is claimed.
<!-- BACKUP_VERIFICATION_END -->

The encrypted-backup policy was superseded by the owner’s explicit acceptance of the plaintext external project copy. Encryption remains cancelled. The copy is not encrypted and does not establish complete disaster-recovery readiness. GIT-03 remains BLOCKED by the outstanding scope, recovery-coverage and migration authorization decisions described in the closeout below. No secret, password or key is stored in this report or repository.

## 6. Verified GitHub settings and recommendations

Read-only API results are in [GitHub validation](git-02/github-readonly-validation.json). Secret scanning and push protection are enabled. Dependabot alerts return explicit disabled status; security updates and private reporting are disabled. Main is unprotected, rulesets are empty and release environments are absent. Default workflow token permissions are read-only and workflow PR-review approval is disabled; allowed Actions are unrestricted and server-side SHA pin enforcement is off. No setting was changed.

For the single maintainer, propose a main PR requirement with **zero mandatory approving reviews**, resolved conversations, blocked deletion/force pushes and a documented owner emergency recovery/bypass route. Do not require self-review or CODEOWNER approval from the author. Initially select the observed successful `rust (ubuntu-latest)` and `rust (macos-14)` checks from GitHub Actions after confirming stable PR names/triggers. Other observed Linux/macOS/Windows/ASan and quality checks currently fail at cached public main; fix and demonstrate them before adding them as required. CodeQL is path-filtered and must not be permanently required for docs-only PRs without an always-emitted gate. Experimental ARM64 is not a mandatory PR check.

Protect release tag creation/update/deletion with an owner-controlled ruleset and a separately logged migration exception. Enable Dependabot alerts, security updates and private reporting; retain weekly Actions/Cargo configuration without automatically merging dependency PRs. Place signing credentials in protected release environment secrets and reference the environment in each signing job. The current workflow has no environment binding; creating an environment alone provides no isolation. A single owner may use an explicit manual approval with self-approval permitted, or appoint a separate reviewer; do not enforce an impossible independent review. Draft assembly is not publication clearance.

Recommendations follow [GitHub branch protection documentation](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-protected-branches/about-protected-branches) and [environment documentation](https://docs.github.com/en/actions/reference/workflows-and-actions/deployments-and-environments). Exact settings, bypass policy and workflow changes require separate owner approval; none were applied.

## 7. Existing releases and PRs

The read-only inventory reconfirmed ten public branches, three tags, six open PRs (#8, #12–16), one known fork and three published prereleases with 27 uploaded assets. Asset names, sizes and GitHub digests match the earlier inventory. Public main remains `c35a9aed…`; local GIT-01-named commit is not on that live main. No fetch, push, merge, retag or release change occurred.

Confirmed rc.8 mismatch: its tag source still declares application version rc.7, explaining eight rc.7-labelled uploaded filenames and the numeric 1.0.0 package convention. The prior three Linux archive inspections remain bounded evidence, not clearance of the other 24 assets. Proposed owner-reviewed notice: “This historical prerelease remains unapproved for binary distribution. The rc.8 tag source declares application version rc.7; asset filenames reflect that value. Existing binary identity and licensing/security clearance are not established by the tag label. Original assets and digests are preserved.” Do not silently relabel or replace binaries.

Preserve existing releases/tags/evidence, keep dependency PRs unmerged and coordinate PR migration only after explicit history authorization. Existing binaries remain NOT CLEARED. SEC-P1.2/P1.3 blockers remain open.

## 8. Exact proposed GIT-03 specification

[Migration specification](git-02/git03-migration-spec.json) consolidates A–F without treating pending choices as approvals:

- **A:** Previously authorized current-index scope is restored; review/commit remains a separate owner action.
- **B:** Historical generated removal is proposed only for the original ten-line exact pathspec and private receipt, with no generic cache glob or private-only bytecode expansion.
- **C:** Exact byte-range transformations are the 394 entries in the ledger. Privately retain originals and derivatives; approve conversation/screenshot retention and historical code policy separately. Keep all timing/assertion/acceptance bytes outside identified root spans.
- **D:** Retain fourteen PNG originals and all artwork/masters unchanged unless separate metadata/evidence authorization is granted.
- **E:** Explicit ten public branch identities and three retained tag identities are recorded with expected-old remote SHAs. Use current local main as the proposed source, subject to the owner resolving the pending index commit. None of the 394 prepared privacy blobs are reachable from the three tag histories in the checked snapshot; this does not clear unrelated tag content. Preserve releases and coordinate the six PRs; do not alter fork histories.
- **F:** Require recovery readiness, a reviewed isolated candidate, per-commit/ref maps and explicit administrator approval. Revalidate every expected-old remote ID immediately before the maintenance window; abort on drift. Update only separately approved explicit public refs with expected-old leases. Never unattended mirror force-push or publish remote-tracking/private checkpoint refs.

There are 498 reachable local commits and seven signature-bearing commit headers; signature validity is unverified. A new candidate must enumerate changed IDs and lost signatures rather than reuse the earlier 497-commit simulation map. Do not re-sign for original authors. Preserve private checkpoints separately, coordinate app writers and never push their tree refs.

Before any migration: apply the now-approved plaintext backup policy and resolve its recovery-coverage limitations; approve exact C/D retention, B/E/F scope; stop writers/bots; capture current work and refs; generate and verify an isolated candidate; compare source/artwork/evidence, mappings, signatures, links and focused privacy results; review exact update commands. A real shared-history operation requires another explicit authorization. On failure stop and recover locally, not by republishing old sensitive objects automatically.

## 9. Validation and preservation

The scoped preparation verifies exact derivative bytes/checksums, JSON structure/non-path values, PNG chunk integrity and unchanged original asset hashes. GitHub configuration was read, not mutated. No source change requires new runtime tests. GIT-01's fresh probe build and 2/2 tests remain evidence for the unchanged retained probe, not new GIT-02 physical-terminal or release acceptance.

Backup verification records, full path/hash inventories, logs and sensitive receipts stay outside the public repository. This phase does not erase historical exposure, recall clones/forks/downloads, or establish whole-corpus conversation/pixel clearance. Final project-copy verification must compare every relative path/type, regular-file hash, link target and supported metadata; Git integrity, HEAD, refs and status must match.

## 10. Remaining owner decisions

At the earlier preparation checkpoint, GIT-02 was partially complete. The closeout below supersedes its pending commit and encryption-policy status; historical privacy/admin approvals remain pending. The plain external project copy is explicitly approved; encryption is cancelled. Decisions still needed: actual GIT-01 deletion commit, historical B scope, C evidence/source/screenshot retention, D PNG metadata policy, private original custody/retention, E release notices/PR coordination and F protection/bypass/migration authorization. No GIT-03 work begins automatically.


## 11. Final owner-decision closeout — 2026-10-12

**GIT-02 bounded preparation is complete; GIT-03 real migration is BLOCKED.** Earlier sections retain their chronological observations. This section supersedes their pending GIT-01 commit, encrypted-backup gate and old candidate identities. No new subphase is introduced.

The actual GIT-01 deletion commit is `d72ac588775a6ab84db5e60100e0d341f3280336`, parent `f920c8abc5aa1de49fe7df372a1bca6a5b4f35fd`. Its complete diff contains exactly 2,767 approved deletions and no other changes. All 2,767 local files still match the original private SHA-256 receipt. The index and tracked working tree are clean; 5,136 files remain tracked. No additional cleanup commit is requested.

The owner accepts the existing **plaintext** external project backup. Encryption is permanently cancelled for this workflow unless the owner changes that decision; no encrypted-image requirement is reintroduced. A fresh read-only verification checked all 131,800 descendant paths/types, 125,513 regular-file hashes and 16,186,361,160 logical bytes against the final backup receipt. Backup Git integrity passes and its HEAD/refs/status still match that receipt. The seven documented OS provenance attribute differences remain explicit non-content metadata exceptions; full metadata equality is not claimed or newly re-audited. The original internal recovery receipt remains unchanged.

The external snapshot retains HEAD `f920c8abc5aa1de49fe7df372a1bca6a5b4f35fd` with the cleanup staged. It does **not** contain the subsequent `d72ac588` commit or these closeout documents. It was not overwritten. Private original evidence, prepared derivative bodies and the internal recovery snapshot outside the project were not copied to this external project directory and are not counted as independently backed up. Plaintext allows anyone with storage access to read the copied history and local evidence; no encryption or complete isolated disaster-recovery readiness is claimed. Recovery of the staged cleanup from the old snapshot is possible, but recovery of the exact later commit/current untracked state is a separate coverage gap.

Defaults are now consolidated: preserve all original Core Warden/Blender/FA-3 assets, fourteen PNG metadata records, historical release assets/tags, raw private evidence, screenshots and conversations. No blind replacement of executable historical source is approved. The exact 394-entry ledger, 3,085 byte-span replacements and original/derivative hashes remain unchanged and pending approval; no derivative bodies were published. Prepared text is not contextual privacy clearance.

The [fresh GIT-03 report](GIT_03_FINAL_MIGRATION_REPORT.md) and [public ref map](git-03/public-ref-map.json) supersede old simulation mappings. Only the original generated-path scope was simulated. Main’s retained tree is byte-identical; 499 mapped commits include 103 changed IDs and seven removed signature headers. Ten public branches and three tags were isolated without private checkpoints or remote-tracking refs. All three tag identities remain unchanged. Probe tests pass 2/2; 651 artwork hashes match. The candidate still contains 413 home-path-matching historical blobs. No primary history or remote was modified.

The read-only GitHub refresh still shows six open dependency/Actions PRs, three prereleases with 27 assets, disabled Dependabot alerts/security updates/private reporting, no main protection/rulesets/release environments, and enabled secret scanning/push protection. No setting or PR was changed. Owner decisions remain: generated-history migration and PR coordination; exact treatment of the 394 text derivatives, screenshots/conversations and historical executable literals; private evidence custody and external coverage; acceptance or closure of the newer-commit backup gap; precise administrator protection/bypass settings. Original artwork/releases remain preserved by default. Plaintext acceptance grants no rewrite permission.


## 12. Owner-approved GIT-03 preparation decisions

The owner approved continued preparation within GIT-03, keeping the exact generated scope and preparing 394 historical text derivatives subject to conversation/context review. Artwork including fourteen PNG metadata cases, releases/tags/assets and raw private originals remain preserved. Encryption stays cancelled. A narrowly scoped plaintext supplement is prepared, but writing its new destination requires approval. Exact security payloads and a sole-maintainer emergency/re-lock policy are proposed only; no settings or shared history mutations are authorized.

The [updated GIT-03 final report](GIT_03_FINAL_MIGRATION_REPORT.md) supersedes the prior generated-only candidate IDs and commands. Its all-394 private evaluation candidate passes exact tree/byte-span/hash checks and 2/2 probe tests, leaving nineteen home-path-matching historical blobs. Three live JSON records retain conversation/tool/session context; no sanitized derivative is cleared for shared publication. Supplemental recovery coverage and contextual decisions remain open. GIT-02 bounded preparation stays closed; GIT-03 is NO-GO for real migration. Existing internal/external backups are preserved and plaintext limitations remain explicit.


## 13. Approved supplemental backup verification — GIT-03

The owner approved the exact new plaintext supplemental destination. Its [verification receipt](git-03/supplemental-backup-verification.json) confirms the latest d72 Git state, exact preparation documents, listed original/private recovery evidence and final candidate/maps are externally copied with file hashes and symlink targets verified. An isolated combined base-plus-supplement restoration passes full file-content and Git HEAD/ref/status/fsck checks. Existing internal and external backups remain preserved. One supplemental symlink group-ownership exception is explicit; the base’s seven provenance exceptions remain unchanged. No encryption or complete machine-wide disaster-recovery claim is made. Final closeout records are retained separately without overwriting the initial payload. GIT-03 remains NO-GO for shared history pending contextual publication decisions and explicit remote/administrator authorization.
