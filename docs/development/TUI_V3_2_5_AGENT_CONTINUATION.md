# V3.2.5 Agent continuation

Date: 2026-10-10–11 (Europe/Istanbul; trace timestamps are UTC). Acceptance is measured separately in the live report.

Classification remains an evidence contract. A successful result initializes the planner intent; it is not a final answer. An indeterminate result terminates before planning. AgentLoop builds the context and invokes LlmAgentRuntime, which receives available registered/allowed tool schemas on supported native bindings. Validated calls pass through the authoritative gateway. Runtime-generated call IDs associate results with the active turn; native continuation contains the actual observed result, while structured evidence remains authoritative for grounding.

Existing typed explanations require current successful read call IDs and exact nonempty source quotes; paths are derived from those reads. Invented/foreign/stale references fail. This is provenance validation, not proof that a natural-language interpretation is correct. Live fixture evaluation must separately verify task facts. Existing generic receipts can still be accepted by the runtime and do not count as successful explanations in this phase.

## Bounded planning transport

Inspection found that native requests ignored ChatRequestOptions.deadlineMs and maxOutputTokens, unlike plain classification. A successful classifier could therefore enter an unbounded native wait. Selected-endpoint native requests now use the requested deadline and output reservation. Claude/Gemini request builders also carry explicit output limits when supplied. Planner calls request a nominal 120000-ms deadline and 1024-token output bound; the OpenAI-compatible builder also clamps the output reservation to the binding-reported model limit. This is a finite request lifetime, not a whole-run wall-clock guarantee. Existing bounded action/repair/step limits remain. Transport cancellation cannot establish upstream inference termination.

The private planner streaming shortcut cannot carry a per-request deadline; bounded planner calls use sendRequest. No planner text is exposed before an accepted AgentLoop final. No uncertain write is automatically replayed. Existing transient transport recovery remains limited; a provider generation retry is not a gateway tool replay.

## Regression contracts

- Long conversation preserves the trusted workspace-root suffix and distinguishes file content from existence in the classifier prompt.
- A successful classification requiring a file observation reaches planning but cannot complete with an invented answer and no observation.
- Existing synchronous/asynchronous indeterminate-classification tests require zero planner calls/tools and a terminal failure.
- A real localhost server accepting a native request without responding exercises the per-request 100-ms deadline, one attempt, Timeout category and 128-token reservation.
- Existing real read-gateway/exact-quote tests, output-limit/empty/malformed/context preflight tests, cancellation and runtime/IPC suites remain required.

Opt-in production classifier probe `test_observation_policy optInLiveClassification` requires SENTINEL_LIVE_CLASSIFIER_MODEL. It calls the actual production policy/provider with built-in descriptors, the synthetic goal/root and 8192 binding context. It is skipped in normal deterministic regression runs and is not a fake provider success fixture.
