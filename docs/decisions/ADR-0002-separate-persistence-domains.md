# ADR-0002: Separate local persistence domains

Status: Accepted

## Context

Settings, memory, chat history, conversations, agent runs, and permission grants have different lifecycles and sensitivity.

## Decision

Keep their stores separate; use Qt SQL for SQLite-backed domains and do not overload `IMemoryStore` with chat messages.

## Consequences

Retention, export, recovery, and access controls can remain domain-specific.
