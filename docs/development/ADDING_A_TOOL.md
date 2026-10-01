# Adding a tool

Add built-ins through the existing tool registration path, not by adding behavior to QML or heuristic shell planning. Define a `ToolDescriptor` with a stable ID, source, JSON input schema, permission domain, and evidence metadata; register it with `IToolRegistry`/the built-in provider.

Implement the handler behind existing system services: filesystem work uses `IFileSystemService`, and process work uses `ProcessExecutor`. `ToolExecutionGateway` must remain the invocation path so schema validation, approval, permission, sandbox, hooks, cancellation, and result handling stay authoritative.

Add focused tests for registration, schema rejection, permission denial, and handler success/failure. Plugin tools follow [Adding a plugin](ADDING_A_PLUGIN.md).
