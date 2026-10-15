# Sentinel — Acceptance Decisions and Adoption Register

Date: 2026-10-16. Contract baseline: `b152a960c11fe6e13b086666752ea5954005797b`, with the pre-existing five-file Desktop voice delta recorded in the [matrix](ACCEPTANCE_MATRIX.md#preflight-and-user-owned-working-tree). [Roadmap](SENTINEL_PLATFORM_ROADMAP.md), [backlog](GITHUB_ISSUE_BACKLOG.md) and preserved [P0.2](../audits/P0_2_ROADMAP_RECONCILIATION.md) remain context. **Adopted product direction is different from accepted implementation or human approval of suggested defaults.** No GitHub assignees/objects or live/native validation are fabricated.

## Adopted product decisions

These ten decisions are **Adopted** because the user explicitly confirmed them in P0.2 and reaffirmed adoption in P0-02. Adoption source is the supplied user requests, not inferred from code or this agent's recommendations. All related verification criteria remain open until executed evidence passes.

| Decision ID | Status | Decision | Adopted meaning / constraint |
| --- | --- | --- | --- |
| AD-A01 | Adopted | Independent project | Sentinel remains an independent cross-platform open-source project under sentinel-projects. |
| AD-A02 | Adopted | Optional Ro-ASD models | Ro-ASD may bundle Sentinel; downloading local model weights requires user choice. |
| AD-A03 | Adopted | Preserved architecture | Preserve C++20/Qt 6/QML and the existing shared runtime architecture. |
| AD-A04 | Adopted | Supported surfaces | Maintain Desktop, Quick Panel, System Tray, Rust CLI/TUI and supported platform interfaces. |
| AD-A05 | Adopted | Backend scope | Support Ollama, llama.cpp, LM Studio and extensible compatible providers. |
| AD-A06 | Adopted | Execution priority | Prioritize Passive and Assisted; Proactive automation is later work. |
| AD-A07 | Adopted | Adaptive resources | Resource policies adapt to hardware; no universal rigid RAM/CPU target. |
| AD-A08 | Adopted | Long-term product roles | Keep System Assistant, Developer Agent and Personal Assistant in scope; they do not imply separate runtimes. |
| AD-A09 | Adopted | Reuse authority | Extend existing components; do not add duplicate AgentRuntime, IModelRouter or tool execution authority. |
| AD-A10 | Adopted | Security and completion | Strict permissions/isolation and truthful accepted completion remain mandatory. |

## Established validation protocol

The current request explicitly requires zero false-success and unauthorized side effects; three repeats per selected live-model case, without statistical reliability claims; 100-repeat Unix handshake target; separate backend/model/availability/ownership/reliability dimensions; per-hardware calibration; fixed security bounds; historical evidence only as baseline; no source edits/model tests/downloads/GitHub writes in this task. The matrix operationalizes those requirements. This is not adoption of a model, a numerical performance threshold or a release candidate.

P0-02 document preparation can finish while P0 and actual matrix adoption/implementation acceptance remain incomplete. No blank decision or suggested default is consent. Decisions become adopted only when the maintainer records actual choice, rationale, affected IDs, candidate/config applicability, accountable human/reviewer and evidence/date. Later changes invalidate affected prior execution cells explicitly; never silently rebaseline. Prior audits are immutable historical evidence.

## Pending implementation and maintainer decisions

Every entry has status **Pending Decision**. Suggested defaults are technically justified recommendations only, not applied settings, permissions, staffing or approved scope. Role owner is a responsibility awaiting assignment, not a GitHub assignee. Review timing: resolve before the dependent execution/phase closure; no invented calendar commitment.

### PD-D01 — Candidate and release scope

**Status:** Pending Decision. **Accountable role:** Maintainer; actual human not assigned here.

**Recommendation and rationale:** Record candidate SHA plus accepted dirty diff, platform/architecture tiers and shipped capabilities before validation; leave Windows terminal and optional capabilities unaccepted until evidence exists.

**Alternatives:** Use clean committed candidate when authorized; alternatively use a reviewed immutable dirty-diff digest for isolated investigation.

**Suggested default:** Default recommendation: preserve current work, use a source-digest manifest for P1-04 only; release candidate remains pending.

**Dependency impact:** All phase closure; live/native/package rows; no after-the-fact rebaseline. Exact affected acceptance IDs are those referencing PD-D01 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D02 — Models and exact runtimes

**Status:** Pending Decision. **Accountable role:** Providers/models + maintainer; actual human not assigned here.

**Recommendation and rationale:** Select already user-owned models: one tool-capable coding model and one inspection case per local backend, recording repository revision/digest, quantization, runtime version and parameters. No model identity is adopted by this document.

**Alternatives:** One initial local backend then expand; multiple model sizes per hardware tier; authorized cloud subset only.

**Suggested default:** Default recommendation: start with one already available local tool-capable model, no download; remaining backends stay blocked.

**Dependency impact:** P1 live corpus, P2/P3/P10 capability claims and all inference budgets. Exact affected acceptance IDs are those referencing PD-D02 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D03 — Hardware and environment availability

**Status:** Pending Decision. **Accountable role:** Validation lead + maintainer; actual human not assigned here.

**Recommendation and rationale:** Confirm owners/access and exact Fedora KDE Wayland x86_64, Apple Silicon and Windows x86_64 versions; record RAM, CPU, GPU/VRAM or unified memory and power mode.

**Alternatives:** Physical hardware, approved VM for non-GPU IPC, external/native tester; do not equate VM with GPU/native acceptance.

**Suggested default:** Default recommendation: declare unknown environments unavailable until confirmed; local Darwin arm64 is only a fixture-host observation.

**Dependency impact:** Native and per-profile performance acceptance; release platform scope. Exact affected acceptance IDs are those referencing PD-D03 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D04 — Calibrated performance budgets

**Status:** Pending Decision. **Accountable role:** Validation/resources + maintainer; actual human not assigned here.

**Recommendation and rationale:** Calibrate each metric/profile with the methodology in the matrix; adopt numeric thresholds before candidate scoring, including responsiveness/cancel/shutdown deadlines and variance policy.

**Alternatives:** Baseline-relative regression budgets versus per-tier absolute targets; both require predeclared sample/window rules.

**Suggested default:** Default recommendation: Pending Calibration; sub-100 MiB idle daemon is an optional candidate target for a defined Linux headless profile only.

**Dependency impact:** P4, timing portions of P1/P5/P6/P10, P12; no unmeasured performance pass. Exact affected acceptance IDs are those referencing PD-D04 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D05 — Legacy C++ CLI disposition

**Status:** Pending Decision. **Accountable role:** Maintainer + CLI owner; actual human not assigned here.

**Recommendation and rationale:** Inventory actual consumers; retain bounded diagnostic compatibility temporarily, correct failure exits, then choose daemon migration/deprecation without a second task authority.

**Alternatives:** Immediate deprecation after consumer review; daemon-backed compatibility shim; bounded retained diagnostics with separate profiles.

**Suggested default:** Default recommendation: retain only documented diagnostics until compatibility evidence supports retirement; failure exits must still be fixed.

**Dependency impact:** P1-05 scope, P5 authority and P0 closure. Exact affected acceptance IDs are those referencing PD-D05 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D06 — Ro-ASD packaging and support

**Status:** Pending Decision. **Accountable role:** Maintainer + actual distro/packaging owner; actual human not assigned here.

**Recommendation and rationale:** Confirm image/version/support responsibility and sentinel-packaging existence/access; keep distribution recipes linked to exact sentinel candidate and no-model-download default.

**Alternatives:** Independent optional package; default bundled app without weights; defer distro integration.

**Suggested default:** Default recommendation: package app only, no weights/auto-download, no publication while rights/security gates remain open.

**Dependency impact:** P7, packaging repository routing, applicable P12 distro claims. Exact affected acceptance IDs are those referencing PD-D06 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D07 — Windows trust and isolation

**Status:** Pending Decision. **Accountable role:** IPC/security owner + maintainer; actual human not assigned here.

**Recommendation and rationale:** Adopt named-pipe user/session access rules, peer identity checks and explicit unsupported sandbox operations; test cross-user rejection natively.

**Alternatives:** Restricted same-user pipe policy with documented session scope; narrower single-session policy; defer Windows terminal release.

**Suggested default:** Default recommendation: fail closed for unsupported confinement; Windows terminal remains unavailable until implemented/tested.

**Dependency impact:** P5 Windows IPC, SP sandbox, Windows P6/P12 acceptance. Exact affected acceptance IDs are those referencing PD-D07 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D08 — TUI and optional integration release scope

**Status:** Pending Decision. **Accountable role:** Terminal/extensions + maintainer; actual human not assigned here.

**Recommendation and rationale:** Keep Plan/Compact/Undo/Redo/Skills/Init/Grill/ExternalEditor unsupported until their daemon contracts pass; do not treat WebUI/ACP stubs as services.

**Alternatives:** Prioritize individual commands after backend support; exclude them from the first release and keep findings open.

**Suggested default:** Default recommendation: explicit unavailable states and no fake execution; optional OCR/WebUI/ACP/Copilot remain unaccepted.

**Dependency impact:** P6/P8/P11 and shipped-capability P12 matrix. Exact affected acceptance IDs are those referencing PD-D08 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D09 — KDE shortcuts, clipboard and Linux launch policy

**Status:** Pending Decision. **Accountable role:** Desktop/security + maintainer; actual human not assigned here.

**Recommendation and rationale:** Select supported KDE/portal activation; keep clipboard unavailable headless unless an explicit permission-scoped GUI broker is reviewed; keep Linux app-launch denied until enforceable.

**Alternatives:** Native KDE adapter or supported portal; explicit broker or continued unavailable; supported supervised launcher or defer.

**Suggested default:** Default recommendation: implement shortcut behind existing platform adapter; retain truthful clipboard/app-launch unavailability until trust policy accepted.

**Dependency impact:** P6 native action/availability and P7 KDE flow. Exact affected acceptance IDs are those referencing PD-D09 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D10 — Provider ownership and hard-stop policy

**Status:** Pending Decision. **Accountable role:** Models/IPC/security + maintainer; actual human not assigned here.

**Recommendation and rationale:** Define owned versus external runtime identity, safe drain/unload and explicit supervised process strategy for any hard-stop guarantee.

**Alternatives:** Cooperative in-process lifetime with truthful Stopping; isolated provider process for bounded hard stop.

**Suggested default:** Default recommendation: do not kill external runtimes or destroy active dependencies; no hard-stop claim without isolation evidence.

**Dependency impact:** P3 ownership, P5 shutdown, P4 adaptive policy and cancellation budgets. Exact affected acceptance IDs are those referencing PD-D10 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D11 — Named humans and review capacity

**Status:** Pending Decision. **Accountable role:** Maintainer; actual human not assigned here.

**Recommendation and rationale:** Assign actual humans to accountable roles, validation reviewer and rights/admin decisions; record one-maintainer review/recovery policy.

**Alternatives:** One maintainer plus external native evidence contributors; scoped delegated reviewers where real people exist.

**Suggested default:** Default recommendation: role-based planning stays unassigned; no invented assignees or fictional second approver.

**Dependency impact:** P0 closure and human signoffs; does not prohibit a bounded local fixture investigation. Exact affected acceptance IDs are those referencing PD-D11 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D12 — Memory/audio/evidence retention

**Status:** Pending Decision. **Accountable role:** Privacy owner + maintainer; actual human not assigned here.

**Recommendation and rationale:** Adopt consent, deletion/export/backup retention and private/raw versus public/sanitized evidence handling, including voice backend fallback visibility.

**Alternatives:** No retention of raw audio by default; opt-in bounded retention; policy-specific memory persistence.

**Suggested default:** Default recommendation: synthetic data, minimum retention, explicit consent and reviewed deletion semantics; no private runtime inspection here.

**Dependency impact:** P8/P10, extension data scope, public P12 evidence. Exact affected acceptance IDs are those referencing PD-D12 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D13 — Dependency/rights/privacy/admin release dispositions

**Status:** Pending Decision. **Accountable role:** Release/security/rights owners + administrator; actual human not assigned here.

**Recommendation and rationale:** Revalidate B01–B14 against the exact candidate and actual payload/remote settings; record valid remediation and corresponding-source/signing evidence under existing gates.

**Alternatives:** Compatible dependency repair versus reviewed upstream migration; proven rights/source route or withhold affected distribution.

**Suggested default:** Default recommendation: all B01–B14 remain open; no waiver from build/signature/fixture success.

**Dependency impact:** All public distribution, including Ro-ASD; AC-P12 blocker rows. Exact affected acceptance IDs are those referencing PD-D13 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

### PD-D14 — Mode mapping and test resource policy

**Status:** Pending Decision. **Accountable role:** Maintainer + runtime/security owner; actual human not assigned here.

**Recommendation and rationale:** Map Passive to requested inspection/conversation without implicit mutation and Assisted to user-initiated scoped tools/approvals; map to actual mode/policy interfaces before execution.

**Alternatives:** Existing Chat/Agent plus bounded permission profiles; explicit read-only inspection profile after backend enforcement.

**Suggested default:** Default recommendation: map semantics, do not invent existing enums or enable Proactive behavior; unknown/denied capability produces non-success.

**Dependency impact:** P1 corpus permissions, P9 triggers and cross-surface behavior. Exact affected acceptance IDs are those referencing PD-D14 in the matrix.

**Adoption evidence required:** Maintainer choice/rationale, concrete version/profile/policy where applicable, affected acceptance IDs, actual responsible human and dated approval record. None recorded in this task.

## P1 readiness decision boundary

Recommend **P1-04 isolated Unix IPC handshake investigation next**. It needs existing Rust fixture/toolchain and private local sockets, not live models, reference GPU hardware, credentials or a finalized release budget. This recommendation does not change the backlog's P0-01 adoption dependency or mark P0 complete. Record the current candidate/fixture digest and initial failing trace; keep closure/stress on unavailable CI OS profiles blocked until measured.

P1-01 corpus implementation can then extend existing runtime seams; P1-02 live acceptance remains blocked by model/hardware/mode/calibration decisions, and P1-03 timing/native closure by calibrated stage budgets and ownership policy. No implicit approval to install models, modify personal runtime settings or run arbitrary generated code follows from this register.
