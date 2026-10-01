# Adding a plugin

Use `plugins/sdk/include/SentinelPluginSdk.h` and the sample plugins in `plugins/samples/`. A plugin supplies a manifest and a Qt loadable module implementing `ISentinelPlugin`; add it to CMake only when it is a maintained in-tree sample or component.

Declare only necessary permissions. Tools registered from `initialize` must have JSON schemas and flow through the normal gateway. Plugin IDs and tool IDs are namespaced by the manager. Test manifest compatibility, dependency handling, permission denial, unload/reload, and active-call behavior.
