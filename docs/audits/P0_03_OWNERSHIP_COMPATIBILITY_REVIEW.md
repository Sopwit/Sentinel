# P0-03 — Ownership and Compatibility Decision Review

Date: 2026-10-16. **Review prepared; maintainer decisions remain pending. P0-03 and AC-P0-04 are not complete.** This document proposes engineering defaults, not staffing, policy adoption, permissions or publication approval.

## Baseline and scope

Verified remote main through read-only `git ls-remote origin refs/heads/main`: `e71cca4e958109e0825b6277072bd1235e77e60a`. The dedicated branch `docs/p0-03-decisions` and worktree `/private/tmp/sentinel-p0-03-decisions` start at that exact commit. Original checkout remains on its existing branch at `0245e5fa94584a357a6b1de01344f69b577e0e41`; it was not switched or edited. Remote verification changes no refs. Personal-Brain was absent on PATH, so no project brief was available; the supplied repository and verified remote identity resolved the project.

Read [AGENTS](../../AGENTS.md), [architecture](../development/ARCHITECTURE.md), [runtime](../concepts/AGENT_RUNTIME.md), [security](../security/SECURITY_MODEL.md), [building](../development/BUILDING.md), [testing](../development/TESTING.md), [P0.1](P0_1_CURRENT_STATE_AUDIT.md), [P0.2](P0_2_ROADMAP_RECONCILIATION.md), [matrix](../roadmaps/ACCEPTANCE_MATRIX.md), [decision register](../roadmaps/ACCEPTANCE_DECISIONS.md), [backlog](../roadmaps/GITHUB_ISSUE_BACKLOG.md), release gates and relevant source below. Historical observations retain their original candidate/date; they are not rerun evidence at this baseline.

Only this report is authored. Shared roadmap/register files, historical audits, production code, tests and other worktrees are unchanged. P1-01, P1-05, P1-04.4 and parallel documentation-consistency work are not modified. No runtime tests, model/download activity, commits, pushes, merges, issues or PRs were performed.

## Confirmed direction preserved

The ten adopted decisions retain their exact IDs and status. None is reopened or replaced by a pending implementation recommendation:

| ID | Preserved adopted direction |
| --- | --- |
| AD-A01 | Independent cross-platform open-source Sentinel under sentinel-projects |
| AD-A02 | Optional Ro-ASD bundling; model acquisition requires user choice |
| AD-A03 | C++20/Qt 6/QML and shared runtime architecture |
| AD-A04 | Desktop, Quick Panel, tray, Rust CLI/TUI and supported platform interfaces |
| AD-A05 | Ollama, llama.cpp, LM Studio and extensible compatible providers |
| AD-A06 | Passive/Assisted priority; Proactive later |
| AD-A07 | Hardware-adaptive resource policy |
| AD-A08 | System Assistant, Developer Agent and Personal Assistant roles |
| AD-A09 | Existing components and one runtime/router/tool execution authority |
| AD-A10 | Strict permissions/isolation and truthful accepted completion |

## Runtime authority and hotspot ownership

The following are source observations and proposed responsibility boundaries, not newly approved role assignments. Source symbols are the stable anchors; all links refer to this review candidate.

| Boundary / source evidence | Responsibility and proposed follow-up |
| --- | --- |
| [Desktop bootstrap](../../apps/sentinel-desktop/bootstrap/ApplicationBootstrapper.cpp), `ensureBackgroundDaemon` | Desktop owns presentation and native activation. It may launch a detached daemon that outlives the UI; this is distinct from an in-process fallback. Quick Panel shares DesktopRuntimeClient. Reconcile startup wording in separate P0-04 work. |
| [DaemonService](../../apps/sentinel-daemon/service/DaemonService.cpp), `initialize` | Claims endpoint before opening stores; composes ApplicationControllerBuilder and shared controller. Daemon/IPC role owns process/profile lifecycle, not another domain runtime. |
| [ApplicationController](../../core/src/app/ApplicationController.cpp), constructor; [AgentRuntime](../../core/src/agent/AgentRuntime.cpp); [AgentLoop](../../core/src/agent/AgentLoop.cpp) | Runtime role owns execution, run/grant stores and accepted-final semantics. Controller is a composition hotspot: future focused extraction must preserve observable behavior and contracts, not replace the runtime. |
| [ModelService](../../core/src/model/ModelService.cpp); [LocalModelRuntime](../../apps/sentinel-daemon/service/LocalModelRuntime.cpp), `ensureRunning` | Providers/models role owns IModelRouter, immutable active ModelBinding and provider capability/identity. LocalModelRuntime is a process helper; successful /health is not proof of requested model identity. |
| [ToolExecutionGateway](../../core/src/runtime/ToolExecutionGateway.cpp); [ProcessExecutor](../../core/src/runtime/ProcessExecutor.cpp) | Security/runtime role owns descriptor validation, approval/resource authorization, filesystem and process boundaries. No client creates a parallel executor or converts natural language heuristically to shell commands. |
| [DaemonIpcServer](../../apps/sentinel-daemon/service/DaemonIpcServer.cpp), `startServer`, request dispatch and shutdown; [canonical schema](../../protocol/ipc-v1.json) | IPC role owns versioned bounded transport and authoritative sessions; schema/generators remain single source. Hotspot changes require generated-contract and surface parity checks. |
| [DesktopRuntimeClient](../../apps/sentinel-desktop/src/DesktopRuntimeClient.cpp) | Desktop role owns remote projections, reconnect and pending UI mutations, never independent run completion. Split only focused presentation/transport concerns with parity evidence. |
| [StandardPathProvider](../../core/src/platform/StandardPathProvider.cpp); [daemon main](../../apps/sentinel-daemon/main.cpp) | Persistence/platform role owns explicit path/profile identity, migration and backup semantics. Settings, memory, history, conversations, runs and grants remain separate Qt SQL/interface owners. |

Retained static catalog/router, metadata task services and null/test adapters are not additional execution authorities. Their cleanup should follow consumer inventory and targeted tests under EP-P2/EP-P5, not a broad rewrite required by this review.

## Legacy compatibility and profile decision

The [legacy dispatcher](../../apps/sentinel-cli/main.cpp) exposes only the commands below. No actual external consumer inventory exists in this review: scripts, installers, distro hooks, documentation users and offline users remain **unknown**. Search results inside the repository cannot establish absence of external consumers. Maintainer must obtain usage/deprecation feedback before retirement.

| Existing legacy command | Source behavior | Rust/daemon counterpart and compatibility gap |
| --- | --- | --- |
| `chat <prompt>` | [ChatCommand](../../apps/sentinel-cli/commands/ChatCommand.cpp), `executeChatCommand`: local builder/controller; 30-second timer still returns zero | Rust `chat` uses daemon sessions/output/cancellation semantics. Not an argument/output/exit-compatible drop-in. No legacy Agent/task entry should be added. |
| `model list` | [ModelCommand](../../apps/sentinel-cli/commands/ModelCommand.cpp), `executeModelCommand`: local Ollama discovery, bounded wait and nonzero discovery failure | Rust `models` queries daemon ModelService across configured providers; output and scope differ. |
| `model pull <name>` | Same source: explicit Ollama acquisition; pull callback prints failure then returns zero | Rust [command enums](../../cli/crates/sentinel-cli/src/main.rs) expose model current/select, not equivalent pull. No silent model download or removal is approved. |
| `status` | [StatusCommand](../../apps/sentinel-cli/commands/StatusCommand.cpp): local paths/settings diagnostics, no daemon health handshake | Rust `status` is daemon connection/runtime status; `doctor` is broader diagnostics and requires daemon authority. Preserve an explicitly defined offline diagnostic use case. |
| `config get/set` | [ConfigCommand](../../apps/sentinel-cli/commands/ConfigCommand.cpp): direct settings keys ollamaEndpoint/routingMode/appLanguage | Rust provider endpoint/select offers partial daemon mutation coverage; no equivalent general config interface. Config mutation requires an authority/profile decision before migration. |

Recommended PD-D05 choice: temporary bounded compatibility, eventual daemon-backed mutations and diagnostic-only offline retention where consumers require it. **This is a target, not a claim that current code is diagnostic-only.** All existing commands remain untouched. P1-05 exit correction remains independently tracked; EP-P5 owns approved migration/deprecation implementation.

Profile risk is concrete: daemon sets a named application identity and nonportable profile directory before building services; legacy CLI uses StandardPathProvider without that profile override or explicit app metadata initialization. Its settings follow QStandardPaths, while default memory/history paths include a shared Documents/Sentinel location. Portable paths use the executable directory. Therefore neither shared-profile equivalence nor safe concurrent access can be assumed. A custom socket changes endpoint selection, not necessarily storage identity. `--profile-name` changes daemon QStandardPaths identity; portable mode must still be reviewed for shared executable-directory collisions.

Recommended ownership contract awaiting approval: one daemon writer/runtime authority per explicit profile and endpoint; separate local Desktop presentation preferences; legacy retained diagnostics use an explicitly inventoried isolated profile and do not mutate active daemon stores. Any one-time migration requires source/destination manifest, backups, schema/version compatibility, no concurrent writers, consent, rollback and validation before switching. This contract is not enforced by documentation and requires separately scoped implementation. Do not assume an endpoint lock alone locks every possible shared database path.

Retirement gates: inventory exact consumers/argv/stdout/stderr/exit expectations; choose supported commands and transition window; establish daemon parity and offline diagnostics; test failure/cancel/profile/upgrade recovery; publish approved deprecation guidance; separately authorize removal. Unknown consumers remain a risk until resolved.

## Fourteen pending decisions

Every row below has status **Pending Decision**. Defaults are recommendations only. Direct acceptance edges are copied from the unchanged matrix in the next section; additional downstream effects are identified separately.

| Decision ID / subject | Alternatives | Recommended default | Technical trade-offs and risks |
| --- | --- | --- | --- |
| PD-D01 — Candidate and release scope | Clean reviewed commit with explicit capability/platform manifest; or immutable dirty-diff manifest for investigation. | Use verified main as review baseline; require a separately approved clean release candidate. | Clean commits simplify reproduction; dirty manifests preserve ongoing work but complicate payload identity. No platform is accepted merely because it builds. |
| PD-D02 — Models and exact runtimes | One existing local tool-capable model first; multiple sizes/backends; explicitly authorized cloud subset. | Start with an already available user-selected local model; record digest, quantization, runtime and parameters. | Narrow first coverage reduces cost but leaves other providers blocked. Do not download resources or infer identity from endpoint health. |
| PD-D03 — Hardware and environment availability | Physical native systems; approved VM for non-GPU IPC; external evidence contributor. | Confirm Fedora KDE Wayland x86_64, Apple Silicon and Windows x86_64 access per claimed cell; unknown access stays unavailable. | VM IPC evidence is useful but cannot establish GPU/native consent, shortcut or physical terminal behavior. No testers are assigned here. |
| PD-D04 — Calibrated performance budgets | Per-tier absolute budgets; baseline-relative regression limits; both predeclared. | Keep Pending Calibration until PD-D02/03 profiles and measurement windows are adopted. | Adaptivity needs measured admission/hysteresis. No new numeric threshold is proposed; existing fixed security limits and repeat counts remain unchanged. |
| PD-D05 — Legacy C++ CLI disposition | Immediate retirement after consumer review; daemon-backed compatibility shim; bounded diagnostic retention. | Retain existing commands pending inventory; target diagnostic-only retention plus daemon migration of supported mutations, subject to approval. | Current chat/model/config mutation paths remain unchanged and are not certified safe. Retirement risks scripts; indefinite retention risks divergent profiles and task authority. P1-05 exits remain separate. |
| PD-D06 — Ro-ASD packaging and support | Independent optional package; app bundled without weights; defer integration. | App-only optional integration, exact Sentinel version lock, no weights or implicit download; verify distro/repository support owners first. | Bundling adds installer/update/support obligations and cannot bypass rights/security gates. Independent Sentinel releases must not require a Ro-ASD image. |
| PD-D07 — Windows trust and isolation | Same-user named pipes with explicit session policy; narrower single-session policy; defer terminal release. | Keep Windows terminal unavailable until peer/ACL/session and sandbox enforcement are implemented and natively evidenced. | Deferral narrows shipped scope without dropping portability. A connected pipe alone is not authorization; unsupported confinement must fail closed. |
| PD-D08 — TUI and optional integration scope | Enable commands individually after daemon contracts; explicitly exclude unsupported commands from initial shipped scope. | Retain unavailable states for unsupported TUI operations and optional OCR/WebUI/ACP/Copilot claims. | Smaller scope is truthful but leaves findings open. Core helper classes do not establish terminal support; exclusions must be approved rather than silently dropping acceptance rows. |
| PD-D09 — KDE shortcut, clipboard and Linux launch | KDE native adapter versus supported portal; reviewed permission-scoped GUI broker versus no clipboard; supervised launcher versus defer. | Use existing platform adapter for shortcut work; retain truthful clipboard/launch unavailability until enforceable policy approval. | Broker expands IPC and GUI trust; portal/native coverage varies by compositor. An opener acknowledgement does not prove successful or authorized action. |
| PD-D10 — Provider ownership and hard stop | Cooperative in-process drain; isolated supervised provider process for bounded hard stop. | Distinguish Sentinel-owned child handles from externally managed endpoints; drain safely, never kill external runtimes. | Cooperative in-process code can remain Stopping without a finite bound. Process isolation costs lifecycle/IPC work but is required before promising forced termination; retain active bindings/dependencies. |
| PD-D11 — Named humans and review capacity | Single maintainer with explicit self-review/recovery record; delegated scoped reviewers with verified access. | Keep roles unassigned until actual humans accept them; record review independence limits honestly. | CODEOWNERS is evidence of review routing, not approval, staffing, admin rights or a fictional second reviewer. External evidence needs candidate/profile identity. |
| PD-D12 — Memory/audio/evidence retention | No raw audio retention by default; explicit bounded opt-in; policy-specific persistent memory. | Synthetic evidence and minimum retention; approve consent, deletion/export/backup and voice fallback visibility before related acceptance. | Less retention reduces disclosure risk but limits debugging; opt-in needs revocation and backup semantics. Conversation persistence does not automatically authorize semantic memory. |
| PD-D13 — Dependency/rights/privacy/admin release dispositions | Compatible remediation versus reviewed upstream migration; proven rights/source route versus withhold distribution. | Keep B01–B14 open until exact-candidate evidence and responsible rights/admin decisions exist. | Builds/signatures do not prove license, privacy or governance clearance. No current advisory scan, repository-protection verification or rights waiver is claimed. |
| PD-D14 — Mode mapping and test resource policy | Map current Chat/Agent to bounded policy profiles; add explicitly enforced read-only inspection profile later. | Specify Passive inspection/conversation and Assisted user-initiated tools against actual policy interfaces; leave Proactive later. | Names alone do not enforce read-only behavior. Unknown or denied capability must remain non-success; do not invent new existing enums or permissions. |

## Decision dependency map

These are exact **direct** decision-to-criterion dependencies from matrix rows; criterion-to-criterion and phase prerequisites still apply. This table adds no acceptance thresholds or edges to the shared matrix.

| Decision | Direct affected acceptance criteria |
| --- | --- |
| PD-D01 | AC-P0-01, AC-P0-04, AC-P1-02, AC-P6-06, AC-P10-01, AC-P12-01, AC-P12-02, AC-P12-03, AC-P12-04, AC-P12-05, AC-P12-06, AC-P12-07, AC-P12-08, AC-P12-09, AC-P12-10, AC-P12-11, AC-P12-12, AC-P12-13, AC-P12-14, AC-P12-15 |
| PD-D02 | AC-P0-03, AC-P1-02, AC-P2-03, AC-P3-01, AC-P8-02, AC-P10-02, AC-P10-03 |
| PD-D03 | AC-P0-03, AC-P1-02, AC-P2-03, AC-P2-04, AC-P4-01, AC-P5-02, AC-P6-03, AC-P6-04, AC-P6-06, AC-P6-07, AC-P9-02, AC-P10-01, AC-P10-02, AC-P11-03, AC-P12-16 |
| PD-D04 | AC-P0-03, AC-P1-02, AC-P1-03, AC-P2-03, AC-P4-01, AC-P5-04, AC-P6-06, AC-P8-01, AC-P10-02 |
| PD-D05 | AC-P0-04, AC-P1-07, AC-P5-04 |
| PD-D06 | AC-P0-04, AC-P7-01, AC-P12-15 |
| PD-D07 | AC-P5-02 |
| PD-D08 | AC-P6-05, AC-P8-01, AC-P10-03, AC-P11-02 |
| PD-D09 | AC-P6-07, AC-P6-08 |
| PD-D10 | AC-P1-03, AC-P3-02, AC-P5-04, AC-P11-01 |
| PD-D11 | AC-P0-01, AC-P0-04, AC-P12-18 |
| PD-D12 | AC-P8-02, AC-P8-03, AC-P10-01 |
| PD-D13 | AC-P7-02, AC-P12-01, AC-P12-02, AC-P12-03, AC-P12-04, AC-P12-05, AC-P12-06, AC-P12-07, AC-P12-08, AC-P12-09, AC-P12-10, AC-P12-11, AC-P12-12, AC-P12-13, AC-P12-14, AC-P12-15 |
| PD-D14 | AC-P0-04, AC-P1-02, AC-P9-01 |

Recommended decision sequencing (engineering dependency proposal, not an adopted schedule):

1. PD-D11 establishes actual accountable humans and review capacity; PD-D01 freezes candidate/capability scope. P0-01 adoption is the existing backlog prerequisite for P0-03 closure.
2. PD-D05 chooses CLI/profile disposition; PD-D10 chooses shared provider/process ownership contract. Resolve contracts before P3/P5 closure without making their entire implementations prerequisites for one another.
3. PD-D02 + PD-D03 establish model/environment profiles before PD-D04 calibration and live scoring. PD-D14 establishes enforceable mode/resource policy before live tool cases. Functional fixture preparation is distinct from live/native acceptance.
4. PD-D07/08/09/12 choose trust, optional capability and retention boundaries for applicable Windows, TUI, native and voice/memory cells. Unavailable/excluded cells stay explicit and unaccepted.
5. PD-D06 establishes distribution handoff with PD-D01/03 scope; PD-D13 and applicable phase evidence gate all independent or Ro-ASD distribution. PD-D11 actual publication decision remains separate at AC-P12-18.

Backlog dependencies retained: P0-03 is blocked by P0-01; P1-05 depends on P0-03; P0-04 depends on P0-01 and P0-03. This report does not edit those tasks. P0-05 release ownership and EP-P5 migration remain distinct work. A recorded disposition can close a decision item only; it cannot close F03 failure semantics, F04 Windows transport or any other implementation finding.

## Repository and distribution responsibility proposal

| Area | Proposed responsible repository/role | Evidence and approval still needed |
| --- | --- | --- |
| Domain runtime, providers, security, Desktop/terminal, protocol, tests and core CI | `sentinel-projects/sentinel`; scoped engineering role appointed by maintainer | Verified source repository identity only; no organization membership/admin rights inferred. |
| Existing build/package definitions | Remain in sentinel at present: [packaging](../../packaging), [CPack](../../cmake/SentinelCPack.cmake) | Moving code is a separate approved change; no migration implied by repository planning. |
| Distribution recipes, exact payload inventories, signing/deployment and corresponding-source integration | Proposed `sentinel-packaging`; release/distro role | Existence, access and accountable humans unverified. Do not route actual issues there until verified. Core defects stay in sentinel. |
| Ro-ASD image integration | Actual distro/image owner plus Sentinel release handoff | Confirm image/version/channel, updates, support/escalation and native validation owner. No new repository name or organizational policy is invented. |
| Governance, rights and private evidence | Actual repository administrator, rights holder and privacy/security owner as applicable | CODEOWNERS is not evidence of admin powers or redistribution rights. Public issues contain sanitized symptoms and restricted evidence pointers only. |

Independent release contract recommendation: Sentinel version/source/dependency/asset gates apply regardless of distro; Ro-ASD pins an approved candidate and carries image-specific deployment/upgrade evidence. No default model weights or automatic acquisition. A Ro-ASD delay need not block an otherwise independently approved Sentinel release; Sentinel's applicable gates cannot be bypassed by bundling. Separate support owners must accept core-versus-image triage and rollback responsibilities.

## Platform, security and process boundaries

Fedora KDE Plasma remains primary; Linux/macOS/Windows portability is preserved. Proposed first release claims should be limited to accepted native capability cells, with Apple Silicon and Windows x86_64 environment access confirmed under PD-D03. [Building guidance](../development/BUILDING.md) calls Windows ARM64 experimental. Windows Rust [transport](../../cli/crates/sentinel-ipc/src/lib.rs) explicitly returns unavailable; do not advertise terminal parity from CMake success. Linux [shortcut adapter](../../apps/sentinel-desktop/src/NativeCompanionAdapter.cpp) reports global shortcut unavailable; [tool descriptors](../../core/src/runtime/BuiltInToolProvider.cpp) keep Linux app launch unsupported. Optional TUI operations remain gated by [command availability](../../cli/crates/sentinel-tui/src/commands.rs). Scope deferral requires approval and keeps associated findings open.

Unix daemon startServer uses owner-access socket options, lock and peer UID checks (SO_PEERCRED/getpeereid). This is a same-user trust boundary, not protection against every hostile process under the same UID. Client UI approval and persistent grants are separate authorities; reconnect does not authorize replay or grant expansion. Security review must preserve descriptor/schema/resource checks, sandbox availability and truthful failure through existing interfaces.

LocalModelRuntime launches a fixed ProcessExecutor request only for eligible local endpoint/model conditions; it retains owned process IDs, avoids competing with an existing listener and terminates its owned selection when idle. That does not establish ownership of an arbitrary Ollama/LM Studio/external listener. PD-D10 must specify owned handles, drain/unload, cancellation and restart policy. Non-cooperative in-process work may remain Stopping; no finite hard-stop guarantee without reviewed isolation. Never terminate an external process based only on name/port or destroy active dependencies to pretend shutdown succeeded.

Native OS consent is additional to Sentinel approval: microphone/speech, shortcut registration, clipboard GUI availability, process sandbox and launch policy require platform-specific acceptance. Desktop captures audio while daemon/native backend owns applicable transcription; privacy review must show backend/fallback and start/stop/cancel ownership. No consent is inferred from an installation or product role.

## Maintainer and reviewer handoff

[CODEOWNERS](../../.github/CODEOWNERS) routes all paths to @Sopwit and says there is one maintainer. This is source evidence only. No new person is assigned or assumed able to approve organization settings. Recommended role map: maintainer approves scope/compatibility; runtime/IPC owner reviews authority/profile contracts; CLI owner inventories consumers; security/privacy owner reviews trust and retention; validation owner verifies exact candidate/native evidence; release/distro owner accepts package/support obligations; actual rights holder/admin supplies restricted approvals. One person may fill multiple roles if explicitly recorded; do not fabricate independent review.

For future issue creation, first verify repository access and search existing issues, then map local backlog keys to actual URLs. Use one primary issue with bounded cross-repository children, not duplicate phase epics. This report grants no permission to create issues. Ownership records need real accountable human, reviewer, affected criteria, evidence and escalation/recovery path; role labels alone do not satisfy AC-P0-04.

## Exact proposed future register updates

Do not apply these until actual maintainer approval exists. For each PD-D01…PD-D14 section in ACCEPTANCE_DECISIONS.md, retain its ID and alternatives, then replace Pending Decision only with a real recorded disposition. Add fields: `Chosen option`, `Rationale/trade-off`, `Candidate/profile/capability scope`, `Affected acceptance IDs`, `Accountable human`, `Reviewer and independence limits`, `Approval date/evidence`, `Implementation follow-up`, `Revalidation trigger`. If deferred, retain Pending Decision and add concrete missing evidence/next review condition. Never change AD-A01…AD-A10.

Proposed PD-D05 wording after approval of the recommendation: “Retain existing C++ CLI compatibility pending consumer inventory; target daemon-backed supported mutations and explicitly bounded isolated offline diagnostics. No new Agent/task authority. Retirement requires consumer/parity/profile migration/recovery evidence and a separate removal approval. P1-05 exit defects remain open until independently verified.” This wording is a proposal only, not approved behavior or completed enforcement.

Proposed backlog update after approval: append P0-03 choice/evidence and consumer/profile inventory; retain P1-05 independently; link EP-P5 migration contract and P0-04 consistency follow-up; route packaging children only after repository access is verified. Do not rewrite P1-01 or P1-04.4 scope. Proposed matrix update after review: attach the approval evidence to AC-P0-04, retaining Blocked until its required decisions and real humans are recorded; leave AC-P0-03 blocked pending concrete models/hardware/budgets. No criterion becomes Passed merely from this report.

## Actionable approval checklist

- [ ] Accept or reject the verified baseline for this review; separately freeze release candidate/platform/capability manifest (PD-D01).
- [ ] Record a choice, rationale, actual accountable human/reviewer, date and evidence for **each** PD-D01…PD-D14; explicitly defer unresolved entries rather than adopting blanks.
- [ ] Confirm all ten AD-A decisions unchanged and existing acceptance/repeat/security thresholds preserved.
- [ ] Inventory real legacy consumers and approve supported commands, profile ownership, concurrency/migration/rollback and deprecation gates (PD-D05); keep exit-fix and migration work separate.
- [ ] Approve daemon/provider owned-versus-external process contract and truthful non-cooperative shutdown policy (PD-D10).
- [ ] Confirm model/hardware access and mode/resource policies, then calibrate budgets before scoring (PD-D02/03/04/14).
- [ ] Approve supported platform/capability and native permission boundaries, optional unavailable states, Windows trust and retention (PD-D07/08/09/12).
- [ ] Verify packaging/distro repository access and real support/image owners; approve independent-release/Ro-ASD handoff without implicit model acquisition (PD-D06).
- [ ] Assign real review/rights/admin responsibilities and verify B01–B14 evidence under existing gates (PD-D11/13); separately authorize any later publication.
- [ ] Authorize a later scoped register/backlog/matrix update with the fields above; preserve historical audits and parallel work.

Unresolved now: all fourteen pending choices, actual legacy consumers, real role acceptance/reviewer capacity, packaging access, distro image/support policy, live/native environments/models, numeric calibrated budgets and rights/admin dispositions. Engineering preparation is complete; approval-dependent P0-03 closure and P0 remain open.

## Validation receipt

Source path/symbol spot checks cover dispatcher commands, daemon initialization/profile paths, Desktop detached startup, controller/runtime routing, provider process helper, Unix trust checks, Windows unavailability, Linux shortcut and tool capability declarations. Fourteen pending IDs and ten adopted IDs are covered; direct criterion references are extracted from the unchanged matrix. Local Markdown links, table shape, fences and whitespace are checked. Tracked file content hashes across this worktree, the original checkout and P1-04.4 worktree are compared against a pre-authoring snapshot; only this new report is intended to differ. No runtime acceptance is inferred. Final check results are recorded below.

Final results: **PASS** local link targets; 10 adopted/14 pending ID coverage; all direct decision-to-criterion edges match the baseline matrix; criterion IDs resolve; Markdown table widths/fences/whitespace; `git diff --check`. Preservation comparison passed for 5226 files in original checkout, 5228 files in review worktree, 5229 files in P1-04.4 worktree. These are byte-preservation checks, not a secrets or runtime audit. Final status contains only the new report. No shared-register or production diff exists.
