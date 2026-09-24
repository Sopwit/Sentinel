# Plugins

Sentinel plugins are discovered and lifecycle-managed by the core `PluginManager`.

Plugins provide a manifest and a Qt loadable module implementing `ISentinelPlugin`. The manager
validates compatibility, resolves dependencies, enforces declared permissions, loads the module,
and manages initialization and shutdown.

Plugins remain behind the core plugin boundary before they are exposed to UI or automation
surfaces. Dynamic libraries are loaded only from the configured plugin storage directory and
remain subject to the plugin permission sandbox.

## Executable tools

During `ISentinelPlugin::initialize`, call `context->registerTool(descriptor, handler)`.
Set `descriptor.id` to a local name such as `echo` and pass a `std::shared_ptr<IToolHandler>`.
The manager assigns `plugin.<escaped-plugin-id>.<escaped-local-id>` as the registry ID,
`plugin:<plugin-id>` as the provider, and the manifest version. It rejects duplicate IDs.
The plugin must declare `ToolExecution` in its manifest. The manager also checks that
permission on each invocation, so revocation takes effect immediately. Normal agent
approval, permission, sandbox, and hooks still run through `ToolExecutionGateway`.

Handlers may complete asynchronously. They should call completion once; the adapter
ignores later completions. Active calls retain the handler and module during unload.
Unload removes tools from discovery immediately, then retires the library after active
callbacks release it. Reload waits for active calls before loading the replacement.
`inputSchema` is enforced before execution. Tools may also declare
`ToolDescriptor.evidenceProduced` to describe the domain, freshness, scope, and resource
argument of observations they return. Undeclared plugin evidence defaults to
`ExternalService`; this metadata does not grant permissions.

## Tool input contracts

Executable plugin tools must provide `ToolDescriptor.inputSchema` as a JSON Schema object.
The registry rejects malformed or missing plugin contracts. The gateway validates every
model call against that schema before approval, hooks, or the plugin handler. Handlers
receive normalized arguments with schema defaults and retain responsibility for semantic
checks such as resource availability and authorization. Use `additionalProperties: false`
when a tool accepts only the declared fields.
