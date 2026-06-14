# Local AI Ecosystem

Phase 50A turns the local AI ecosystem foundation into guarded foreground local execution for
Ollama while keeping all privacy boundaries explicit.

## Surfaces

- Model Library: installed, discoverable, recommended, and detailed model summaries.
- Provider Discovery: Ollama, LM Studio, llama.cpp server, and OpenAI-compatible local endpoint
  readiness labels.
- Model Roles: Primary Chat, Coding, Summarizer, Research, Fast, Voice, and Embedding model
  assignments persisted as settings metadata.
- Model Advisor: deterministic local recommendations and avoid-list reasons.
- Downloads Center: download state/readiness placeholders.
- Benchmark Hub: manual/placeholder tokens/sec, latency, response time, RAM, and performance class
  metadata.

## Execution

- Ollama is executable through `http://127.0.0.1:11434` only.
- Installed Ollama models are listed from local `/api/tags`.
- Chat generation uses local `/api/generate`, supports streaming, timeout handling, cancellation,
  stale request rejection, and user-facing error states.
- OpenAI-compatible Local, LM Studio, and llama.cpp server are selectable local provider targets
  only. They require a later endpoint/model configuration phase before execution.
- Each send resolves to one provider/model from the active selection and role assignment context.
  No automatic fallback, parallel execution, or autonomous agent execution is performed.

## Diagnostics

Runtime diagnostics expose answered-by model, provider/route, request id, streaming state, total
duration, first-token latency, and approximate tokens/sec when available. Metrics stay local.

## Safety

Only foreground local provider calls are allowed. This phase adds no background probing, hidden
network calls, cloud activation, catalog fetch, model download/update/delete, filesystem scan,
subprocess, benchmark execution, automatic routing, autonomous agents, tool execution, telemetry,
or hidden notifications.
