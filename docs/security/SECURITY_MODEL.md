# Security model

Sentinel is local-first, but it is not an isolation boundary by itself. Its security model separates provider access, tool execution, plugins, persistence, and platform services behind explicit interfaces and policy checks.

Agent tool calls pass through `ToolExecutionGateway`, argument validation, approval policy, permission policy, authorization checks, and sandbox policy before a handler executes. Cloud providers and remote MCP servers are network boundaries. Plugins are dynamically loaded extension code and therefore a separate trust boundary.

Credentials use an OS-secret-store preference and must not be surfaced through the Desktop settings boundary. Settings, memory, chat history, conversations, and logs have distinct local storage owners. Sentinel documents no telemetry collection, but local logs and persisted records still require ordinary device security.

Read [Permissions](PERMISSIONS.md), [Sandbox](SANDBOX.md), [Secrets](SECRETS.md), and [Threat model](THREAT_MODEL.md). Vulnerability reporting is in the root [security policy](../../SECURITY.md).
