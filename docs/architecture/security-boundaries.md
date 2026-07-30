# Security & Authority Boundaries

Sentinel enforces explicit user authority and strict sandbox isolation for AI capabilities and system actions.

---

## Authority & Consent Model

1. **Explicit Foreground Action**: Tool executions, document indexing, and external requests require active user initiation.
2. **Metadata-Only Execution**: Task plans, tool capabilities, and agent registries represent planning metadata only. They do not grant execution access to terminal, filesystem, network, or clipboard APIs.
3. **No Silent Background Actions**: Sentinel does not run background file indexing, hidden cloud uploads, or silent auto-updates.

---

## Workspace Isolation

- Document indexes, vector stores, memory items, and task histories are partitioned by unique `workspace_id`.
- Switching active workspace instantly changes active memory scopes without leaking data across projects.
