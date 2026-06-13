# Local AI Ecosystem

Phase 49.7 adds Sentinel's local AI ecosystem foundation as metadata only.

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

## Safety

Only existing safe local Ollama metadata may populate installed model data. All other providers and
catalogs are disabled/readiness placeholders. This phase adds no background probing, hidden network
calls, cloud activation, catalog fetch, model download/update/delete, filesystem scan, subprocess,
benchmark execution, automatic routing, autonomous agents, tool execution, telemetry, or hidden
notifications.
