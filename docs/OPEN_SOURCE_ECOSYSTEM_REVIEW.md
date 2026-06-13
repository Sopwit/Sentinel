# Open Source Ecosystem Review

Phase 41.0 through Phase 41.15 is a strategic review only. It evaluates the modern
open-source AI assistant ecosystem and records architectural lessons for Sentinel. It does not
integrate external code, enable new providers, activate agents/tools, activate STT/TTS, add cloud
execution, or change runtime behavior.

## Review Baseline

Sentinel should remain a local-first, cross-platform Qt/C++ modular monolith. External ecosystems
are useful as references for patterns, UX, interoperability, and risk analysis, but Sentinel
should not become a Python agent platform, Electron app, cloud-first IDE, or autonomous execution
runner.

Primary review date: 2026-06-11.

## Adoption Matrix

| Component | Category | Classification | Sentinel position |
| --- | --- | --- | --- |
| MCP | Tool/skill protocol | Adapt | Use protocol ideas for capability declarations, transports, registries, and permission prompts. Future MCP support must be opt-in and sandboxed. |
| OpenAI MCP ecosystem | Tool/skill ecosystem | Optional | Treat as a future provider/tool interoperability layer, not a default authority path. |
| LangGraph | Agent orchestration | Adapt | Adopt graph/state/checkpoint vocabulary. Do not import Python runtime architecture. |
| Semantic Kernel / Microsoft Agent Framework | Agent/plugin orchestration | Adapt | Use plugin, connector, and process-framework ideas as inspiration for typed C++ contracts. |
| AutoGen | Multi-agent framework | Optional | Useful historically for human-agent patterns, but current upstream maintenance posture makes it inspiration-only. |
| CrewAI | Multi-agent framework | Optional | Learn from crews/flows separation; avoid role-playing/autonomous defaults in core UX. |
| OpenHands / OpenDevin derivatives | Software agent platform | Adapt | Adapt sandbox, workspace, audit, and lifecycle patterns. Avoid embedding a browser/VNC/docker agent stack into Sentinel Desktop. |
| opencode | Coding agent | Adapt | Adapt plan/read-only vs build/write agent posture and permission UX. Do not copy terminal-first assumptions. |
| aider | Coding assistant | Adapt | Adapt repo-map, diff, git-aware review loop, and patch transparency concepts. Avoid automatic commits by default. |
| Continue | Coding assistant | Optional | Useful IDE/context lessons. Repository maintenance status means inspiration-only for long-term architecture. |
| Cline | Coding assistant | Adapt | Adapt explicit approval, file diff review, and tool-call visibility. Keep VS Code-specific parts out of Sentinel core. |
| Roo Code | Coding assistant | Adapt | Adapt mode/profile and approval policy ideas. Treat extension-specific execution as out of scope. |
| Cursor concepts | Coding UX | Adapt | Adapt context assembly, review loop, and inline patch UX. Proprietary concepts only; no code reuse. |
| Windsurf concepts | Coding UX | Adapt | Adapt continuity, task-flow, and agent session UX patterns. Proprietary concepts only; no code reuse. |
| whisper.cpp | STT runtime | Adopt | Preferred future offline STT engine candidate because it is C/C++, MIT, cross-platform, and local-first. |
| faster-whisper | STT runtime | Optional | Good benchmark/reference and server-side candidate, but Python/CTranslate2 dependency weight conflicts with Sentinel core. |
| Piper | TTS runtime | Adopt | Preferred future lightweight local TTS candidate, subject to active fork/license review and voice-model licensing. |
| Kokoro | TTS runtime/model | Optional | Strong lightweight TTS option with Apache-licensed code/weights, but integration maturity and Turkish voice coverage need validation. |
| Coqui TTS | TTS toolkit | Optional | Rich multilingual research toolkit; MPL and Python/PyTorch footprint make it optional/inspiration, not core. |
| StyleTTS variants | TTS research | Avoid for core | Code can be permissive, but pretrained voice-use restrictions and cloning risks are a poor default for Sentinel. |
| Haystack | Context/RAG | Adapt | Use pipeline and explainable retrieval ideas; avoid importing Python RAG stack. |
| LangChain memory concepts | Context/memory | Optional | Useful vocabulary, but too dynamic/implicit for Sentinel's deterministic memory authority. |
| Graph memory approaches | Context/memory | Optional | Future opt-in for explainable relationship memory; never automatic hidden memory mutation. |
| Ollama | Local provider | Adopt | Keep as primary local desktop runtime boundary. Continue loopback-only, explicit readiness and model validation. |
| llama.cpp | Local runtime | Adapt | Strong future embedded/local-runtime candidate; direct embedding raises packaging, model, GPU, and update complexity. |
| LM Studio | Local provider | Optional | OpenAI-compatible local server can be supported later as a user-selected provider, not bundled. |
| OpenAI | Cloud provider | Optional | Future explicit opt-in provider behind credentials, audit, and no-background-call gates. |
| Anthropic | Cloud provider | Optional | Same cloud-provider posture as OpenAI. |
| Gemini | Cloud provider | Optional | Same cloud-provider posture as OpenAI. |
| OpenRouter | Cloud aggregator | Optional | Useful for provider interoperability, but higher trust and routing ambiguity; never default. |
| Odysseus | Self-hosted AI workspace / agent product | Inspiration only | AGPL-3.0 posture means no code copy or dependency import without separate approval. Extract only concepts such as self-hosted workspace posture, tool categories, agent planning, and workflow/recipe ideas. |

## Agent Framework Findings

Sentinel should adapt the following ideas:

- Graph-shaped state with explicit lifecycle nodes: requested, planned, awaiting approval, running,
  paused, completed, refused, failed, and cancelled.
- Durable checkpoints and resumable state, but only inside a Sentinel-owned session store.
- Human approval as a first-class state transition, not a UI interruption layered over execution.
- Capability-based agent profiles: read-only analysis, patch proposal, local runtime request,
  tool request, and privileged system request.
- Registry records that describe agents, capabilities, required permissions, risk level,
  provenance, version, and current enablement.
- Audit trails for every planned action, approval, refusal, execution attempt, output, and rollback
  recommendation.

Sentinel should not adopt:

- Autonomous multi-agent execution as a default.
- Implicit tool execution from model output.
- Hidden background planning, hidden retries, or hidden provider switching.
- Python framework ownership of core app lifecycle.
- Docker/browser/VNC workspaces as mandatory desktop architecture.

## Coding Assistant Findings

Coding assistant ecosystems converge on several useful patterns:

- Context assembly is as important as model selection.
- Patch UX must make changed files, hunks, commands, test output, and failure reasons visible.
- Good agents separate planning, editing, reviewing, and applying.
- IDE integrations are valuable because they understand open files, selection, diagnostics, and
  git state.
- Safety depends on reversible diffs, explicit approvals, scoped filesystem access, and command
  logs.

Sentinel should eventually build developer tooling around a proposal-first workflow:

1. Assemble explainable context.
2. Produce a plan.
3. Produce a patch or action proposal.
4. Ask for approval by scope.
5. Apply through a controlled local workspace boundary.
6. Run only approved verification commands.
7. Show audit, diffs, and residual risk.

Automatic commits, unreviewed broad rewrites, and unbounded shell loops should be avoided.

## Voice Ecosystem Findings

Recommended STT direction:

- `whisper.cpp` should be the primary offline STT candidate. It is MIT licensed, C/C++ native,
  supports macOS Intel/Arm, Linux, Windows, WebAssembly, mobile targets, CPU, quantization, and
  several acceleration backends. Its memory table ranges from roughly hundreds of MB for tiny/base
  models to several GB for large models.
- `faster-whisper` should remain optional for advanced/server deployments. It is efficient and MIT
  licensed, but its Python/CTranslate2/CUDA footprint is not a good fit for Sentinel Desktop core.

Recommended TTS direction:

- Piper remains the best default local TTS direction for Sentinel because it is fast, local, and
  lightweight. Future work must review the current active fork and every selected voice model's
  license before shipping.
- Kokoro is promising as an optional future high-quality lightweight TTS engine. Turkish support
  and packaging maturity must be validated before adoption.
- Coqui TTS is a broad toolkit and useful reference, but its MPL-2.0 code, PyTorch dependency
  weight, and older upstream release posture make it optional.
- StyleTTS-style voice cloning should not become a default Sentinel feature. Voice cloning and
  pretrained voice-use restrictions create consent, attribution, and abuse risks.

Turkish support:

- Whisper-family STT is multilingual and should be evaluated with Turkish test clips before any
  activation phase.
- Piper Turkish voice availability and quality must be verified per voice model.
- Kokoro and Coqui Turkish quality must be treated as unknown until measured with local tests.

## Tool And Skill Ecosystem Findings

MCP is the strongest interoperability pattern, but Sentinel should adapt it through Sentinel-owned
security boundaries:

- Tool descriptors must include name, version, provenance, permissions, input schema, output
  schema, data access scope, network scope, filesystem scope, execution scope, and audit policy.
- Tool installation and enablement must be separate. Installed does not mean active.
- Every tool invocation must carry a session id, user-visible reason, requested capability,
  approval state, and result summary.
- Remote servers and local subprocess tools require separate trust classes.
- Tool output should be treated as untrusted model context unless explicitly promoted.

OpenAI MCP, Semantic Kernel plugins, Continue tools, and OpenHands tools all reinforce the same
direction: plugin ecosystems need schemas, capability declarations, lifecycle records, approval UX,
and revocation.

## Context And Memory Findings

Sentinel should adopt a layered memory strategy:

- Deterministic retrieval remains the default for committed memory and transcript continuity.
- Semantic/vector retrieval can be added later only as an optional, explainable layer with source
  citations, confidence, and deterministic fallback.
- Summaries should be explicit artifacts with coverage ranges, freshness, source ids, and user
  visibility.
- Compression should never silently replace original transcript history.
- Salience scores should expose why an item was included or excluded.
- Graph memory may become useful for relationships, projects, entities, and preferences, but it
  must not silently infer or commit sensitive facts.

Sentinel should never make automatic memory writes from model output a default. User-authored,
user-approved, or clearly system-derived memory records need separate provenance and revocation.

## Runtime And Provider Findings

Sentinel should keep provider abstraction separate from runtime execution:

- Local providers: Ollama first, llama.cpp as a future deeper local runtime option, LM Studio as an
  optional OpenAI-compatible local endpoint.
- Cloud providers: OpenAI, Anthropic, Gemini, and OpenRouter should be future opt-in integrations.
- Provider selection must distinguish configured, selected, active, ready, refused, and failed.
- Credential readiness must not imply provider execution readiness.
- Failover must never be automatic across local/cloud boundaries. User approval is required before
  crossing from local to cloud.
- Model registries should record source, capabilities, context size, modality, local/cloud scope,
  credential requirement, and readiness evidence.

## Security Model Findings

Future Sentinel security principles:

- Default deny for tools, plugins, filesystem mutation, subprocesses, network access, cloud calls,
  microphone capture, playback, and provider fallback.
- Capability grants are scoped by provider, tool, path, network target, session, and time.
- Least privilege is enforced in C++ core policy, not only QML.
- Approval checkpoints are required before state-changing operations.
- OS credential stores are mandatory for cloud provider secrets; plaintext fallback is forbidden.
- Audit trails must be append-only from the user's perspective and include refused attempts.
- Sandboxes should be per-session and per-tool, not global.
- Tool outputs, remote server responses, and generated patches are untrusted until reviewed.

## Licensing Summary

| Project | License posture | Reuse feasibility | Sentinel category |
| --- | --- | --- | --- |
| opencode | MIT | Code reuse possible with attribution, but architecture/language mismatch favors inspiration. | Safe as inspiration |
| OpenHands | MIT | Code reuse possible with attribution; heavy platform stack makes direct reuse unlikely. | Safe as inspiration |
| AutoGen | MIT for code, CC-BY-4.0 for docs/content | Code reuse possible with attribution; maintenance mode reduces adoption value. | Safe as inspiration |
| CrewAI | MIT | Code reuse possible with attribution; Python/autonomy defaults unsuitable for core. | Safe as inspiration |
| LangGraph | MIT | Code reuse possible with attribution; Python runtime not aligned with Qt/C++. | Safe as inspiration |
| Semantic Kernel | MIT | Code reuse possible with attribution; .NET/Python/Java stack not core-aligned. | Safe as inspiration |
| aider | Apache-2.0 | Reuse possible with license/notice/patent terms; best used as UX/architecture reference. | Safe as inspiration |
| Continue | Apache-2.0 | Reuse possible with license/notice/patent terms; read-only maintenance posture. | Safe as inspiration |
| Cline | Apache-2.0 | Reuse possible subject to notices; VS Code extension assumptions limit direct reuse. | Safe as inspiration |
| Roo Code | Apache-2.0 | Reuse possible subject to notices; extension architecture limits direct reuse. | Safe as inspiration |
| whisper.cpp | MIT | Strong candidate for future native dependency after packaging/security review. | Safe to reuse |
| faster-whisper | MIT | Optional backend; Python dependency makes direct desktop reuse less attractive. | Safe as optional |
| Piper | MIT in original repo; active fork must be reviewed | Good future dependency candidate; voice model licenses must be checked separately. | Safe to reuse with review |
| Kokoro | Apache-2.0 code and advertised Apache-licensed weights | Optional candidate; model and voice coverage need validation. | Safe as optional |
| Coqui TTS | MPL-2.0 | File-level copyleft obligations; dependency weight. | Optional/inspiration |
| StyleTTS2 | MIT code, additional pretrained model voice-use conditions | Code inspiration possible; pretrained model use is risky. | Avoid for core |
| MCP SDKs | Mixed MIT/Apache-2.0 depending SDK/repo | Protocol adoption preferred over direct dependency initially. | Adapt |
| OpenAI MCP ecosystem | Mixed by server/project plus OpenAI service terms where applicable | Review each server independently; do not assume one ecosystem-wide license. | Optional/inspiration |
| Haystack | Apache-2.0 | Reuse possible with notices; Python stack not core-aligned. | Safe as inspiration |
| LangChain | MIT | Reuse possible with attribution; dynamic Python abstractions not core-aligned. | Optional inspiration |
| Ollama | MIT | Existing local integration target; do not vendor unless separately scoped. | Adopt |
| llama.cpp | MIT | Strong future native dependency candidate after packaging/GPU/model review. | Adapt |
| LM Studio | Proprietary app with local server/API documentation | No code reuse. Treat only as an optional user-installed provider endpoint. | Safe as concept/integration target |
| Cursor | Proprietary product | No code reuse. Product/UX concepts only. | Safe as inspiration only |
| Windsurf | Proprietary product | No code reuse. Product/UX concepts only. | Safe as inspiration only |
| OpenAI API | Proprietary cloud service/API terms | No code reuse. Optional provider integration only after credential, billing, privacy, and policy review. | Optional provider |
| Anthropic API | Proprietary cloud service/API terms | No code reuse. Optional provider integration only after credential, billing, privacy, and policy review. | Optional provider |
| Gemini API | Proprietary cloud service/API terms | No code reuse. Optional provider integration only after credential, billing, privacy, and policy review. | Optional provider |
| OpenRouter | Proprietary aggregator/API terms | No code reuse. Optional aggregator only after routing, logging, privacy, and failover review. | Optional provider |

License obligations before implementation:

- MIT: preserve copyright and license notices in redistributed source/binaries.
- Apache-2.0: preserve license and notices, track NOTICE files if present, respect patent terms.
- MPL-2.0: modifications to MPL-covered files must remain under MPL; evaluate compatibility before
  vendoring.
- Model weights and voice files have separate licenses from code and must be reviewed individually.
- Proprietary products such as Cursor and Windsurf are concept references only; no code reuse.

## Source Notes

- opencode: https://github.com/anomalyco/opencode
- OpenHands: https://github.com/OpenHands/OpenHands
- AutoGen: https://github.com/microsoft/autogen
- CrewAI: https://github.com/crewAIInc/crewAI
- LangGraph: https://github.com/langchain-ai/langgraph
- Semantic Kernel: https://github.com/microsoft/semantic-kernel
- aider: https://github.com/Aider-AI/aider
- Continue: https://github.com/continuedev/continue
- whisper.cpp: https://github.com/ggml-org/whisper.cpp
- faster-whisper: https://github.com/SYSTRAN/faster-whisper
- Piper: https://github.com/rhasspy/piper
- Kokoro: https://github.com/hexgrad/kokoro
- Coqui TTS: https://github.com/coqui-ai/TTS
- StyleTTS2: https://github.com/yl4579/StyleTTS2
- MCP: https://modelcontextprotocol.io and https://github.com/modelcontextprotocol
- Cline: https://github.com/cline/cline
- Roo Code: https://github.com/RooCodeInc/Roo-Code
- Cursor concepts: https://cursor.com
- Windsurf/Devin concepts: https://docs.windsurf.com
- Ollama: https://github.com/ollama/ollama
- llama.cpp: https://github.com/ggml-org/llama.cpp
- LM Studio: https://lmstudio.ai/docs
- OpenAI API: https://platform.openai.com/docs
- Anthropic API: https://platform.claude.com/docs
- Gemini API: https://ai.google.dev/gemini-api/docs
- OpenRouter: https://openrouter.ai/docs
- Haystack: https://github.com/deepset-ai/haystack
- LangChain: https://github.com/langchain-ai/langchain
