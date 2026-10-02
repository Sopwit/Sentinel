# Plugins

**Status: Experimental.** This directory contains the public Qt SDK and small
build-only sample plugins. It is not a directory for built-in Sentinel tools or
for installed production extensions.

Plugin lifecycle, manifests, the host protocol, permission brokerage, and tool
registration are owned by `core/plugin`; the separately launched host executable
is `apps/sentinel-plugin-host`. Native plugin modules never enter the Sentinel
process. The SDK intentionally exposes no Sentinel core headers and uses the
experimental native ABI/protocol declared in `plugins/sdk`.

Plugin tools register through the host, receive namespaced IDs, and execute via
`ToolExecutionGateway`. Input schemas, approvals, permission checks, sandbox
policy, and host-capability brokerage remain authoritative. Samples are staged
only for development and tests; they are not release payloads. See
[Plugins](../docs/capabilities/PLUGINS.md) and the
[security model](../docs/security/SECURITY_MODEL.md).
