# Sentinel — Audit & Evidence Index

This registry catalogs the 51 independent engineering audits, remediation reports, and architecture proposals for the Sentinel repository across Phases P0 through P6, platform stabilization (CI, WIN, SEC), and controlled integration milestones (INT).

All documents are deterministic evidence reports anchored to explicit Git commits, independent test runs, and verified contracts under [AGENTS.md](../../AGENTS.md).

---

## 1. P0 — Architecture, Roadmap & Consistency

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P0_1_CURRENT_STATE_AUDIT.md](P0_1_CURRENT_STATE_AUDIT.md) | Baseline codebase audit, component boundaries, and debt analysis | EP-P0 |
| [P0_2_ROADMAP_RECONCILIATION.md](P0_2_ROADMAP_RECONCILIATION.md) | Reconciliation of implementation against platform roadmap | EP-P0 |
| [P0_03_OWNERSHIP_COMPATIBILITY_REVIEW.md](P0_03_OWNERSHIP_COMPATIBILITY_REVIEW.md) | Service ownership, process boundaries, and cross-platform compatibility | AC-P0-03 |
| [P0_DOCUMENTATION_CONSISTENCY_RESULT.md](P0_DOCUMENTATION_CONSISTENCY_RESULT.md) | Verification of documentation consistency across architecture guides | AC-P0-02 |

---

## 2. P1 — Agent Runtime & IPC Reliability

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md](P1_01_AGENT_RELIABILITY_HARNESS_RESULT.md) | Comprehensive deterministic agent execution harness results | AC-P1-01 |
| [P1_01_7_PLANNER_STREAMING_INTEGRATION_RESULT.md](P1_01_7_PLANNER_STREAMING_INTEGRATION_RESULT.md) | Streaming task planner integration with bounded loop execution | AC-P1-01 |
| [P1_04_IPC_HANDSHAKE_INVESTIGATION.md](P1_04_IPC_HANDSHAKE_INVESTIGATION.md) | Root cause investigation for daemon IPC socket handshake failures | AC-P1-04 |
| [P1_04_2_LINUX_CI_VALIDATION.md](P1_04_2_LINUX_CI_VALIDATION.md) | Linux CI environment IPC validation under Bubblewrap sandbox | AC-P1-04 |
| [P1_04_3_GITHUB_CI_READINESS.md](P1_04_3_GITHUB_CI_READINESS.md) | Verification of runner constraints and socket cleanup in CI | AC-P1-04 |
| [P1_04_4_HOSTED_IPC_STRESS_INTEGRATION.md](P1_04_4_HOSTED_IPC_STRESS_INTEGRATION.md) | Stress testing of IPC daemon connections under concurrent load | AC-P1-04 |
| [P1_05_CURRENT_MAIN_INTEGRATION_VALIDATION.md](P1_05_CURRENT_MAIN_INTEGRATION_VALIDATION.md) | Integration validation of agent runtime and CLI exit semantics | AC-P1-05 |
| [P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md](P1_05_1_PORTABLE_CLI_TEST_ISOLATION_RESULT.md) | Portable CLI test profile isolation across macOS, Linux, and Windows | AC-P1-05 |
| [P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md](P1_05_LEGACY_CLI_EXIT_SEMANTICS_RESULT.md) | POSIX / sysexits.h exit code compatibility validation | AC-P1-05 |

---

## 3. P2 — Multi-Backend & Protocol Modularization

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md](P2_01_PROVIDER_CAPABILITY_CONTRACT_RESULT.md) | Provider capability matrix and capability negotiation tests | AC-P2-01 |
| [P2_02_PROVIDER_ERROR_REDACTION_RESULT.md](P2_02_PROVIDER_ERROR_REDACTION_RESULT.md) | Error sanitization, secret redaction, and bounded error summaries | AC-P2-02 |
| [P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md](P2_03_VISION_CAPABILITY_ENFORCEMENT_RESULT.md) | Enforcement of multimodal input contracts and rejection of unsupported payloads | AC-P2-03 |
| [P2_04_HTTP_CLASSIFICATION_RESULT.md](P2_04_HTTP_CLASSIFICATION_RESULT.md) | HTTP status code and network error categorization | AC-P2-04 |
| [P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md](P2_05_BOUNDED_STREAMING_CONTRACT_RESULT.md) | SSE and NDJSON frame buffer boundary contracts | AC-P2-05 |
| [P2_06_NATIVE_CLOUD_PROVIDER_CONTRACT_RESULT.md](P2_06_NATIVE_CLOUD_PROVIDER_CONTRACT_RESULT.md) | Anthropic Messages and Gemini native protocol contracts | AC-P2-06 |
| [P2_07_VENDOR_CLOUD_CONTRACT_RESULT.md](P2_07_VENDOR_CLOUD_CONTRACT_RESULT.md) | Cloud provider credential handling and endpoint contracts | AC-P2-07 |
| [P2_08_ENDPOINT_SECURITY_CAPABILITY_RESULT.md](P2_08_ENDPOINT_SECURITY_CAPABILITY_RESULT.md) | Host validation, SSRF defense, and loopback security enforcement | AC-P2-08 |
| [P2_09_PROVIDER_POLICY_LIVE_ACCEPTANCE_PLAN.md](P2_09_PROVIDER_POLICY_LIVE_ACCEPTANCE_PLAN.md) | Live provider test plan with synthetic fixture gates | AC-P2-09 |
| [P2_DEF_01_MODEL_DISCOVERY_REMEDIATION_RESULT.md](P2_DEF_01_MODEL_DISCOVERY_REMEDIATION_RESULT.md) | Fix for model discovery parser and rejection of malformed catalogs | AC-P2-01 |
| [P2_M01_PROVIDER_MODULARIZATION_ARCHITECTURE.md](P2_M01_PROVIDER_MODULARIZATION_ARCHITECTURE.md) | Architecture specification for transport-independent protocol modules | AC-P2-01 |
| [P2_M02_OPENAI_COMPATIBLE_PROTOCOL_EXTRACTION_RESULT.md](P2_M02_OPENAI_COMPATIBLE_PROTOCOL_EXTRACTION_RESULT.md) | Extraction results for pure OpenAI-compatible chat and catalog parsing | AC-P2-01 |

---

## 4. P3 — Model Lifecycle & Ownership

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md](P3_01_MODEL_LIFECYCLE_OWNERSHIP_RESULT.md) | Local model process lifecycle ownership baseline | AC-P3-01 |
| [P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md](P3_02_LIFECYCLE_OWNERSHIP_CONTRACT_RESULT.md) | Process executor contract and mock process ownership tests | AC-P3-02 |
| [P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md](P3_03_LIFECYCLE_READINESS_REMEDIATION_RESULT.md) | Model readiness verification and crash recovery | AC-P3-03 |
| [P3_04_MODEL_LIFECYCLE_RELIABILITY_RESULT.md](P3_04_MODEL_LIFECYCLE_RELIABILITY_RESULT.md) | Provider switching, endpoint switching, and in-use deletion protection | AC-P3-04 |
| [PD_D10_PROVIDER_OWNERSHIP_LEASE_DECISION_PROPOSAL.md](PD_D10_PROVIDER_OWNERSHIP_LEASE_DECISION_PROPOSAL.md) | Decision proposal: Sentinel process ownership, active lease, and shutdown | PD-D10 |

---

## 5. P4 — Resource Telemetry & Adaptive Policy

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P4_01_RESOURCE_MANAGEMENT_BASELINE_RESULT.md](P4_01_RESOURCE_MANAGEMENT_BASELINE_RESULT.md) | Resource measurement baseline, telemetry metrics, and test contracts | AC-P4-01 |
| [P4_02_NATIVE_RESOURCE_TELEMETRY_RESULT.md](P4_02_NATIVE_RESOURCE_TELEMETRY_RESULT.md) | Native platform resource monitors (Linux `/proc`, macOS `mach`/`host_statistics`, Windows `GetProcessMemoryInfo`) | AC-P4-02 |
| [P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md](P4_03_ADAPTIVE_RESOURCE_POLICY_DESIGN.md) | Deterministic adaptive policy state machine, signal evaluation, and hysteresis | AC-P4-03 |
| [P4_03_1_ADAPTIVE_POLICY_INDEPENDENT_REVIEW.md](P4_03_1_ADAPTIVE_POLICY_INDEPENDENT_REVIEW.md) | Adversarial review, RSS ascending signal fix, and saturating deadline bounds | AC-P4-03 |

---

## 6. P5 — Daemon IPC Security & Storage Reliability

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P5_01_DAEMON_IPC_BASELINE_RESULT.md](P5_01_DAEMON_IPC_BASELINE_RESULT.md) | IPC socket security baseline, permissions, and peer credential verification | AC-P5-01 |
| [P5_02_DAEMON_IPC_RELIABILITY_RESULT.md](P5_02_DAEMON_IPC_RELIABILITY_RESULT.md) | Socket reconnection, crash resilience, and connection lifetime management | AC-P5-02 |
| [P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md](P5_02_1_IPC_SECURITY_PERSISTENCE_REVIEW.md) | Hardening of SO_PEERCRED checks, UID matching, and SQLite WAL recovery | AC-P5-02 |

---

## 7. P6 — Desktop UI/UX Functional Baseline

| Audit Report | Subject / Scope | Roadmap Criteria |
|---|---|---|
| [P6_01_DESKTOP_UX_FUNCTIONAL_BASELINE.md](P6_01_DESKTOP_UX_FUNCTIONAL_BASELINE.md) | Desktop QML architecture, IPC projection, qmllint audit, and headless smoke | AC-P6-06 |

---

## 8. Platform Stabilization & Security Remediations

| Audit Report | Subject / Scope | Focus Area |
|---|---|---|
| [CI_01_LINUX_AGENT_DESCRIPTOR_REMEDIATION.md](CI_01_LINUX_AGENT_DESCRIPTOR_REMEDIATION.md) | Agent descriptor test stabilization on Fedora/Ubuntu Linux runners | Linux CI |
| [CI_02_MACOS_SANDBOX_FORMAT_REMEDIATION.md](CI_02_MACOS_SANDBOX_FORMAT_REMEDIATION.md) | Sandbox profile path formatting and clang-format stabilization on macOS | macOS CI |
| [CI_03_COMBINED_CROSS_PLATFORM_REMEDIATION_RESULT.md](CI_03_COMBINED_CROSS_PLATFORM_REMEDIATION_RESULT.md) | Unified remediation across Linux, macOS, and Windows build matrices | Cross-Platform CI |
| [CI_04_INTEGRATED_MAIN_STABILIZATION_RESULT.md](CI_04_INTEGRATED_MAIN_STABILIZATION_RESULT.md) | Stabilization of core test targets on consolidated main branch | Branch Hygiene |
| [CI_05_WINDOWS_MAIN_STABILIZATION_RESULT.md](CI_05_WINDOWS_MAIN_STABILIZATION_RESULT.md) | MSVC 2022 compatibility and Windows test skips for Unix-only primitives | Windows CI |
| [WIN_01_WINDOWS_CI_FAILURE_TRIAGE.md](WIN_01_WINDOWS_CI_FAILURE_TRIAGE.md) | Detailed triage and root cause analysis of Windows runner test failures | Windows Triage |
| [WIN_02_WINDOWS_IPC_TEST_COMPATIBILITY_RESULT.md](WIN_02_WINDOWS_IPC_TEST_COMPATIBILITY_RESULT.md) | Guarding Unix socket credentials on Windows and portable IPC contracts | Windows IPC |
| [SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md](SEC_01_HISTORICAL_PRIVACY_REMEDIATION_RESULT.md) | Repository-wide personal path and developer credential sanitization | Privacy & Security |

---

## 9. Controlled Integration & Reconciliation

| Audit Report | Subject / Scope | Focus Area |
|---|---|---|
| [INT_01_MULTI_WORKTREE_INTEGRATION_READINESS.md](INT_01_MULTI_WORKTREE_INTEGRATION_READINESS.md) | Multi-worktree divergence analysis and merge readiness audit | Integration Preflight |
| [INT_02_SEC_01_INTEGRATION_REVIEW.md](INT_02_SEC_01_INTEGRATION_REVIEW.md) | Verification of privacy fixes across active branches | Privacy Integration |
| [INT_03_SOURCE_HISTORY_RECONCILIATION.md](INT_03_SOURCE_HISTORY_RECONCILIATION.md) | Reconciliation of diverged branches and commit topology | History Hygiene |
| [INT_04_POST_MERGE_VALIDATION_RESULT.md](INT_04_POST_MERGE_VALIDATION_RESULT.md) | Full 143-test suite validation after initial branch consolidation | Regression Gate |
| [INT_05_PLATFORM_PROVIDER_RESOURCE_INTEGRATION_RESULT.md](INT_05_PLATFORM_PROVIDER_RESOURCE_INTEGRATION_RESULT.md) | Candidate integration validation across P2-M02, P3-04, P4-03.1, and CI-05 | Main Integration |
| [INT_06_CURRENT_MAIN_RECONCILIATION_RESULT.md](INT_06_CURRENT_MAIN_RECONCILIATION_RESULT.md) | Source history reconciliation and dirty worktree closure on main branch | Final Main Reconciliation |
