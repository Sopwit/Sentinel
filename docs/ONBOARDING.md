# Onboarding

Phase 51 makes onboarding a first-class local product flow.

## Steps

1. Welcome to Sentinel.
2. Privacy Philosophy: local-first, no telemetry by default, user control.
3. Choose Theme: Sentinel Dark, Midnight, Aurora, Graphite, or System Adaptive.
4. Configure AI: Ollama, LM Studio, llama.cpp server, or OpenAI-compatible local endpoint.
5. Workspace Introduction: Brain, Workspaces, Tasks, and Notifications.
6. Finish.

## Persistence

Completion state, selected onboarding AI provider, use case, and theme are persisted through
`AppSettings` in local settings JSON.

## Replay

Onboarding can be replayed from Settings. Replay clears completion state only; it does not reset
chat history, Brain entries, workspaces, providers, notifications, or exports.

## Boundary

Onboarding does not download models, enable cloud providers, scan files, start tasks, create
telemetry, or activate hidden indexing.
