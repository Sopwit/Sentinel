# Plugins

**Status: Experimental.** Sentinel has a tested SDK, samples, and runtime lifecycle integration, but not a stable plugin ecosystem or marketplace.

`PluginManager` discovers manifests, checks compatibility and dependencies, enforces declared permissions, and manages plugin-host lifecycle, enable/disable, unloading, and hot reload. Native plugin code is hosted outside the Sentinel process through the plugin-host protocol; it is not exposed as an in-process core object.

Plugins can register tools during initialization through the plugin context. Plugin tool IDs are namespaced, require a manifest `ToolExecution` declaration, and still execute through `ToolExecutionGateway`. The SDK and working samples live in [`plugins/`](../../plugins/README.md).

Plugins are a trust boundary, not a way to bypass approval, permissions, schemas, or sandbox policy. Reload does not permit widened credential declarations. Treat the SDK/API and compatibility rules as subject to change.
