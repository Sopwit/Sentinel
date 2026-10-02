# ADR-0003: Provider and model routing boundaries

Status: Accepted

## Context

Sentinel supports local and credential-gated cloud inference without binding the application to one backend.

## Decision

Keep provider behavior behind `IChatProvider` and select active runs through `IModelRouter` and `ModelBinding`.

## Consequences

Interfaces share provider semantics, and models remain distinct from provider implementations.
