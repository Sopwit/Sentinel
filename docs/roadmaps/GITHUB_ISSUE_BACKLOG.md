# Sentinel — Proposed GitHub Implementation Backlog

Date: 2026-10-16. Source baseline: `b152a960c11fe6e13b086666752ea5954005797b`. HEAD matches this SHA, but the reconciliation records five subsequently modified Desktop dictation files; rebaseline their eventual accepted diff before P6/P10 verification. **Preparation only:** no issues, milestones, PRs, labels or Projects created. Keys below are document-local identifiers, not GitHub issue numbers. This is a bounded initial backlog of **26 candidate entries: 13 phase epics, 10 actionable P0/P1 sub-issues and 3 cross-cutting parents**. Later phases deliberately remain parent-level until prerequisites produce concrete scope. [Roadmap](SENTINEL_PLATFORM_ROADMAP.md) supplies acceptance; [reconciliation](../audits/P0_2_ROADMAP_RECONCILIATION.md) supplies retained finding/owner ledger.

## Acceptance ID traceability — P0-02 contract

The [unified acceptance matrix](ACCEPTANCE_MATRIX.md) is the entry-level verification contract; the [decision register](ACCEPTANCE_DECISIONS.md) records ten adopted product decisions and pending implementation choices. This is a minimal traceability addendum, not closure of existing issue checklists or creation of remote objects. Existing repository, priority, blocked-by, DoD and evidence metadata above/below remains authoritative. HEAD still matches baseline; the matrix records the five-file user-owned voice diff at entry, temporary disappearance and later reappearance during this task, with no automatic rebaseline or acceptance.

| Actionable candidate | Acceptance IDs / Definition of Done and evidence linkage |
| --- | --- |
| P0-01 | AC-P0-01, AC-P0-04: reviewed candidate/authority/actual ownership and pending decision record; no phase completion by class count |
| P0-02 | AC-P0-02, AC-P0-03: 65-entry matrix, COR-01–18, ENV/BE/BUD profiles, unique IDs/noncyclic dependencies, adopted choices and all unresolved cells; exact models/hardware/budgets remain blocking |
| P0-03 | AC-P0-04, AC-P5-04 and PD-D05: CLI consumer/disposition decision, separate authority and scoped composition map |
| P0-04 | AC-P0-02: current-guidance source/link review with unchanged historical audits; no docs-as-live-validation claim |
| P0-05 | AC-P0-04, AC-P12-01–14 and PD-D06/PD-D13: B01–B14 owners/repository/distribution decisions retained |
| P1-01 | AC-P1-01, AC-P1-05, AC-P1-06: deterministic COR-01–18 fixture variants and independent verifier; no unsupported claim from tool receipts |
| P1-02 | AC-P1-02, AC-P1-06: three repetitions per adopted live case/model/backend/profile, all attempts and independent artifact/final oracle; no statistical claim |
| P1-03 | AC-P1-03, AC-P5-03/04: planning/generation/tool/approval cancellation, denial/late-event fencing, next-task recovery and calibrated timestamps |
| P1-04 | AC-P1-04, AC-P5-05: first-failure/close/deadline diagnosis, immediate-close/event-before-response/EOF regression, 100 repeats per Unix CI OS; zero unexplained failures |
| P1-05 | AC-P1-07 and PD-D05: deterministic legacy CLI success/failure/timeout exit contracts; no real pull/model downloads |

EP-P0…EP-P12 map to all AC rows bearing the matching phase; phase closure additionally requires the existing roadmap contract. WS-SP maps to AC-P1-05, AC-P2-04, AC-P3-02, AC-P4-03, AC-P5-01/02/04, AC-P6-08, AC-P8-02/03, AC-P10-01, AC-P11-01/02/03 and applicable AC-P12 rows. WS-QP owns COR/ENV/BUD validation, flake handling and native/profile evidence. WS-RE owns distribution-specific AC-P7/AC-P12 evidence in sentinel-packaging after ownership/access is confirmed, with core/runtime/dependency fixes in sentinel.

P0-02 document preparation is delivered, **not** full human adoption or calibrated P0 completion. Suggested next engineering task remains P1-04 isolated investigation; existing P0-01 dependency is retained for human adoption/parent closure. Safe fixture preparation does not imply live/native/model availability or release approval. No acceptance criterion is Passed by this addendum.

## Organization Project proposal

One organization-level GitHub Project: **Sentinel Platform**, owner `sentinel-projects`. Use views for phase, blocked work, active implementation, cross-cutting work and release gates; do not create a Project per repository/phase. Proposed fields: Status (Backlog/Ready/In progress/Blocked/In review/Done), Phase (P0…P12/Cross-cutting), Workstream (Security and Privacy/Quality and Performance/Release Engineering), Priority (Critical/High/Normal), Repository, Accountable role, Assignee, Blocked by, Acceptance evidence and Release scope. Labels/fields are proposals, not assertions of current GitHub setup.

Proposed small label set: `type:epic`, `type:task`, `type:defect`, `type:validation`, `type:decision`, `type:documentation`; `area:runtime`, `area:providers`, `area:models`, `area:resources`, `area:ipc`, `area:desktop`, `area:terminal`, `area:memory`, `area:automation`, `area:multimodal`, `area:extensions`, `area:platform`, `area:packaging`; `workstream:security-privacy`, `workstream:quality-performance`, `workstream:release-engineering`. Phase/priority/status belong in fields rather than duplicative label taxonomies. Add only labels actually used. Record blocking dependencies through GitHub blocked-by relations if available, otherwise explicit linked checklist references.

Repository ownership: **sentinel** owns core/runtime/UI/CLI/TUI/contracts/tests/core CI and source/dependency fixes. **sentinel-packaging** owns distribution recipes, exact payload inventories, signing/deployment/source-delivery integration and distro-specific packaging. Existing packaging sources still live under sentinel/packaging; moving them is a separate decision, not assumed here. Existence/access/ownership of sentinel-packaging was not queried; verify before creating any issue there. A cross-repository epic is created once in the primary repo and links bounded child work in the other; no duplicate phase epics or duplicate runtime implementations.

Actual sentinel CODEOWNERS lists @Sopwit. Accountable roles below are proposed responsibilities; do not fabricate teams, legal reviewers or named assignees. Maintainer appoints real humans and reviewer capacity. One-maintainer review/admin decisions remain explicit. Creating issues later requires a separate instruction; this document is no permission to publish security/private evidence.

## Shared issue Definition of Done

An issue closes only when its listed acceptance outcomes and parent phase criteria applicable to its scope pass, implementation/docs/contracts agree, tests are appropriate and recorded, security/privacy impact is reviewed, failures/skips/limitations are retained, and exact-candidate evidence is linked. Fixture success alone cannot satisfy live/native criteria. “Decision recorded” can close a decision sub-issue, but never silently closes an implementation defect. A scope exclusion keeps the corresponding implementation finding open with owner/reason/review point. Parent epics remain open until their adopted acceptance scope closes.

For shared implementation changes use existing CMake configure/build/CTest and relevant Cargo fmt/clippy/test checks from repository docs; include generated IPC/Desktop checks when affected. Native/live testing occurs only with an adopted matrix and authorized resources. No downloads/personal profiles by implication. Documentation tasks use baseline/link/structure/whitespace/privacy checks, not invented runtime evidence. Preserve historical reports and existing release gates.

Evidence must identify source SHA/diff, platform/hardware/toolchain/provider/model/runtime, synthetic case, expected and actual outcomes, all repetitions, timings/budgets, grant/approval/cancel state, independent artifact verification, native checklist and remaining findings. Raw sensitive evidence stays private; public issues use sanitized summaries.

## Phase parent candidates

Parent `Blocked by` below denotes prerequisites for phase **closure**; child readiness may depend on a narrower adopted contract, as documented below. This distinction permits early contract/fixture work without claiming a phase complete. Acceptance sections in the roadmap are the exact parent DoD; changing them requires a decision record, not after-the-fact relaxation.

| Key / proposed title | Repository; accountable role | Labels; priority | Blocked by for closure | Finding/scope and DoD |
| --- | --- | --- | --- | --- |
| EP-P0 — P0: Baseline, architecture and acceptance reconciliation | sentinel; Maintainer/architecture | type:epic, area:runtime; Critical | None | Adopt baseline/authority/acceptance and assign unresolved decisions; children P0-01…P0-05. F03/F09/F13 planning, no defect closure. See roadmap P0 twelve-field contract. |
| EP-P1 — P1: Agent Reliability | sentinel; Runtime | type:epic, area:runtime; Critical | EP-P0 | Reliable Passive/Assisted Agent final/failure/cancel and IPC regression; children P1-01…P1-05; F01/F02/F03 exit semantics. See roadmap P1 twelve-field contract. |
| EP-P2 — P2: Multi-Backend Architecture | sentinel; Providers | type:epic, area:providers; High | EP-P0, EP-P1 | Capability/readiness/frozen binding across advertised providers; F09/F12. See roadmap P2 twelve-field contract. |
| EP-P3 — P3: Model Lifecycle Management | sentinel; Models | type:epic, area:models; High | EP-P1, EP-P2; P5 ownership contract | Consent, artifact/process ownership, load/unload identity/recovery; F08/F12. See roadmap P3 twelve-field contract. |
| EP-P4 — P4: Adaptive Resource Management | sentinel; Resources | type:epic, area:resources; High | EP-P1, EP-P2, EP-P3; WS-QP profile contract | Adaptive telemetry/admission/hysteresis and per-profile measured budgets; F08. See roadmap P4 twelve-field contract. |
| EP-P5 — P5: Daemon and IPC Hardening | sentinel; Daemon/IPC | type:epic, area:ipc; High | EP-P0, EP-P1; P2/P3 lifetime contracts | Windows pipes, profile/endpoint trust, bounded history/restart/drain, adopted CLI authority disposition; F03/F04/F09. See roadmap P5 twelve-field contract. |
| EP-P6 — P6: Fast Actions and Desktop UX | sentinel; Desktop/terminal | type:epic, area:desktop; High | EP-P1, EP-P2, EP-P5; P8/P11 contracts for any newly enabled dependent commands | KDE shortcut, headless clipboard decision/broker, Linux launch policy, readiness/native flow/TUI parity; F05/F06/F07/F11. See roadmap P6 twelve-field contract. |
| EP-P7 — P7: Ro-ASD Native Integration | sentinel-packaging; Packaging/distro | type:epic, area:platform; High | EP-P3, EP-P5, EP-P6; applicable WS-SP/WS-RE distribution gates | Ro-ASD clean-image bundle/no implicit model acquisition, independent identity; core companion changes linked in sentinel. See roadmap P7 twelve-field contract. |
| EP-P8 — P8: Memory and Context | sentinel; Memory/context | type:epic, area:memory; High | EP-P2; P3/P4 budget/embedding and P5 history contracts | Bounded context, retrieval, retention/consent and migrations; /compact only after authoritative contract. See roadmap P8 twelve-field contract. |
| EP-P9 — P9: Automation | sentinel; Runtime/automation | type:epic, area:automation; Normal | EP-P1, EP-P4, EP-P5, EP-P8; P6 controls and WS-SP authority contract | Later opt-in trigger automation through AgentRuntime; dedup/revoke/restart outcomes, no silent Proactive mode. See roadmap P9 twelve-field contract. |
| EP-P10 — P10: Voice and Multimodal | sentinel; Audio/multimodal | type:epic, area:multimodal; Normal | EP-P2, EP-P3; P4/P5/P6 contracts | Optional configured STT/TTS/vision/document native acceptance and explicit limitations. See roadmap P10 twelve-field contract. |
| EP-P11 — P11: Extensions and Ecosystem | sentinel; Extensions | type:epic, area:extensions; Normal | EP-P1, EP-P2, EP-P5; P8 privacy and WS-SP contracts | Experimental extension compatibility/isolation and explicit WebUI/ACP/Copilot helper disposition; F10/F11 skills dependency. See roadmap P11 twelve-field contract. |
| EP-P12 — P12: Release Acceptance | sentinel-packaging; Release maintainer | type:epic, area:packaging; Critical | All shipped phase gates and WS-SP/WS-QP/WS-RE signoffs | Exact-candidate native/package/source/signing/human-publication acceptance; F13 and B01–B14; sentinel core candidate linked. See roadmap P12 twelve-field contract. |

P3/P5 contract dependencies are not circular phase completion: adopt provider/process ownership contract before either closes; P5 does not wait for all P3 model acquisition implementation. P6/P8/P11 command contracts are similar: P6 may retain explicitly unsupported commands; any command promoted into its accepted scope waits for the relevant backend contract and tests. P12 uses an explicitly adopted shipped-capability matrix, not an unconditional requirement that every exploratory feature ship. Excluded later phases remain open.

## Detailed P0 sub-issue candidates

### P0-01 — Adopt baseline, authority map and phase contract

**Repository:** sentinel. **Parent:** EP-P0. **Accountable role:** Maintainer/architecture. **Labels:** type:decision, area:runtime. **Priority:** Critical. **Blocked by:** None.

**Problem/evidence:** F03/F09: preserve one AgentRuntime/IModelRouter/ToolExecutionGateway and distinguish live authority from metadata/stubs. New roadmap IDs currently have no adoption record. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Record exact branch/SHA/source-diff state; review P0.1/P0.2 and product decisions; map daemon/Desktop/Rust/legacy/store owners; assign real accountable humans and issue routing.

**Acceptance / Definition of Done:**

- [ ] The recorded baseline and source diff match the proposed implementation starting point; any drift has a scoped re-audit.
- [ ] Maintainer adopts the P0–P12 contract or lists explicit changes; every F01–F13 retains an owner and closure condition.
- [ ] Authority map distinguishes metadata/helper classes, daemon runtime, client presentation and separate persistence; no new runtime/router/gateway proposed.

**Automated and native/manual verification:** Document structure/link/dependency checks and before/after historical hashes; maintainer review required; no native/model claim.

**Completion evidence and exclusions:** Adoption/ownership decision record plus baseline receipt. Out of scope: source changes, automatic GitHub creation or closing any implementation finding.

### P0-02 — Adopt isolated Agent acceptance corpus and hardware/provider budgets

**Repository:** sentinel. **Parent:** EP-P0. **Accountable role:** Validation lead appointed by maintainer. **Labels:** type:validation, workstream:quality-performance. **Priority:** Critical. **Blocked by:** P0-01.

**Problem/evidence:** F01: P0.1 fixture passes do not establish real-model completion. P1 needs independently testable success/failure/denial/cancel cases and predeclared budgets. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Define synthetic disposable corpus; local model/backend/OS/hardware matrix; repeats and independent artifact checks; Passive/Assisted mapping; all stage cancellation and missing-capability cases. Reuse tools/agent/README.md without running models in this planning issue.

**Acceptance / Definition of Done:**

- [ ] Case list records inputs, authorized resources/tools, expected artifacts and terminal outcome, verifier and cleanup/retention.
- [ ] Choose available user-selected model identities and reference profiles; unavailable cells stay blocked. Adopt or revise proposed three attempts before runs.
- [ ] Record per-profile provider/task/cancel/memory deadlines and stop/failure rules before execution; no TBD cell counted accepted.
- [ ] Success requires both independent verification and accepted final; refusal/failure/denial/cancel never counts as successful task completion.

**Automated and native/manual verification:** Validate case schema/fixtures and forbidden-side-effect checks; native/model execution is a later P1 activity after resources are authorized.

**Completion evidence and exclusions:** Versioned adopted matrix, budgets and privacy/permission review. Out of scope: model downloads, personal data, post-result threshold weakening.

### P0-03 — Decide legacy C++ CLI compatibility and composition ownership

**Repository:** sentinel. **Parent:** EP-P0. **Accountable role:** Maintainer + CLI/runtime owner. **Labels:** type:decision, area:terminal. **Priority:** High. **Blocked by:** P0-01.

**Problem/evidence:** F03/F09: C++ CLI constructs an in-process controller, while production Desktop/Rust use daemon. Retained controller and metadata overlaps need ownership clarity. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Inventory documented/actual consumers and required offline diagnostics; compare daemon command parity; choose migration, deprecation or bounded diagnostic retention with profile/concurrency boundary. Assign focused future cleanup only when behavior-preserving.

**Acceptance / Definition of Done:**

- [ ] Decision explicitly identifies supported C++ CLI commands, consumers and profile ownership; unknown consumers remain a recorded risk.
- [ ] Any retained path cannot become a second task/agent authority; any retirement has compatibility/lifecycle/approval/recovery gates.
- [ ] P1 failure-exit fix stays tracked independent of final retirement; no silent removal of commands.
- [ ] Large controller/IPC classes receive an ownership and hotspot map, with scoped follow-ups rather than mandatory broad rewrite.

**Automated and native/manual verification:** Read-only command/source/contract comparison and documentation review; no runtime configuration or source refactor.

**Completion evidence and exclusions:** Consumer/parity table, chosen disposition and EP-P5 implementation gate. Out of scope: actual migration/deletion or inventing new in-process Agent commands.

### P0-04 — Prepare narrow current-state documentation corrections

**Repository:** sentinel. **Parent:** EP-P0. **Accountable role:** Documentation/architecture owner. **Labels:** type:documentation, area:ipc. **Priority:** High. **Blocked by:** P0-01, P0-03.

**Problem/evidence:** DAEMON.md migration/no-spawn, ROADMAP.md small CLI/exploratory daemon and PERMISSIONS.md blanket CLI approval wording are stale; architecture diagram omits Desktop IPC. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Update active guidance only after baseline adoption; link new roadmap and distinguish detached daemon startup from in-process fallback, Rust approval from permanent grant management, and source implementation from acceptance. Inventory canonical organization/source links.

**Acceptance / Definition of Done:**

- [ ] Current guidance matches bootstrap, daemon/Rust interfaces and one-use approval code, with dated links to preserved historical records.
- [ ] ARCHITECTURE.md no-in-process-fallback prose is not falsely treated as forbidding detached launch; diagram shows actual IPC route.
- [ ] Historical audits/migration/TUI/SEC-P1 records remain byte-identical; no test outcome/status is rewritten.
- [ ] New project links use confirmed canonical organization/repository identity; unresolved links are recorded rather than guessed.

**Automated and native/manual verification:** Local Markdown links/anchors and source-symbol spot checks; diff/privacy/whitespace and historical hash preservation.

**Completion evidence and exclusions:** Focused documentation diff and check receipt. Out of scope: broad architecture rewrite, feature claims or historical report edits.

### P0-05 — Assign SEC-P1 release blockers and cross-repository distribution ownership

**Repository:** sentinel. **Parent:** EP-P0. **Accountable role:** Release maintainer + security/rights/admin decision owners. **Labels:** type:decision, workstream:release-engineering. **Priority:** Critical. **Blocked by:** P0-01.

**Problem/evidence:** F13/B01–B14 remain unresolved; dependency/legal/privacy/admin/native gates cannot close from CI alone. sentinel-packaging ownership/access is unverified. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Map each register entry to accountable decision/remediation role and release gate; verify repository ownership/access before future issue creation; identify restricted evidence and distribution/core split. No scanner/admin changes implied.

**Acceptance / Definition of Done:**

- [ ] All B01–B14 have an owner role, next evidence action and applicable release blocking condition; unresolved rights/admin decisions remain open.
- [ ] Ro-ASD distribution uses the same source/dependency/asset/privacy gates; optional model downloads do not waive application redistribution rights.
- [ ] sentinel-packaging existence/access and responsibility are verified before routing real issues; absent access leaves entries blocked, not silently rerouted as completed.
- [ ] Public-safe and restricted evidence handling is specified; destructive history cleanup/publication remain separately authorized.

**Automated and native/manual verification:** Ledger/links/ownership review and baseline report preservation; actual advisory/remote protection revalidation is explicitly scheduled, not claimed.

**Completion evidence and exclusions:** Blocker ownership/dependency table and repository routing decision. Out of scope: dependency edits, rights waivers, admin settings, packaging publication or history rewrite.

### P1-01 — Build deterministic Agent reliability and outcome verification harness

**Repository:** sentinel. **Parent:** EP-P1. **Accountable role:** Runtime + validation owner. **Labels:** type:validation, area:runtime. **Priority:** Critical. **Blocked by:** P0-01, P0-02.

**Problem/evidence:** F01: success fixtures alone miss provider/tool failures and grounding/recovery/cancel boundaries. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Extend existing loop/runtime/controlled-task tests with adopted corpus seams and independent artifact/unauthorized mutation verification. Cover sync/async final parity, malformed tools, repeated reads/mutations, classification indeterminate and terminal ordering.

**Acceptance / Definition of Done:**

- [ ] Each adopted deterministic case specifies expected run outcome and independently checks artifacts/forbidden changes.
- [ ] Tool acknowledgement, rejected final, denial, provider/tool failure and cancellation cannot emit accepted success.
- [ ] Repeated ineffective actions and malformed calls reach bounded recovery/failure without repeated mutations.
- [ ] Sync/async paths preserve equivalent approval, final-gate, cancellation and event-order contracts.

**Automated and native/manual verification:** Run relevant existing QtTest targets plus full documented CMake build/CTest for shared changes; generated IPC tests if contract affected. No live success inferred.

**Completion evidence and exclusions:** Case-to-test mapping, regression diff and full failure/pass receipt. Out of scope: replacement runtime/planner or loosening evidence gates.

### P1-02 — Run isolated real-model acceptance and fix evidenced completion defects

**Repository:** sentinel. **Parent:** EP-P1. **Accountable role:** Runtime/providers + validation owner. **Labels:** type:task, area:runtime. **Priority:** Critical. **Blocked by:** P0-02, P1-01.

**Problem/evidence:** F01: no trustworthy live-model acceptance in P0.1; local/provider task outcomes need reproducible evidence. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Use existing helper/adopted corpus and already user-selected resources in disposable profiles. Record every attempt, inspect failures and implement only diagnosed bounded fixes; retain provider/model-specific unsupported cells.

**Acceptance / Definition of Done:**

- [ ] Every required success case/repetition has independently correct artifacts and accepted grounded final; all expected negative cases have their specified terminal outcomes.
- [ ] All required backend/profile cells pass adopted budgets; unavailable resources or failing attempts leave the item blocked/failed rather than averaged away.
- [ ] Evidence records exact source/model/runtime/policy and no unrelated fixture mutations or personal paths.
- [ ] Each source fix has a deterministic regression and fresh affected live-case results; original failures remain visible.

**Automated and native/manual verification:** P1-01 regressions/full shared checks; authorized live/native Desktop/TUI Assisted approval and task runs in isolated profiles. No implicit model downloads.

**Completion evidence and exclusions:** All-attempt result matrix, independent verification, redacted traces and residual findings. Out of scope: model-specific success hacks, hidden fallback, Proactive execution or release clearance.

### P1-03 — Verify cancellation, denial and late-result terminal fencing end to end

**Repository:** sentinel. **Parent:** EP-P1. **Accountable role:** Runtime/IPC + security owner. **Labels:** type:task, area:runtime. **Priority:** Critical. **Blocked by:** P0-02, P1-01.

**Problem/evidence:** F01: cooperative providers/tools and approval races require truthful once-only terminal state and next-task recovery. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Exercise cancel during classification, provider streaming, tool/process, batch and approval; denial/revoke; disconnect/reattach and daemon drain using existing contracts. Distinguish cooperative cancel budget from hard process termination.

**Acceptance / Definition of Done:**

- [ ] Each case produces one authoritative terminal state; late output/approval/result cannot resurrect or replace it.
- [ ] Denied/replayed approvals never grant authority; cancel invalidates pending requests and prevents queued tool dispatch.
- [ ] Cooperative cases meet adopted per-profile cancellation budgets and permit a subsequent clean task.
- [ ] Non-cooperative cases retain ownership/Stopping truthfully; no destructor/thread-kill success is fabricated and any hard-stop design goes through EP-P5/SP.

**Automated and native/manual verification:** Delayed/non-cooperative deterministic fixtures, event identity/replay tests, process cancellation and shutdown tests; native/live selected cases from adopted matrix.

**Completion evidence and exclusions:** Event/state timelines, process/resource cleanup record, budget results and next-task recovery. Out of scope: unsafe thread destruction or silently broadening cancellation into permissions.

### P1-04 — Investigate intermittent Rust IPC handshake close timing and add regression

**Repository:** sentinel. **Parent:** EP-P1. **Accountable role:** Terminal/IPC owner. **Labels:** type:defect, area:ipc. **Priority:** High. **Blocked by:** P0-01.

**Problem/evidence:** F02: handshake_correlation_and_event_queue failed once with Disconnected then passed in the full rerun. Fixture close/socket timeout restoration is only a hypothesis. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Reproduce in temporary Unix sockets; inspect handshake/request/queued-event/deadline behavior; distinguish fixture race from production disconnect; fix diagnosed layer only and retain absolute deadlines/no mutation replay.

**Acceptance / Definition of Done:**

- [ ] Failure trace/root-cause evidence identifies what closed the connection and whether production semantics are affected; no root cause inferred solely from rerun success.
- [ ] Regression covers event-before-response, immediate peer close, timeout restoration and subsequent EOF with expected behavior.
- [ ] 100 repetitions per supported Unix CI OS have zero unexplained failures; any residual failure stays open with evidence.
- [ ] Request deadline/transport invalidation, late-reply fencing and no-replay contracts stay intact.

**Automated and native/manual verification:** Scoped sentinel-ipc regression/stress, CLI command fixtures, cargo fmt/clippy/workspace tests with temporary sockets enabled; no daemon/model dependency.

**Completion evidence and exclusions:** First failure and diagnosis, bounded stress summary and full regression result. Out of scope: hiding test failure, retry-to-green policy, increasing timeouts without explanation or Windows implementation.

### P1-05 — Correct legacy C++ CLI failure and timeout exit contracts

**Repository:** sentinel. **Parent:** EP-P1. **Accountable role:** CLI/runtime owner. **Labels:** type:defect, area:terminal. **Priority:** High. **Blocked by:** P0-03.

**Problem/evidence:** F03: ChatCommand returns 0 after timeout; ModelCommand pull prints failure but returns 0. Scripts can misclassify failed work. See the reconciliation finding ledger and P0.1 source anchors.

**Technical scope:** Define/document nonzero timeout/provider/pull failure exits for retained commands; add injected/subprocess fixtures. Preserve chosen legacy scope and avoid invoking real provider downloads in tests.

**Acceptance / Definition of Done:**

- [ ] Timeout and explicit provider/pull failure return the documented nonzero code; success remains zero.
- [ ] Synchronous and asynchronous failure paths settle once, flush useful stderr and cannot wait indefinitely beyond the adopted command deadline.
- [ ] Tests deterministically exercise failure/timeout/success without personal settings/network/model downloads.
- [ ] CLI documentation names this retained compatibility surface and its separate authority disposition; no new Agent/task entry path is introduced.

**Automated and native/manual verification:** Existing/new bounded CLI subprocess fixtures with injected local seams; relevant CMake/CTest and build; command help/exit smoke with synthetic configuration.

**Completion evidence and exclusions:** Exit-contract table and deterministic receipts. Out of scope: daemon migration/deprecation implementation (EP-P5), extra commands or dependency installation.

## Cross-cutting parent candidates

### WS-SP — Security and Privacy

Repository: **sentinel**; link distribution children in sentinel-packaging when concrete. Accountable role: maintainer-appointed security/privacy owner; legal/rights/admin decisions assigned to actual responsible humans. Labels: type:epic, workstream:security-privacy. Priority: Critical. Initial blocking dependency: P0-01/P0-05 owner and evidence map. Ongoing closure dependency: applicable phase capability evidence; this parent is not a circular prerequisite for its own child work.

Scope: retain F06/F07/F13 and B01–B14; resource-specific authorization, secrets, same-user IPC trust, native/extension sandbox limits, consent/retention and public-safe evidence. DoD: scoped negative tests and privacy review for capability changes; no raw credential/personal payload leaks; exact dependency/right/privacy/admin findings verified; every residual gap remains open. Evidence: policy/negative-test matrix, private/public evidence inventory and blocker dispositions. No unapproved history rewrite, waiver, account change or public vulnerability disclosure.

### WS-QP — Quality and Performance

Repository: **sentinel**; sentinel-packaging links for native deployment measurements. Accountable role: validation lead appointed by maintainer. Labels: type:epic, workstream:quality-performance. Priority: High. Initial blocked by P0-02 adopted matrix (P1-04 scoped flake can start after P0-01). Final closure requires applicable native and exact-candidate evidence.

Scope: F01/F02, deterministic fixtures, Rust declared-minimum/toolchain CI, native keyboard/focus/responsive/loading/error, physical terminal fallback, hardware-sensitive budgets and regression sampling. DoD: zero unexplained test failures in adopted repeats; all mandatory per-profile budgets and native cases pass; skipped/unavailable cells remain unaccepted. Evidence: first failures plus all reruns, automated receipts and per-profile measurements. No universal resource cap or fixture-only platform claim.

### WS-RE — Release Engineering

Repository: **sentinel-packaging** (verify existence/access first); sentinel owns core build/CI, source/dependency and repository administration changes. Accountable role: release maintainer/packaging lead. Labels: type:epic, workstream:release-engineering. Priority: Critical. Initial blocked by P0-05 repository/gate decisions; final acceptance blocks EP-P12.

Scope: F13/B01–B14 applicable payload/source/rights/privacy/native/signing/governance evidence, package ownership/lifecycle, Ro-ASD optional-model default and cross-repo version locks. DoD: exact candidate manifests, tested corresponding-source delivery, checksum and native signature/installer evidence; all applicable blockers dispositioned under existing gates; separate human publication approval. Dependencies: core candidate from sentinel, WS-SP policy/blocker review, WS-QP native results. No actual release object creation or publication in this task.

## Initial actionable dependency graph

Only these ten P0/P1 children are ready for detailed issue preparation. Direction means “blocked by”; graph is intentionally acyclic:

```text
P0-01 <- P0-02, P0-03, P0-05, P1-04
P0-01 + P0-03 <- P0-04
P0-01 + P0-02 <- P1-01
P0-02 + P1-01 <- P1-02, P1-03
P0-03 <- P1-05
```

The dependency metadata above is authoritative if graph shorthand is unclear. P1 live execution waits for actual resources and signed-off budgets; no edge implies authorization to install/download. Phase parents additionally track close-out prerequisites listed in the table. Cross-stream reviews attach to affected changes; do not create a cycle by requiring all stream work before beginning a stream child.

## Suggested creation order and handoff

After a separate instruction to create GitHub objects, verify organization/repository permissions, search existing issues/Project entries to avoid duplicates, and map local keys to actual URLs. Create/reuse the single Project and parent epics, then the ten detailed children and three workstream parents; attach dependency links and evidence fields. Do not auto-create hundreds of later tasks or invent numbered issues/milestones. Reuse existing blockers where equivalent and retain their evidence.

Start with P0-01/P0-02 adoption, P0-03 compatibility decision and P0-05 release owner routing. P1-04 fixture investigation can proceed once authority baseline is adopted; P1-01 precedes real-model remediation. P0-04 follows the compatibility/current-state decisions. P1-05 stays bounded while EP-P5 owns the adopted CLI authority transition.

Open decisions: actual people/reviewer capacity, sentinel-packaging availability, release/platform scope, Ro-ASD image/support policy, model/hardware/terminal profiles and budgets, mode mapping, CLI disposition, GUI broker/Linux launch/Windows trust policy and rights/privacy/source/signing dispositions. These are recorded blockers, not permissions to silently choose weaker acceptance.
