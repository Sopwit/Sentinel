# Sandbox

Sentinel applies sandbox policy at tool execution. `ISandboxPolicy`, `ProcessSandbox`, `ToolSandbox`, `ExternalDirectoryGate`, and secure file-mutation services are the relevant core boundaries. They constrain approved work; they do not make arbitrary native code safe.

Platform behavior is not equivalent. Linux uses Bubblewrap when available, with namespace, filesystem-bind, and optional network isolation. macOS uses `sandbox-exec` when available and generates a profile from approved paths; detached-child control is unavailable. Windows uses a restricted token and Job Object, but the implementation reports partial enforcement because filesystem confinement and network denial are unavailable. If a required backend is unavailable or enforcement fails, launch is denied rather than silently described as sandboxed. Package sandboxes such as Flatpak are deployment-specific. Plugin code runs through a separate plugin host, but remains a privileged extension boundary.

Users should treat tool approval as meaningful authority. Sandbox policy supplements, rather than replaces, careful workspace selection and review.
