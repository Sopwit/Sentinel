# V3.2.1 context reliability

## Reproducible capacity mismatch

The V3.2 source edit was real, but its continuation failed with “Context size has been exceeded”, mapped to CapabilityUnsupported. Its saved error contains a nested engine code 500; the original HTTP transport status was not independently recorded. A V3.2.1 reproduction received outer HTTP 400 with nested engine code 500. The exact original prompt token count, loaded-window snapshot and generation default were not recorded; they cannot be reconstructed as measurements.

Read-only observation on 2026-10-10: LM Studio `/api/v0/models` reported `nvidia/nemotron-3-nano-4b` loaded, published maximum 1048576, loaded context 8192. `/api/v1/models` independently reported `loaded_instances[].config.context_length=8192`. Other local models were catalogued but not loaded; no model/server settings were changed. The V3.2 native catalog adapter read only max_context_length. The selected model's theoretical capacity therefore incorrectly informed planning budgets. This is an assembly/budget defect interacting with an actual configured runtime limit, not proof the model cannot support larger contexts.

Native schemas and the native last-batch tool-results envelope were additional to ContextEngine's prompt estimate. The OpenAI-compatible native request did not specify max_tokens, so the original generation reservation was unspecified. Native observations could contribute up to 6800 characters per result, duplicated in budgeted planner observations. Execution history is selected by ContextEngine; the native protocol retains the last batch, not the whole call transcript. No original per-component token counts are asserted.

## Fixes

- Native catalog discovery now prefers the minimum positive loaded-instance context over the published maximum. No loaded snapshot means only published metadata is known; it is not initialization/readiness attestation. Active ModelBinding remains frozen; no silent provider/model switch.
- OpenAI-compatible native requests explicitly reserve up to 1024 output tokens (bounded further by a reported model output maximum).
- The serialized native body, including tool schemas, prompt and last-batch results, receives a conservative size estimate `ceil(UTF8 bytes/3)+256`. If this estimate plus output reservation exceeds the binding window, no inference request is sent. This is a heuristic guard, not actual tokenizer accounting or guaranteed capacity negotiation. Provider errors remain authoritative.
- Native top-level tool descriptions are bounded to 200 characters. Authoritative schemas and validation constraints remain intact.
- Last-batch result presentation uses a 2000-character excerpt with an explicit truncation marker and narrower-observation guidance. The full tool record/evidence is retained; an excerpt does not turn partial evidence into complete evidence.
- Known provider context-exhaustion messages and preflight failures stop with a clear context-exhausted explanation, preserving partial work and directing users to inspect changes before a smaller task. They never cause automatic mutation retry, provider fallback or implicit summarization.

## Evidence and limits

See [live evidence](../reviews/agent-v321-2026-10-10/README.md) and [final report](TUI_V3_2_1_FINAL_REPORT.md). The local forwarding harness records only byte contributions, max_tokens, status, actual provider usage when present, and public tool names. It does not save private reasoning or raw planner prompts. Component byte fields are re-encoded JSON sizes and are not an additive wire decomposition; body_bytes is the actual received wire size. Byte contribution is not token contribution. Legacy V3.2 token usage remains unavailable.

UNIT PASS coverage: loaded-vs-published windows (including multiple loaded instances and unloaded fallback), context failure stops after one request, oversized native results are explicitly excerpted while the original record is unchanged, and an oversized serialized native request is rejected before a network call. Full context compaction, per-provider tokenizers, paged schemas, durable resumable mutation IDs and loaded-instance limit refresh require a separate design; no V3.3 implementation was started.

## Measured native requests

Task A completed with two real reads and three native planner requests. Their actual provider input counts were 5395, 5774 and 5992 tokens; completion counts were 310, 93 and 193. Each native request reserved 1024 output tokens. The largest measured input plus reservation was 7016/8192, approximately 85.6% of the reported window. Classifier usage was separately 413 input / 558 completion tokens and did not use the new native output reservation.

Actual native wire bodies were 16818, 17813 and 18327 bytes. Re-encoded schema JSON was 15305 bytes in each request; last-batch tool content was 0, 76 and 93 bytes. These are byte measurements, not separately measured schema/history tokens. No exact history contribution or provider default generation limit is available.

Task D still received a context-size rejection for a 16783-byte native request with the same 1024-token reservation. Similar-sized A requests succeeded. Loaded-window correction alone therefore does not explain all provider failures. Backend cache/parallelism and proxy cancellation effects were not established as causes. The metadata proxy cannot abort an upstream blocked read; direct-endpoint follow-ups are reported separately. The request is safely failed without retry or model switching, but robust capacity negotiation remains PARTIAL.
