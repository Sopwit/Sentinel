# SENTINEL — SETTINGS / ONBOARDING / SECRETS / NETWORK / RECOVERY REPORT

Date: 2026-10-03. Platform: macOS 27.0.1, Qt 6.11.2.

## Evidence scope

Real production QML surfaces and DesktopShellViewModel were exercised using the opt-in `settings_ui_certification_probe` application, disposable JSON/SQLite profiles, and an explicitly injected in-memory credential backend. Computer-use verification covered clicks, keyboard focus, language changes, a narrow window, quit/relaunch and first-run flow. This is not a certification of the complete shipping desktop bootstrap. No model/provider success was simulated in this UI harness; LM Studio remained unavailable with no selectable model. The user's running Sentinel application and profile were not changed.

Backend evidence is in `test_settings_recovery_runtime`, the existing owner suites, and the final CTest log. The local HTTP test uses a deterministic TCP server and the production inference transport, not a real LLM. No live cloud success is claimed. Existing unrelated working-tree changes were preserved.

## 1. Settings

- Persistence: PASS within tested scope. SettingsService delegates to AppSettings and ISettingsStore/JsonSettingsStore; QML does not independently persist values. Conversations, memory, Agent runs, grants and secure credentials remain separate stores. Defaults, validation and migration are owned by AppSettings and its store; reset is exposed by SettingsService.
- UI round-trip: language, theme and selected unavailable provider survived quit/relaunch. Service/disk round-trip additionally covered network mode, provider/model identity and workspace preferences. There is no network-mode control in the inspected Settings QML; this setting was exercised through its service API, not invented UI.
- Invalid values: safe defaults preserved unrelated valid values. Malformed JSON produced a truthful storage error without overwriting original bytes. An invalid destination produced StoreUnavailable and rolled back the attempted setting.
- Reset: targeted language reset preserved unrelated settings.

## 2. Onboarding

- First run: PASS in disposable product-surface harness. Welcome and the local-first route appeared; no cloud account or ready provider was required.
- Resume: closing at step 2 and relaunching resumed step 2.
- Completion: all seven existing steps completed and persisted; onboarding did not repeat on relaunch. Optional speech was left unchanged.
- Edges: an unavailable local provider/no model was represented truthfully. Backend write failure now leaves onboarding incomplete and exposes its error; the QML completion action no longer dismisses an unsuccessfully saved flow. Exhaustive full-desktop cloud/close edge combinations were not rerun.

## 3. Localization

- EN/TR: actual runtime language switching, Settings, onboarding, empty Chat and model-missing readiness were verified. Keyboard focus was visible; the 780-pixel Settings window remained usable and scrollable.
- Fixed 24 targeted catalog entries per language, covering missing appearance labels, local-first/privacy welcome cards, Chat empty/model labels and translated controller readiness messages. Backend readiness-change notification now refreshes translated Chat placeholders.
- Compiled catalogs: EN 803 finished, TR 849 finished, zero unfinished entries. This does not imply complete source coverage.
- Scoped lupdate audit of QML and desktop/app sources found 1,047 current messages, with 343 absent EN catalog entries and 297 absent TR entries. English source fallback is not inherently a defect, but TR coverage remains incomplete. Exhaustive Agent/permission/error localization and layout certification is NOT VALIDATED; no broad translation rewrite was performed.

## 4. Secrets

- New credential writes: fake-backed integration verified secure-store ownership, reference/presence surfaces, deletion and truthful unavailable-store errors. New credentials were absent from settings and backup JSON; test secrets were never logged or printed.
- Native macOS Keychain: the existing smoke test stored, read and deleted a safe test credential using a unique UUID identity. PASS, no Keychain skip in this run. This does not prove interactive access to existing user-profile keys will never prompt.
- The DesktopShell fixture previously reached the native Keychain through a ModelService with no shared test settings. It now uses an explicit CredentialStore seam and one settings-linked ModelService. Production's default native backend remains unchanged; no secure storage was disabled.
- Legacy migration: successful migration removes plaintext. If native storage fails, the existing loss-avoidance contract preserves the original plaintext and reports MigrationRequired; the legacy credential remains readable. This was reproduced with a safe fixture and is remaining security debt, NOT secure-persistence PASS. Changing that failure/access contract was not silently imposed.
- Plugin credential-positive runtime remains the prior known residual; no claim of new validation.

## 5. Network Modes

- Online: production local transport reached a deterministic loopback server; absent cloud credentials were rejected before HTTP.
- Local Only: loopback local transport succeeded; cloud was rejected by policy before HTTP, with no substitution.
- Offline: current policy still permits loopback local inference; the production transport succeeded against the local fixture. External/cloud transport was blocked. Offline does not currently mean all loopback traffic is prohibited.
- Active-run transition: NOT VALIDATED. The inspected UI does not expose mode switching. NetworkPolicyService is mutable process-wide and transports consult it per request/retry; a frozen network contract across an active run is not established by this certification. Provider/model binding remains independently authoritative; no fallback was added.

## 6. Provider Readiness

- Real QML selected LM Studio with no available model, displayed truthful unavailability and disabled Send; it did not substitute another provider.
- Missing/deleted credentials and network blocking have deterministic owner/transport evidence. No empty fake credential request was sent to a cloud endpoint.
- Existing provider/Agent suites passed. Positive cloud or real LLM runtime was not repeated in this phase; prior runtime baselines are not being relabeled as new evidence.

## 7. Backup

- PASS for the current backend contract: real JSON file creation and structural validation using disposable stores.
- Supported domains: settings subset (theme/language/network), workspace profiles/presets, portable extension/MCP configuration and credential references, chat and memory. References are not secret values.
- Not exported: raw credentials, models/cache, full workspace catalog, Agent run metadata, plugin/skill enabled state or permission grants. Do not interpret portable extension configuration as a complete plugin backup.
- Invalid destination returns a truthful failure. No backup/import file-picker product surface was found; testing used existing backend APIs.

## 8. Import / Restore

- PASS for supported combinations: export, modify state, import settings/workspace profiles/memory with replacement and chat with merge. Actual SQLite conversation data was restored/merged.
- Malformed import was rejected without changing the participating stores. Credential import is unsupported; chat replacement is explicitly unsupported. This is not a blanket guarantee of transactionality for every hypothetical multi-store failure.

## 9. Recovery

- Missing DB: real SQLite stores created cleanly.
- Malformed settings/DB: truthful unavailable/error state; original corrupt bytes retained.
- Interrupted state: reopening a real persisted Streaming conversation recovered it as Interrupted/Recoverable, retained partial output and did not replay the request.
- Invalid/incomplete JSON import rejection was covered. Unsafe stale-WAL/SHM corruption scenarios were NOT VALIDATED; no user database was altered.

## 10. Retention / Clear

- Real SQLite retention boundaries: old terminal Agent runs removed; recent and active runs retained. Old completed conversations removed by conversation updated_at; recent and streaming conversations retained.
- Clear Chat removed only chat data; Clear Local Memory removed only memory. Cross-store settings and the other data store remained intact.
- Memory has no automatic TTL under the current RetentionPolicy contract. Diagnostics/model-cache cleanup runtime and every filesystem retention boundary were NOT VALIDATED.
- Privacy/network/retention backend surfaces exist, but the inspected QML does not expose all of them. Missing product surfaces were recorded, not redesigned.

## 11. Defects Fixed

1. SettingsService reported accepted success after a failed persistence write: now reports the owning storage error.
2. Onboarding mutations and DesktopShell completion ignored persistence failure; the completion dialog could disappear anyway: service/view-model/QML now retain truthful incomplete state and error.
3. Missing targeted EN/TR messages and hardcoded Chat readiness placeholders: use the existing translation path and refresh signal.
4. Unit/integration fixture accidentally accessed native Keychain: explicit secure-store dependency injection and shared settings remove interactive dependency without changing production storage.

Deterministic regression coverage was added for these failures and the tested persistence/recovery/security boundaries. No new feature, provider, credential, dependency or audio behavior was introduced.

## 12. Regression

- Configure/build: `cmake --preset tests`, `cmake --build --preset tests -j4`: PASS.
- Settings/Security/Network/Persistence: PASS. New `test_settings_recovery_runtime`: 16 passed, 0 failed, 0 skipped.
- Agent/MCP/Plugins/Skills/Chat owner suites: PASS at CTest target level. This preserves automated baselines, not a new real-provider completion claim.
- Full: **105/105 targets PASS**, 99.03 seconds, `ctest --preset tests --output-on-failure --timeout 60`.
- ApplicationController: **127/0/0**. DesktopShellViewModel: **73/0/0**. Native credential suite: **10/0/0**.
- Existing case-level skips: four shell/Docker AgentRuntime fixtures incompatible with strict macOS no-fork containment; two missing-dependency negative branches unreachable because Docker/npx are installed. They were not represented as executed PASS cases. No Keychain-driven skip was needed in this run.
- QML lint target: exit 0 with existing warnings; not warning-free certification.
- `git diff --check`: PASS.
- Owned disposable UI helper shut down normally. Final process check found no certification helper or sentinel-plugin-host process. The user's Sentinel process was left untouched.

## 13. Remaining Blocks

Full TR coverage and exhaustive Agent/permission/error localization; frozen active-run network semantics; full desktop bootstrap/profile coverage; legacy plaintext migration failure contract; existing-profile interactive Keychain access; real cloud-positive runtime; WAL/SHM corruption and diagnostics/cache retention; unsupported/missing backup and privacy UI surfaces. Prior plugin credential-positive and abrupt-parent-death residuals remain unchanged, not new regressions.

Settings/onboarding tested scope: FIXED + PASS. New secret persistence, native Keychain smoke, policy/local transport and supported backup/recovery/retention scope: PASS. Complete requested certification: **PARTIAL**.

FINAL VERDICT: **PARTIAL** — proven defects fixed and all reachable automated targets pass; the unvalidated/security-residual items above prevent an unconditional PASS.
