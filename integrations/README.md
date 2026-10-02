# Integrations

`integrations/` is reserved for Sentinel-owned concrete adapters to external
systems: provider transports, OS services, secure storage, or protocol clients.
An integration implements a core contract; it does not own agent policy, QML,
plugin lifecycle, or package installation.

There are currently no production integration targets in this directory. The
existing provider, MCP, and platform implementations remain in `core/` as part
of the modular monolith and should only move here alongside a deliberate target
and composition-boundary change. See the [architecture](../docs/development/ARCHITECTURE.md).
