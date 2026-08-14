# Plugins

Sentinel plugins are discovered and lifecycle-managed by the core `PluginManager`.

Plugins provide a manifest and a Qt loadable module implementing `ISentinelPlugin`. The manager
validates compatibility, resolves dependencies, enforces declared permissions, loads the module,
and manages initialization and shutdown.

Plugins remain behind the core plugin boundary before they are exposed to UI or automation
surfaces. Dynamic libraries are loaded only from the configured plugin storage directory and
remain subject to the plugin permission sandbox.
