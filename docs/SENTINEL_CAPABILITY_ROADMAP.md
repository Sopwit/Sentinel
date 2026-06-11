# Sentinel Capability Roadmap

Phase 42.0 through Phase 42.12 converts the open-source ecosystem review and long-term Sentinel
strategy into a product roadmap. This document is planning-only. It does not authorize runtime
execution, tools/plugins, cloud/API calls, API-key input, filesystem scanning, microphone capture,
playback, STT/TTS activation, autonomous agents, background workers, dependencies, or product
behavior changes.

## Roadmap Principles

- Local-first by default.
- Explicit user permission before new authority.
- No hidden background execution.
- Installed does not mean enabled.
- Selected does not mean active.
- Configured does not mean executable.
- Workspace/path scopes beat broad system access.
- Audit approvals, refusals, and execution attempts.
- Keep C++ core policy authoritative and expose only QML-safe summaries.

## Core Features To Build

- Companion/Menu Bar/System Tray: user-visible shell integration for app status, quick capture,
  daily brief entry, pause/permission access, and foreground navigation.
- Workspaces: named local project scopes with allowed paths, visible context sources, session
  ownership, revocation, and no broad scanning.
- Skills/Profiles: manifests, provenance, capability declarations, profile defaults,
  installed-vs-enabled lifecycle, and disabled-by-default activation.
- Permission Modes: Disabled, Ask Every Time, Trusted, and Enabled states enforced in core policy.
- Agent Runtime Activation: foreground proposal-first sessions with plans, approvals,
  checkpoints, cancellation, and audit before any autonomous behavior.
- Tool Execution: gateway-mediated tool calls after descriptors, scope prompts, sandboxing,
  result summaries, and audit exist; start with narrow read-only pilots.
- Voice Activation: local-first STT/TTS pipeline candidates with disabled defaults, explicit
  microphone/playback permission, model and voice license review, and visible session state.
- Cloud Provider Activation: opt-in provider setup with OS credential storage, provider gates,
  privacy/billing warnings, local/cloud boundary prompts, and no hidden fallback.
- Filesystem/Workspace Access: explicit workspace/path grants, purpose labels, session/time
  limits, revocation, and mutation review.
- Packaging/Productization: platform packages, dependency and license notices, release QA,
  settings polish, crash-safe startup, and durable update strategy.

## Optional Features

- MCP-compatible import or bridge metadata after Sentinel-owned permission, sandbox, and audit
  policies are implemented.
- Semantic/vector retrieval as an optional explainable supplement with source attribution,
  deletion, fallback, and no hidden indexing.
- Graph memory for relationships and projects only after provenance, sensitivity, review, and
  revocation controls exist.
- llama.cpp or LM Studio integrations as optional local runtime/provider targets.
- Kokoro or other TTS engines as optional voice candidates after packaging, quality, Turkish
  coverage, and model-license review.
- Developer assistant workflows for patch proposals, approved edits, and approved verification
  commands after workspace and tool policies exist.

## Permission Defaults

| Capability | Default | Activation requirement |
| --- | --- | --- |
| Companion shell surface | Disabled/future-scoped | User-visible platform integration phase |
| Quick Capture | Disabled/future-scoped | Explicit foreground capture UI and target selection |
| Daily Brief | Disabled/future-scoped | Approved local sources and visible source/audit summaries |
| Workspaces | Disabled/future-scoped | User-created scope with approved paths |
| Skills/Profiles | Disabled/future-scoped | Manifest review, provenance, enable action, permission grants |
| Agent runtime | Metadata-only | Proposal, approval, session, safety, audit, and cancellation |
| Tool execution | Metadata-only | Descriptor, approval, sandbox, audit, and narrow pilot |
| Filesystem access | Disabled | Workspace/path grant with purpose and revocation |
| Voice activation | Disabled | Microphone/playback prompts, local runtime gates, license review |
| Cloud providers | Disabled | OS credential storage, explicit opt-in, privacy/billing warnings |
| Full automation | Disabled | Explicit bounded profile/scope and audited revocation |

## Future Phase Order

1. Companion/Menu Bar/System Tray roadmap and platform boundary planning.
2. Workspaces and scoped path model.
3. Skills/Profiles manifest and lifecycle metadata.
4. Permission Modes and policy centralization.
5. Audit log persistence for proposals, approvals, refusals, and attempted actions.
6. Agent Runtime Activation dry-run and ask-every-time foreground pilot.
7. Tool Execution gateway with one narrow read-only local pilot.
8. Voice Activation local-first pilot after STT/TTS selection and license review.
9. Cloud Provider Activation after real OS credential backends and provider gates.
10. Filesystem/Workspace mutation review after sandbox and rollback policies.
11. Packaging/Productization with dependency notices, platform packaging, and release QA.

## Safety Boundaries

- Do not add hidden daemons, polling loops, schedulers, or background workers as a side effect of
  roadmap phases.
- Do not let companion/tray/menu bar actions run providers, tools, agents, filesystem scans,
  microphone capture, playback, or cloud calls in the background.
- Do not allow cloud provider fallback from a local provider without explicit user approval.
- Do not treat API-key configuration as provider execution permission.
- Do not treat workspace selection as permission to scan or mutate all files.
- Do not let installed skills/plugins execute until explicitly enabled, scoped, sandboxed, and
  audited.
- Do not let model output invoke tools directly; all tool execution must pass through policy,
  approval, sandbox, and audit gates.
- Do not enable voice activation, STT, TTS, or playback without visible session state and explicit
  permission.
- Do not persist secrets outside OS credential stores.
- Do not expose raw secrets, provider payloads, filesystem handles, microphone handles, hidden
  prompts, or untrusted tool output directly to QML.
