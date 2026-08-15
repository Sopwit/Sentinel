# Changelog

## Unreleased

- Added autonomous multi-step agent execution: `AgentLoop` runs a ReAct-style
  plan-execute-observe cycle with LLM-driven step planning (`LlmAgentRuntime`),
  context handoff between steps, self-correction after tool failures, an
  iteration limit (12 steps), doom-loop detection, and mid-run cancellation.
- Agent tool execution now routes through the `ToolExecutionGateway` with
  per-step approval and sandbox gates; risky steps pause the loop until the
  user approves (`onayla`/`iptal`) in chat, and Autonomous Mode bypasses
  per-step approval as before.
- Large tool observations are truncated (preview + full output on disk) before
  being fed back to the planner and the chat transcript.
- The agent loop runs on a background thread with step/final messages streamed
  into the chat transcript; the existing stop control cancels the active run.
- `LlmAgentRuntime` falls back to deterministic heuristic planning whenever
  the chat provider is unavailable or returns unparseable JSON.
- Existing single-shot agent pipeline behavior is preserved for callers that
  do not configure a step planner (all existing tests unchanged).

## 1.0.0-rc.7 - 2026-08-01

- Added full 8-language support (English, Turkish, German, Spanish, French, Chinese, Japanese, Arabic)
  with real translations and language selection in settings.
- Added cloud API provider routing for OpenAI, Claude, Gemini, DeepSeek, Groq, and Mistral with live
  dynamic model discovery and native C++ REST/SSE clients.
- Added in-app software update popup, native premium system sounds, and notification rate-limiting
  and coalescing.
- Completed premium UI polish with glass atmosphere, theme elevation tokens, redesigned settings
  tabs, models page, and tray companion window.
- Added Windows/macOS/Linux platform services (DefaultPlatformService), DPAPI-encrypted settings
  store, crash handler, protocol handler, and taskbar integration.
- Added plugin SDK with sandbox, manifest, permissions, dependency resolution, and sample plugins.
- Added daemon, CLI, packaging (AppImage, RPM, DEB, DMG, PKG, MSI, EXE), and release automation.
- Resolved all production-readiness audit findings including dead interfaces, stub implementations,
  QML-backend property mismatches, and missing UI controls.
- Preserved manual-only update behavior, local diagnostics export, no telemetry, no hidden cloud
  calls, and no autonomous behavior.

## 1.0.0-rc.1 - 2026-06-14

- Added release-candidate build metadata for app version, build number, git commit, build type,
  platform, and architecture.
- Added release, RelWithDebInfo, and package-ready CMake preset coverage.
- Added macOS Info.plist metadata, Windows version resource metadata, Linux desktop metadata, and
  Linux AppStream metadata.
- Added packaging, QA, CI, and Sentinel 1.0 RC checklist documentation.
- Added GitHub project readiness files and templates.
- Preserved manual-only update behavior, local diagnostics export, no telemetry, no hidden cloud
  calls, and no autonomous behavior.
