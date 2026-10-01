# Model Context Protocol (MCP)

**Status: Experimental.** Local and remote server integration is implemented, but the transport and configuration surface should not be treated as a stable compatibility promise.

Sentinel includes `IMcpService`, `McpService`, OAuth support, an MCP tool catalog, and an MCP tool provider. Desktop settings supply configured local-command or remote-URL servers. The service can connect, discover tools and resources, invoke tools (including asynchronous calls), and register discovered tools through the shared descriptor and execution-gateway path.

Remote MCP configuration may use credentials from the credential store. A configured server is external and untrusted; its tools remain subject to permission and approval policy. Prompt handling, universal server compatibility, a marketplace, and a stable public MCP configuration contract are not documented as supported.
