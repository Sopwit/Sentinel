# Threat model

| Asset / boundary | Threat | Current mitigation | Known limitation |
| --- | --- | --- | --- |
| User instructions and workspace content | Prompt injection causes unsafe tool requests | Tool schemas, approval, permission, authorization, and sandbox gates | A model can still produce misleading text or requests |
| Files and processes | Malicious arguments change local state | Gateway validation and filesystem/process boundaries | Approved actions still have real effects |
| Provider and MCP network boundary | Compromised or malicious remote response | Explicit configuration, credential boundary, local network policy | Remote services are outside Sentinel’s control |
| Plugin boundary | Plugin abuses native process access | Manifest validation, dependency checks, declared permissions, sandbox hooks | A native plugin is trusted code once loaded |
| Credentials | Disclosure through UI, settings, or logs | OS secret-store preference and presence-only UI data | Device/account compromise can expose OS-managed secrets |
| Local databases and logs | Other local users access persisted data | Standard platform locations and separate stores | At-rest encryption is not universally asserted |
| Sandbox boundary | Escape or platform mismatch | Policy and platform-specific implementations | No claim of uniform OS-level isolation |

This model is deliberately bounded to implemented controls. Review the root [security policy](../../SECURITY.md) for reporting a vulnerability.
