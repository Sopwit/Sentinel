# Tools

**Status: Implemented.** Whether a particular tool can run depends on its descriptor, selected policy, authorization, approval, and sandbox result.

Tools give an agent controlled access to capabilities. `IToolRegistry` and `ToolDescriptor` are the contracts for built-in, MCP, plugin, and internal tools. A descriptor identifies its source, input JSON Schema, required permission domain, and evidence metadata.

`ToolExecutionGateway` normalizes and validates arguments before approval, hooks, sandbox policy, and the handler. Filesystem built-ins use `IFileSystemService`; process-backed work uses `ProcessExecutor`. A handler result becomes an observation for `AgentLoop`, subject to output truncation and grounding policy.

Tool availability is not authority. A tool can be discoverable yet rejected by approval, permission, resource authorization, or sandbox policy. See [Permissions](../security/PERMISSIONS.md), [Sandbox](../security/SANDBOX.md), and [adding a tool](../development/ADDING_A_TOOL.md).
