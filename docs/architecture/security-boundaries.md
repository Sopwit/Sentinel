# Security & Authority Boundaries

Sentinel enforces explicit user authority and strict sandbox isolation for AI capabilities and system actions.

---

## Authority & Consent Model

1. **Explicit Foreground Action**: Tool executions, document indexing, and external requests require active user initiation.
2. **Gated Execution**: Tool executions run through the approval policy, sandbox policy, and tool execution gateway. The autonomous agent loop (`AgentLoop` + `LlmAgentRuntime`) evaluates these gates per step; medium/high-risk steps pause for explicit chat approval unless the user opts in to Autonomous Mode. Planning metadata alone never grants execution access.
3. **No Silent Background Actions**: Sentinel does not run background file indexing, hidden cloud uploads, or silent auto-updates.

---

## Workspace Isolation

- Document indexes, vector stores, memory items, and task histories are partitioned by unique `workspace_id`.
- Switching active workspace instantly changes active memory scopes without leaking data across projects.
