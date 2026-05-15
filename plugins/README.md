# Plugins

Placeholder for the future Sentinel plugin surface.

The first alpha defines `IPlugin` only. Runtime plugin loading is intentionally not implemented yet.

Future plugins should be lifecycle-managed through the core plugin boundary before they are exposed to UI or automation surfaces. The alpha does not load dynamic libraries or run external plugin code.
