# Sentinel Long-Term Strategy

Phase 41.0 through Phase 41.15 defines strategic direction after reviewing the open-source AI
assistant ecosystem. This document is a roadmap reference only. It does not authorize execution,
providers, agents, tools, plugins, STT/TTS, cloud calls, or runtime behavior changes.

## Strategic North Star

Sentinel should become a production-grade cross-platform AI assistant with:

- Local-first operation.
- Explicit user authority.
- Native desktop UX through Qt/QML.
- Conversation continuity and explainable context.
- Memory systems with provenance and revocation.
- Agent workflows with approval checkpoints.
- Developer tooling based on patch proposals and audit logs.
- Voice interaction through local-first STT/TTS.
- Plugin/skill extensibility through capability declarations.
- Future provider interoperability without cloud-by-default behavior.

Phase 49.5 product realignment sharpens this north star: Sentinel should feel like a premium native
AI desktop companion with a large chat-first Home, Brain for memory/context/continuity, Agents for
metadata-only planning readiness, and Settings as a floating modal. Advanced/Diagnostics copy
replaces Advanced Diagnostics wording in user-facing surfaces. The realignment does not authorize new
runtime execution, tools, agents, model management, cloud calls, telemetry, update checks, or
background work.

Phase 49.6 turns that direction into a daily-use native UX baseline: chat-first Home, command
palette, modal Settings, usable themes, first-run onboarding, local recovery, notification center,
activity timeline, updates/about, backup metadata, and expanded exports. These remain local and
foreground-only foundations; they do not authorize autonomous agents, tool execution, hidden
background behavior, cloud calls, telemetry, update polling, workspace scans, model downloads, or
voice execution.

## Architecture Position

Sentinel should remain a C++20/Qt modular monolith with explicit service boundaries:

- Providers stay behind provider/runtime interfaces.
- Tools stay behind tool descriptors, permission policy, sandbox policy, and execution boundaries.
- Memory stays separate from chat history and settings.
- Voice stays behind STT/TTS provider boundaries and audio permission policy.
- Plugins/skills are described and approved before they can run.
- QML receives view-model-safe summaries only.

External frameworks should influence Sentinel's vocabulary and UX, not replace the app core.

## Long-Term System Shape

Future Sentinel should have these layers:

1. Desktop shell: Qt/QML presentation, local settings, safe status, approvals, diffs, transcripts.
2. Core policy layer: permissions, safety, audit, provider readiness, tool readiness, memory
   provenance, session state.
3. Runtime boundary layer: local provider clients, cloud provider clients, local model runtimes,
   voice runtimes, tool runners, plugin hosts.
4. Storage layer: settings, chat history, committed memory, summaries, audit logs, provider/model
   registries, plugin manifests.
5. Optional interoperability layer: MCP clients/servers, OpenAI-compatible endpoints, local
   runtime APIs, external IDE bridges.

## Recommended Future Phases

| Roadmap area | Recommended phases | Scope |
| --- | --- | --- |
| Phase 42 | Ecosystem policy checkpoint | Convert Phase 41 findings into non-executing policy records: adoption status, license posture, dependency review checklist, and source registry metadata. |
| Phase 43 | Plugin/skill manifest foundation | Add local manifest schema metadata for skills/plugins with capability declarations and disabled lifecycle states. No loading or execution. |
| Phase 44 | Tool approval UX foundation | Build richer approval records and UI summaries for proposed tool actions. No real tools. |
| Phase 45 | Audit log persistence | Persist local audit records for refused/proposed/approved metadata events. No execution activation. |
| Phase 46 | Developer patch proposal planning | Add metadata-only patch proposal records, hunk summaries, risk summaries, and review states. No filesystem writes from agents. |
| Phase 47 | Local workspace sandbox design | Define path scopes, session ids, process policy, network policy, and rollback metadata. No sandbox execution. |
| Phase 48 | MCP compatibility planning | Add MCP descriptor/import metadata and trust classifications. No MCP server/client execution. |
| Phase 49 | Voice engine selection checkpoint | Evaluate whisper.cpp, Piper, Kokoro, model licenses, Turkish quality, packaging, and latency on target platforms. No activation. |
| Phase 50 | Optional semantic retrieval planning | Define vector index metadata, source attribution, explainability, deletion, and fallback rules. No embeddings yet. |
| Phase 51 | Cloud provider activation design | Define credential entry, OS keychain backends, provider test-call approvals, billing warnings, and local/cloud boundary UX. No cloud calls until explicitly scoped. |
| Phase 52 | Controlled local tool execution pilot | Only after sandbox, approval, audit, and rollback policies are implemented and tested. Start with one narrow read-only tool. |
| Phase 53 | Controlled STT/TTS pilot | Only after explicit microphone/playback permissions, model license review, and local-only runtime gates. |
| Phase 54 | Developer assistant pilot | Proposal-first coding workflow with explicit patch application and approved verification commands. |
| Phase 55 | Agent workflow pilot | Single-agent foreground workflow before multi-agent orchestration. No autonomous background work by default. |

## Technical Trade-Offs

### Agent Orchestration

Benefits:

- Graph state and checkpoints improve resumability and explainability.
- Agent registries make capabilities visible before activation.
- Human approval states reduce accidental authority escalation.

Costs and risks:

- Stateful agents increase persistence, migration, and audit complexity.
- Multi-agent coordination can obscure responsibility if not surfaced clearly.
- Framework parity pressure can pull Sentinel toward Python service architecture.

Decision:

Adapt graph/state/approval patterns in C++ core. Do not embed LangGraph, CrewAI, AutoGen, or
OpenHands as runtime dependencies.

### Developer Tooling

Benefits:

- Patch proposal workflows make AI edits reviewable and reversible.
- Diff-first UX fits explicit user authority.
- Test-command approval creates a clean boundary between suggestions and execution.

Costs and risks:

- Reliable code context assembly is hard and can become slow.
- Patch application must handle conflicts, user edits, binary files, and generated files.
- Command execution requires sandboxing and audit before it is safe.

Decision:

Build proposal-first developer tooling. Avoid automatic commits, hidden edits, and unbounded shell
loops.

### Voice

Benefits:

- Offline STT/TTS strengthens local-first assistant identity.
- whisper.cpp and Piper align with native/local architecture.
- Turkish voice support can become a product differentiator if tested carefully.

Costs and risks:

- Voice model licenses vary independently from code.
- Microphone and playback permissions are high-trust surfaces.
- Latency, memory use, and model downloads can degrade desktop experience.

Decision:

Prefer whisper.cpp for STT and Piper for baseline TTS. Keep Kokoro optional. Treat Coqui and
StyleTTS as research/reference, not default runtime direction.

### Tool And Skill Extensibility

Benefits:

- MCP-style descriptors improve interoperability.
- Capability declarations allow least-privilege approvals.
- Plugin lifecycle records support installation, enablement, revocation, and audit.

Costs and risks:

- Tool ecosystems expand attack surface quickly.
- Remote MCP servers can leak data or inject untrusted content.
- Plugin compatibility and versioning create maintenance load.

Decision:

Adapt MCP concepts behind Sentinel's own permission, sandbox, and audit policies. Installed tools
must remain inert until explicitly enabled and approved.

### Context And Memory

Benefits:

- Deterministic retrieval keeps context explainable.
- Optional semantic retrieval can improve recall when bounded and cited.
- Summaries and graph memory can support long-term continuity.

Costs and risks:

- Automatic memory writes can create false, sensitive, or unwanted facts.
- Vector retrieval can be hard to explain and delete cleanly.
- Compression can silently distort past conversations if not source-linked.

Decision:

Keep deterministic retrieval as default. Add semantic/vector or graph memory only as optional,
source-linked, explainable layers. Never make hidden memory mutation automatic.

### Providers And Runtimes

Benefits:

- Provider abstraction supports local/cloud interoperability.
- Local providers preserve privacy and offline operation.
- Cloud providers can offer stronger models when users explicitly opt in.

Costs and risks:

- Provider failover can violate user expectations and privacy.
- Credentials require OS-native secure storage and careful UI.
- Model capability registries can drift from reality.

Decision:

Ollama remains the primary local provider. llama.cpp is a future native-runtime option. Cloud
providers are optional and require explicit credentials, user approval, audit, and clear local/cloud
boundary copy.

## Security Principles

- Default deny all execution and data egress.
- Separate planning from action.
- Separate installed from enabled.
- Separate selected from active.
- Separate credential configured from provider executable.
- Require explicit user approval before filesystem mutation, subprocess execution, network access,
  cloud calls, microphone capture, playback, plugin execution, or provider boundary crossing.
- Record refusals and approvals in audit logs.
- Store secrets only in OS credential stores.
- Treat generated patches, tool output, and remote context as untrusted until reviewed.

## What Sentinel Should Not Become

- A Python backend wrapped by QML.
- An Electron IDE clone.
- A cloud-first provider router.
- A hidden autonomous agent runner.
- A tool marketplace where install implies execution.
- A memory system that silently writes inferred personal facts.
- A voice cloning product.
- A multi-agent framework that obscures user authority.

## Licensing Strategy

Before future implementation:

- Maintain a third-party notice inventory for every dependency and bundled model.
- Review code licenses and model/weight/voice licenses separately.
- Prefer MIT or Apache-2.0 dependencies for core runtime paths.
- Treat MPL-2.0 libraries as optional and avoid modifying vendored files unless obligations are
  explicitly managed.
- Avoid GPL/AGPL runtime dependencies in the desktop core unless a later legal review approves the
  distribution model.
- Use proprietary systems such as Cursor and Windsurf as product references only.
- Require license review for every voice model, embedding model, local LLM, MCP server, and plugin
  before distribution.

## Phase 41 Outcome

Phase 41 establishes Sentinel's ecosystem posture:

- Adopt local-first native runtime candidates where they match the architecture.
- Adapt open protocols, graph state, patch UX, approvals, and audit ideas.
- Keep heavyweight Python agent/RAG frameworks optional or inspiration-only.
- Avoid autonomous defaults, hidden memory mutation, unsafe voice cloning, and cloud/provider
  boundary crossing without explicit user authority.
