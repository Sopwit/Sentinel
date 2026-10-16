# Tools

**Status: Implemented.** Whether a particular tool can run depends on its descriptor, selected policy, authorization, approval, and sandbox result.

Tools give an agent controlled access to capabilities. `IToolRegistry` and `ToolDescriptor` are the contracts for built-in, MCP, plugin, and internal tools. A descriptor identifies its source, input JSON Schema, required permission domain, and evidence metadata.

`ToolExecutionGateway` normalizes and validates arguments before approval, hooks, sandbox policy, and the handler. Filesystem built-ins use `IFileSystemService`; process-backed work uses `ProcessExecutor`. A handler result becomes an observation for `AgentLoop`, subject to output truncation and grounding policy.

There are 38 built-in definitions and handler mappings, not 38 guaranteed available tools. `local-plan-summary` is disabled/internal. Registry enablement, platform/handler availability and runtime exposure constrain the usable inventory; dynamic plugin/MCP tools vary with configuration. Production headless daemon clipboard tools are unavailable, and Linux application launching is explicitly unsupported. Plugin and MCP integrations remain experimental; schema declarations and registration do not certify execution or native-code isolation.

Tool availability is not authority. A tool can be discoverable yet rejected by approval, permission, resource authorization, or sandbox policy. See [Permissions](../security/PERMISSIONS.md), [Sandbox](../security/SANDBOX.md), and [adding a tool](../development/ADDING_A_TOOL.md).
