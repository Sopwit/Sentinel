# SEC-P1.4 — GitHub repository final hardening

Date: 2026-10-12. Baseline: `46013c3323f0c973d1cf05e9023666fd03dba20f` on `main`. Recommendation: **PARTIALLY RESOLVED for repository hardening; BLOCKED for public binary distribution and for starting historical remediation without its separate approval.**

SEC-P1 through SEC-P1.3, local instructions, all seven workflows, community files and security documentation were reviewed. Personal-Brain was unavailable (`brain` absent); the user-supplied checkout was used. This phase makes repository-file improvements only. No runtime, dependency, artwork, remote configuration, history, index or publication change was performed.

## 1. GitHub repository settings assessment

Read-only authenticated GitHub REST requests succeeded after the initial sandbox network attempt failed. The credential had repository admin access, but only GET requests were made. Sanitized values, endpoint response statuses/dates and private response digests are recorded in [github-settings.json](sec-p1-4/github-settings.json). Full headers and PR payloads remain outside the repository. These are time-specific observations, not guarantees about future settings.

| Setting | Observed state | Assessment |
| --- | --- | --- |
| Visibility/default branch | Public / `main`; live remote main equals baseline SHA. | PASS — confirmed remotely, not inferred from a cached tracking ref. |
| Secret scanning | Enabled. | PASS for setting; alerts/coverage NOT VERIFIED. |
| Push protection | Enabled. | PASS for setting; bypass policy/history NOT VERIFIED. |
| Non-provider patterns / validity checks | Disabled. | Observed; owner should assess availability, privacy and desired scope before changing. |
| Dependabot alerts | Endpoint returned 404 with explicit `Vulnerability alerts are disabled.` | FAIL — distinguish this explicit response from a generic inaccessible/unknown 404. |
| Dependabot security updates | Disabled in `security_and_analysis`. | FAIL — scheduled version-update configuration does not enable this setting. |
| Private vulnerability reporting | `enabled: false`. | FAIL — policy link currently cannot provide its advertised private intake. |
| Default workflow token | `read`; workflow PR approval capability false. | PASS — keep read-only and no automated approval. Job-specific writes remain possible. |
| Actions policy | Enabled, all actions allowed, SHA pinning not required. | Partial — current files pin actions, remote policy does not enforce pins. |
| Outside-contributor workflow approval | `first_time_contributors`. | Partial — previously contributing outside users are not all subject to approval. Owner decision required. |
| Classic `main` protection | 404 with explicit `Branch not protected`; branch listing also says false. | FAIL — no classic required checks/reviews/force-push protection observed. |
| Rulesets including parents | Empty list. | FAIL — no branch/tag ruleset protections reported. |
| Required checks/review approvals | None established by classic main protection or returned rulesets. | FAIL — CODEOWNERS does not itself enforce approval. |
| Protected release tags | No tag rulesets returned; three tags inventoried. | FAIL for observed ruleset protection; any control outside queried repository APIs is NOT VERIFIED. |
| Release environments | Zero environments. | FAIL for environment-based signing approval/isolation; secret contents and scopes NOT VERIFIED. |
| Discussions | Disabled. | Confirmed; the issue chooser's dead Discussions link was corrected. |

No settings were enabled/disabled. No scanner alerts, secret values, collaborator credential data or raw vulnerability reports were queried. A setting enabled today does not clear preexisting history or prove that secrets were never bypassed.

## 2. Actions security results

| Workflow | Boundary and result |
| --- | --- |
| Linux CI | Read-only push/PR jobs; format/lint/static analysis, build/test and ASan gates preserved. Quality checkout now uses depth 2 because its format diff needs `HEAD^` or PR merge parents, absent from the default depth-1 checkout. No gate suppressed. |
| macOS CI | Read-only push/PR build/test; checkout credentials disabled, bounded job. PASS static review; no native CI dispatch. |
| Windows CI | Read-only x64 build/test; experimental ARM64 remains explicitly optional/manual. PASS static boundary review; ARM64 is not a required native acceptance result. |
| Rust IPC clients | Read-only Linux/macOS fmt/Clippy/tests with Rust 1.94. Added per-workflow/ref/PR concurrency cancellation for superseded runs. No dependency migration or Windows transport claim. |
| CodeQL | Read-only workflow default; analysis job alone gets `security-events: write` plus read Actions/contents. No signing/provider secrets. Documentation path filters mean it must not be made an unconditional required check without an owner-approved check strategy. |
| PR labeler | `pull_request_target`, contents read/PR metadata write, pinned metadata action, no checkout or shell execution. Preserve this boundary; never add PR-code execution here. |
| Release Build | Tag-only package jobs use read-only contents; draft assembler alone has contents write. Draft flag, current-run artifact selection, unique manifests, checksum validation, duplicate rejection and signing cleanup retained. Draft job label/body now explicitly say unapproved and identify SEC-P1.2/P1.3 blockers. No release produced. |

All external action references use full 40-hex commits: nine unique pins across seven workflows. These are unchanged from SEC-P1's upstream identity verification. Pin syntax and unchanged identities were rechecked; transitive action source/downloads were not exhaustively re-audited. All jobs have timeouts; all checkouts disable credential persistence. No direct PR title/body/comment interpolation into shell, external artifact run-ID promotion, PR-triggered signing or privileged fork-code checkout was found in the reviewed files.

The Linux formatting correction was validated in an isolated synthetic depth-2 Git clone for both `HEAD^` and a merge's base parent. It does not address every checkout/event race or future non-merge checkout strategy; native GitHub execution remains NOT VERIFIED. No fetch of Sentinel was performed locally.

Dependabot PR CI uses public dependencies and no custom secrets. Ordinary fork/Dependabot runs retain the read-only/no-secret boundary; approval should not be mistaken for authorization to expose signing credentials. Metadata labeler token limitations may differ for Dependabot-specific events; diagnose a labeling failure without moving PR code into a privileged event or adding a broad PAT. [GitHub's Dependabot Actions restrictions](https://docs.github.com/en/code-security/reference/supply-chain-security/dependabot-on-actions).

Remaining supply-chain limits: allowed-actions policy permits unpinned future additions; Qt patch wildcards, apt/Chocolatey/Homebrew packages, create-dmg and runner images evolve. Signing secrets are currently referenced at repository-secret scope with no protected release environment. A malicious trusted tag/commit could change its own workflow; draft creation is not a secret-access boundary. Windows dist/CPack payload transfer and native signing/deployment remain SEC-P1.3 gates. Existing Bash strict modes, PowerShell native exit checks and cleanup remain intact; runner loss can bypass cleanup.

## 3. Public documentation and collaboration findings

README, installation/CLI docs and capability pages accurately distinguish implemented services from acceptance. AgentRuntime completion acceptance remains incomplete; Plugins/MCP/Skills are experimental; native Windows Rust CLI/TUI IPC is unsupported; V4-B remains local conditional evidence. README now directly links the blocked dependency/distribution assessments and consolidated register. Root GPL text and THIRD_PARTY_NOTICES are retained; no legal clearance language added.

SECURITY provides private-disclosure guidance and a fallback request for private contact without sensitive details. Actual private reporting is disabled, so administrator enablement is an urgent follow-up; no invented private address or response SLA was added. Public bug/security-documentation forms warn against sensitive material. Security findings must not be posted to public issue templates. The release-check form and PR checklist now link the exact-candidate publication gates and evidence/privacy requirements.

CODEOWNERS names existing `@Sopwit` only and remains unchanged. It identifies ownership, not independent review or enforced approval. Requiring a sole maintainer to obtain their own approving review would obstruct maintenance; choose a practical owner bypass/recovery policy or a real additional reviewer before enforcing independent approvals. [GitHub ruleset guidance](https://docs.github.com/en/repositories/configuring-branches-and-merges-in-your-repository/managing-rulesets/available-rules-for-rulesets).

Existing Contributor Covenant text promises investigation by project leaders but supplies no concrete private enforcement contact. Owner confirmation of the contact/capacity is needed; this phase neither invents an enforcement team nor rewrites community policy. No funding or dedicated support file was found; no funding/support guarantee is inferred. Discussions is disabled, so the issue chooser now links existing documentation instead.

Dependabot already groups Actions version updates and checks `/cli` Cargo weekly, capped at five PRs per ecosystem. This is retained. Broad automatic Cargo grouping/merging is not added: coupled Ratatui/image/Crossterm upgrades already failed compatibility. Review them together in a controlled change rather than treating each bot PR as security clearance. No automatic dependency approval/merge workflow exists in the inspected files.

## 4. Applied safe fixes

1. Linux quality checkout depth 2, preserving its existing formatting gate and read-only credentials.
2. Rust CI concurrency for superseded PR/ref runs.
3. Unapproved release-draft job label and fixed warning body; `draft: true` preserved.
4. Issue chooser's disabled Discussions link replaced by project documentation.
5. PR template gains relevant privacy/evidence, provenance, workflow-boundary and release checks, with not-applicable explanation allowed.
6. README explicitly links SEC-P1.2/P1.3 blockers and this consolidated register.
7. Release process gains evidence-based publication gates, human approval and administrator-protection distinction.
8. Release-check issue template links those gates and warns against private findings.

No artificial reviewers, support commitments, new required approvals or administrative settings were introduced. No existing CI gate was weakened. Changes remain uncommitted for review.

## 5. Consolidated blocker register

Severity is audit prioritization, not CVSS or a legal determination. Each row links detailed evidence instead of duplicating raw findings. **All listed owner decisions remain pending**; no row is a risk waiver.

| ID / severity / scope | Evidence and remediation already attempted | Recommended next step / maintainer decision | Distribution impact |
| --- | --- | --- | --- |
| B01 High — Rust memory safety | [SEC-P1.2 §2–4](SEC_P1_2_DEPENDENCY_SECURITY_REPORT.md#2-advisory-assessment): lru 0.12.5, RUSTSEC-2026-0002; source review narrowed observed triggers but did not prove unreachability. Ratatui 0.30.2 candidate removed affected package but regressed Sixel; reverted. | Approve a compatibility-reviewed upstream fix or bounded backport proposal; do not lockfile-bump unsupported constraints. | TUI/public binary gate remains BLOCKED. |
| B02 High — Rust panic safety | Same evidence: lru 0.12.5, RUSTSEC-2026-0253; patched >=0.18.2, rejected candidate selected 0.18.5. | Same controlled remediation with panic-safety and terminal acceptance; owner selects approach. | BLOCKED; no exploit or waiver established. |
| B03 Low — build maintenance | Same evidence: paste 1.0.15, RUSTSEC-2024-0436, unmaintained; upstream candidate removed it but was incompatible. | Select maintained compatible upstream integration or reviewed temporary substitution/backport. | Open project release gate; not a demonstrated application exploit. |
| B04 High — macOS Rust notices/SDK | [SEC-P1.3 §2](SEC_P1_3_DISTRIBUTION_COMPLIANCE_REPORT.md#2-four-original-notice-investigations): block2 0.6.2, dispatch2 0.3.1, objc2 0.6.4, objc2-encode 4.1.0 lack complete verified originals; exact-revision policy raises SDK questions. Two unrelated MIT gaps fixed. | Obtain exact-revision upstream originals and owner/legal disposition of Apple-derived portions; never fabricate attribution. | macOS Rust distribution BLOCKED. |
| B05 High — Qt / source delivery | [SEC-P1.3 §4/6](SEC_P1_3_DISTRIBUTION_COMPLIANCE_REPORT.md#4-qt-licensing-route-and-actual-linkage): dynamic Qt 6.11.2/Pdf observed; notice subset improved, complete target source/notice/replacement route missing. | Select per-module terms, collect exact payload notices, establish and test matching Corresponding Source delivery; owner chooses mechanism. | Desktop binaries BLOCKED; generic GPL/source URL insufficient evidence. |
| B06 High — Core Warden reference rights | [SEC-P1.3 §5](SEC_P1_3_DISTRIBUTION_COMPLIANCE_REPORT.md#5-artwork-branding-fonts-icons-and-sounds): reference/master/frame hashes preserved; no sufficient original reference grant. | Owner establishes creator/acquisition/derivative/redistribution permission and contributor chain. | Source and binary art clearance BLOCKED; no visual edit authorized. |
| B07 High — FA-3 provenance | Same evidence; 69-file integrity passes, provisional lock/external evidence waived. | Owner confirms copyright/trademark/provenance and review disposition. | Branding distribution clearance BLOCKED; hashes prove integrity only. |
| B08 High — sound ownership | Same evidence; three embedded WAVs lack adequate ownership/license records. | Obtain author/license records for notification/error/success WAVs; owner decides terms. | Desktop sound redistribution clearance BLOCKED. |
| B09 Medium — privacy / raw evidence | [SEC-P1.1 §1–5](SEC_P1_1_PRIVACY_CLEANUP_REPORT.md#1-confirmed-exposure-categories-and-retention-classification): 20 reports safely redacted, raw diagnostics/screenshots and 14 PNG metadata/source literals retained. No genuine credential confirmed by prior scans. | Owner chooses private originals/public derivatives and evidence hash addenda; complete sensitive-context review. | Public source/history exposure persists; privacy clearance BLOCKED. |
| B10 Medium — tracked build outputs | [SEC-P1.1 proposal](sec-p1-1/untracking-proposal.json): 2,758 target files + 9 bytecode files, 703,355,351 bytes; rebuildability established, no removal. | Revalidate exact proposal/receipt, approve index-only removal separately; historical rewrite needs separate scope and backups. | Size/privacy/evidence gate; ignoring or untracking would not remove historical blobs. |
| B11 High — acceptance / binary-assets | [SEC-P1.3 §6–8](SEC_P1_3_DISTRIBUTION_COMPLIANCE_REPORT.md#6-packaging-and-source-distribution-assessment): known V4-B/CW03B mismatch guarded at evaluator; native deployments, version identity, physical terminal/signing incomplete. | Owner selects fresh candidate/target and requires its own native tests, resource/source inventory and signing evidence. | Public binaries BLOCKED; Windows native CLI IPC unsupported. |
| B12 High — repository governance | This phase's [settings receipt](sec-p1-4/github-settings.json): main unprotected, no rulesets, no release environments, first-time-only approval. | Administrator selects practical branch/tag protections, reviewer/bypass/recovery policy and signing isolation. | CI/draft flags cannot enforce human publication or credential safety alone. |
| B13 Medium — security intake/automation | Same receipt: alerts, security updates and private reporting disabled; scanning/push protection enabled. | Administrator enables/verifies appropriate settings and privately triages existing findings. | Reporting/automation gap; not proof of zero alerts. |
| B14 Medium — remaining third-party/toolchain provenance | [SEC-P1.3 §1/3/5](SEC_P1_3_DISTRIBUTION_COMPLIANCE_REPORT.md#1-licensing-audit-and-scope): font/Lucide notices improved; exact icon/font acquisition, target libraries, Rust toolchain terms and Qt vulnerability coverage incomplete. | Owner closes acquisition and actual-payload inventories; evaluate original terms and target security coverage. | Relevant source/package clearance remains BLOCKED/NOT VERIFIED. |

SEC-P1 checksum collision, deployment digest, signing-order, workflow-permission and public-security-form findings have repository-file fixes retained; native behavior is still unverified where documented. Historical audit counts describe their original scan definitions, not a newly measured privacy reduction.

## 6. Manual administrator actions

These require separate owner approval and were not executed:

1. Enable private vulnerability reporting, Dependabot alerts and appropriate security updates; verify private reporting actually works. Keep scanning/push protection enabled. Review bypasses and non-provider/validity options without assuming coverage.
2. Protect `main` against unintended deletion/force-push and select required checks from actual recent successful check contexts. Linux quality/build/ASan, macOS, Windows x64 and Rust matrix are candidates. Avoid unconditional CodeQL/path-filter or optional ARM64 requirements that cannot report on every applicable PR.
3. Select a realistic PR review model for the single maintainer: use real reviewers when available and an explicit narrow owner/recovery bypass rather than inventing a second reviewer or a self-approval requirement. Decide stale-approval handling and apply it deliberately.
4. Restrict `v*` tag creation/update/deletion through a tested ruleset. Define a deliberate, temporary exception process for any later approved historical remediation; never silently weaken protections.
5. Review all-outside-collaborator workflow approval, allowed Actions and required full pins. Existing pin compliance is not remote enforcement. [GitHub Actions settings](https://docs.github.com/en/repositories/managing-your-repositorys-settings-and-features/enabling-features-for-your-repository/managing-github-actions-settings-for-a-repository).
6. Design protected release credential/environment access and human approval appropriate for one maintainer. Move signing secret scope only after reviewing workflow/environment behavior; do not expose it to fork/Dependabot code. Verify release write permissions and human publication authority separately.
7. Confirm community enforcement/private-contact arrangements without publishing sensitive reports or promising unsupported response windows.

## 7. Release gate checklist

The maintained checklist is [RELEASE_PROCESS.md](../release/RELEASE_PROCESS.md#required-publication-gates). All must apply to the exact candidate:

- Dependency advisories and compatibility reviewed and resolved.
- Original notices, Qt/source obligations, artwork/reference/sound rights established.
- Secret/privacy review and historical exposure decisions documented.
- Native build/runtime and applicable terminal/signal acceptance complete.
- Binary/resource/version compatibility and exact package/source/dependency inventories verified.
- Checksums and source delivery tested; signing/notarization or explicit target policy approved.
- Human maintainer publication approval after the above evidence.

**Current result: BLOCKED.** CI passing or a signed draft cannot close legal/privacy/runtime gates. The workflow still creates a draft; its token could technically publish if trusted workflow code were altered. Remote protection and human practice are essential. No workflow, signing, notarization, package build, release draft or publication was triggered here.

## 8. Validation results

PASS is limited to the stated check. [Static receipts](sec-p1-4/validation.json) and [preservation ledger](sec-p1-4/change-ledger.json) support this phase.

| Check | Result |
| --- | --- |
| Prior reports/instructions and complete workflow inventory | PASS — seven workflows; security/community/capability documentation reviewed. |
| Read-only GitHub inspection | PASS — repository settings/protections, environments, tags/branches and open PR metadata obtained. Initial sandbox network failure corrected with authorized read-only network access. No settings mutation. |
| `actionlint -no-color` | PASS — Homebrew actionlint 1.7.12, ShellCheck available; before and after edits. |
| YAML/config static validation | PASS — 13 YAML files parsed; seven workflows/82 steps, nine full action pins, job timeouts, checkout credentials, privileged boundaries and draft invariant checked. Issue/Dependabot configuration parsed; live form rendering/update execution NOT VERIFIED. |
| Checkout-depth regression | PASS — isolated synthetic Git merge/depth-2 clone resolves both intended comparison inputs. GitHub execution NOT VERIFIED. |
| Artifact collection regression | PASS — synthetic three-platform success, corrupt artifact rejection and duplicate filename rejection; existing script executed locally, no real release artifact. |
| Documentation/link checks | PASS for local Markdown targets and referenced heading anchors in changed docs/templates and this report; external endpoints not exhaustively crawled. Disabled Discussions discovered through API and link corrected. |
| Git whitespace/preservation | PASS — `git diff --check`, index unchanged, baseline digests and tracked inventory checked; only named repository-file edits. No deletion/untracking. |
| Runtime/native/release suites | NOT VERIFIED in this phase — not rerun for workflow/documentation changes; prior results are not new acceptance. |
| Remote protection/intake gaps | FAIL — observed disabled settings and absent protections, pending administrator decisions. |
| Public distribution / historical remediation | BLOCKED — unresolved register and separate approval requirements. |

No sensitive values were added to the report or public issues. New content received a focused home-path/private-key/token-pattern check, not a universal secret certification. Full scanner/API/ref/worktree outputs stay outside the repository.

## 9. SEC-P1.1B preparation checklist

**Assessment only; not permission to start or rewrite.** The initial working tree/index were clean. Four owner-created commits after the old audit baseline retain prior security/legal/report changes: `decc657e`, `67951784`, `28178a26`, `46013c33`. They were already present when this phase began; this agent made none. This phase's eight existing-file corrections and new report/receipts remain uncommitted and must be reviewed and durably retained before later cleanup.

Local inventory: 7,896 tracked files and 491 commits reachable from local refs. Local main, cached origin/main and live remote main currently match. Local refs include main, four cached nondefault remote branches, three tags and Codex checkpoint/capture refs; cached branch inventory is incomplete compared with the live 10-branch response. No fetch updated it. Checkpoints can retain old objects even after a future branch rewrite; do not delete them without a scoped retention decision.

The three live tags are `v1.0.0-rc.1`, `v1.0.0-rc.7`, `v1.0.0-rc.8`. Two open PRs were returned: #8 Actions group and #12 rustix update, both against main. The live branch inventory also includes Ratatui/image/Crossterm and other Dependabot branches, even where no open PR was returned. Recheck immediately before migration; do not merge any bot upgrade on the basis of this phase. GitHub reports one fork; its retained content and other clones/downloads are unknown.

Before any separately approved SEC-P1.1B operation:

1. Pause/coordinate writers and automation with the owner. Recheck worktrees, current diffs/index, local/live refs, open PRs and tags. Review/retain this phase's uncommitted changes through an approved process; no automatic commit is authorized.
2. Create owner-approved **private durable** backups of all needed local refs/objects, working-tree/untracked files and relevant ignored evidence. A Git bundle alone omits uncommitted/ignored files; a local bundle alone also omits unfetched remote/PR refs. Test recovery before modifying anything. Move required prior `/tmp` originals, receipts and rejected candidate evidence to that storage; temporary existence is not a backup guarantee.
3. Revalidate the exact SEC-P1.1 proposal and private per-file receipt. The target tree still equals `7e8bfa3a3f71af5c1d59b460a4b867e90f70f397`; 2,758 target files and nine bytecode files remain tracked. No untracking was performed. Preserve source/manifests/locks, canonical art and original review evidence; identify the referenced build-log evidence addendum.
4. Decide separately between current index-only untracking, safe current redactions, and any historical filtering. Specify exact paths/objects/refs, asset metadata policy, original-evidence retention, changed hashes and approval scope. Do not treat this report or the old proposal as rewrite authorization.
5. Plan remote protection exceptions and recovery with the administrator, including branch/tag force-update authorization, PR rebasing/recreation, contributor notifications and old-clone/fork recontamination. No messages were sent automatically. Restore/verify protections after any future approved operation.
6. Account for tags/releases, PR refs, GitHub cached views and forks/downloads. Owner may need GitHub support for applicable sensitive-object cleanup; no guarantee of recalling copies. If genuine credentials are later confirmed, separate revocation from history work.
7. Validate an isolated proposed result before changing shared refs: retained source/art/evidence, index/local file preservation, reproducible build inputs, privacy scans, object/ref scope, source links and release-gate continuity. Require explicit final owner approval for destructive/shared operations.

## 10. Remaining risks and recommendation

Repository-file hardening is reviewable and its scoped checks pass. GitHub settings gaps are now confirmed, not merely assumed. Privacy/history retention, two lru advisories, paste maintenance, original notices, Qt/source/art/sound rights and native release acceptance remain open. No confirmed credential was newly found or claimed absent through this limited review.

Recommendation: **PARTIALLY RESOLVED** for SEC-P1.4; **BLOCKED** for public binary release. Complete the administrator decisions and maintain the consolidated register. Stop here; do not start SEC-P1.1B, dependency migration, Desktop work or publication automatically.
