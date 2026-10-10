# V3.2.3 final report

Verdict: **IMPROVED — FURTHER FIXES REQUIRED**. Safe macOS Agent build execution is still BLOCKED. No end-to-end coding acceptance or competitor parity is claimed. This phase stops before V3.3, without committing, pushing or merging.

## Scope and changes

The baseline was clean fbfc41d8. The architecture/runtime/security/build/testing instructions and V3.2.2 evidence were reviewed first. Personal-Brain CLI was unavailable; the user-provided Sentinel checkout was used. Qt/QML Desktop, Quick Panel, Rust/Ratatui layout and keyboard contract are unchanged. There is no second shell path, additional grant, model fallback, context increase, or automatic uncertain-write retry.

**IMPLEMENTED + VERIFIED — narrow C++ fixes:**

1. Opt-in allowlisted OpenAI-compatible native provider/Agent diagnostics, attached to the internal ChatProviderReply. Actual request size, labelled context estimate, output budget, observation sizes, continuation/recovery count, known finish reason, real usage when provided, HTTP status and terminal category are observable without persisting prompts, credentials, response content or reasoning. IPC remains authoritative and unchanged.
2. Successful HTTP status had been discarded by postJsonOnce and completeOpenAiChat. The real relay observed HTTP200 while native diagnostics reported zero; a real local HTTP regression failed before the fix and now passes. Actual status is retained, including malformed-JSON responses, rather than fabricated as 200.
3. finish_reason=length is rejected distinctly as output_limit. Even apparently complete tool calls in a length-finished response are not executed. Empty response, malformed response, context rejection, unrelated schema rejection, cancellation and timeout remain distinguishable. No budget increase or write replay was introduced.
4. read-file no longer advertises directories; it directs directory discovery to list-directory and warns against invented /workspace aliases. list-directory now documents '.' and workspace-relative paths. Parameter names, schema validation, file authorization and handlers are unchanged.

HTTP/local registry regression evidence is distinct from live model acceptance. The historical V3.2.2 empty response still has no recorded finish reason and is not retrospectively explained by the new length fixture.

## Execution environment

**BLOCKED — macOS:** repeated real gateway probes independently establish CMake executable/dependency confinement and fork denial under required process-tree policy. Process groups cannot guarantee ownership of detached descendants; removing fork denial would weaken the threat model. No policy was relaxed. The [execution report](TUI_V3_2_3_EXECUTION_ENVIRONMENT.md) specifies a future canonical executable/argv capability, bounded filesystem/toolchain environment, resource/output/network limits, audited exact approvals and descendant ownership surviving cancellation, timeout and supervisor crash. That design is **SPECIFIED ONLY**.

**NOT VALIDATED — Linux:** Docker CLI exists but its desktop-linux daemon socket does not. No usable Linux runner was found. Source inspection of Bubblewrap confinement is not Linux acceptance.

## Live model comparison

Both already-installed models were loaded sequentially at an actually reported 8192 context/four slots; Qwen loading had explicit user approval. No download or silent fallback occurred. The original empty loaded-model state was restored after testing.

**EXISTING + REUSED:** actual release Rust TUI, typed IPC, private portable daemon profiles, AgentRuntime, authoritative gateway/policies and disposable C++20/CMake fixtures. The initial five journeys per model used an identical frozen instrumented daemon and goals/permissions. No edits, process approvals or accepted finals occurred there. Nemotron produced 18 completed/eight failed tool results, with read loops and a local estimated-context refusal; Qwen's five observations hit the observer bound with no tool results.

**PARTIALLY VERIFIED — direct controls:** both models answer a bounded transport-only request with HTTP200/stop. On the final-source direct TUI repeat, Nemotron observes all three files and reaches run.completed after six reads, but the authoritative final is a file receipt rather than the requested explanation (**PARTIAL** user journey). Qwen's direct A reaches no tools and is cancelled at the bound. Its owned daemon stack isolates the wait to plain ObservationIntentPolicy classification before native planning. Direct B–E were gated off after zero tool progress; they are not marked as passes.

The relay does not guarantee upstream cancellation and the initial matrix overlapped developer builds. Therefore the direct control was necessary; neither timing nor model-capability parity is inferred. Provider reason for the classification delay remains unknown. The plain local classification body omits an output bound and has no default client deadline; bounded plain-request authority/metadata and fail-closed indeterminate outcomes are the next reliability work. The native 1024-token limit has not yet been reached in Qwen's stalled path.

No Agent edited multiple files or executed a successful build/test in this phase. D's real host baseline only establishes a failing fixture. F remains blocked by confinement. None of these facts is replaced by unit-test success.

## Verification

| Gate | Actual result |
|---|---|
| Tests/Debug configure and full build | PASS |
| Release configure and full build | PASS |
| Changed C++ ranges / git diff whitespace | PASS |
| Focused Agent/runtime QtTest cases | 41 PASS |
| Focused tool schema/validator QtTest cases | 9 PASS |
| Full C++/IPC CTest | 121/122 PASS; test_upgrade terminated while blocked in macOS Keychain |
| Non-Keychain upgrade compatibility/migration subtests | 2 PASS, plus setup/cleanup |
| Rust formatting, Clippy, Debug/Release builds | PASS |
| Rust tests | 73 PASS |

The full CTest invocation returns failure and is not reported as green. The unchanged test_upgrade encryption test waits in SecItemCopyMatching; its stack was sampled, then only that owned process was terminated. No Keychain access was auto-approved, no secret was displayed, and Keychain permissions/data were not changed. Hermetic credential testing or a separately authorized isolated Keychain is needed to close that gate.

## Evidence and next work

[Acceptance report](TUI_V3_2_3_AGENT_ACCEPTANCE.md), [provider diagnostics](TUI_V3_2_3_PROVIDER_DIAGNOSTICS.md), [raw evidence and reproduction](../reviews/agent-v323-2026-10-10/README.md), and [remaining gaps](TUI_V3_2_REMAINING_GAPS.md) separate deterministic tests, real-model runs and blocked platform journeys. Both comparison profiles' frozen binary hashes match. Final build hashes are recorded separately; late fixes are not retroactively credited to the comparison binary.

Priority remains authoritative process containment and bounded, observable intent classification/native continuation under actual loaded limits. Reliable context accounting and outcome-aware grounding need further evidence. The current private-store boundary and explicit permissions should remain intact. Existing presentation gaps observed in actual PTY output include clipped long notices, literal Markdown in failure messages and stale cancellation-waiting notice text after a cancelled terminal event. These were recorded, not redesigned in this phase.


Recommended next reliability batch, before V3.3: bound and instrument plain intent-classification requests; exercise that exact request under the approved loaded limits; add a typed evidence-backed explanation projection; obtain a usable Linux runner and separately review macOS descendant containment; isolate credential tests from the user's Keychain. Do not solve any of these by bypassing daemon authority, relaxing permissions or trusting unverified model prose.
