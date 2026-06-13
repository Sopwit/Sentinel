# Sentinel Product Realignment

Phase 49.5 realigns Sentinel around a premium native AI desktop companion while keeping runtime
authority unchanged.

This phase is UI, documentation, and metadata foundation only. It does not implement Phase 50 tool
approval, real tool execution, autonomous agents, filesystem scanning, model downloads, cloud/API
calls, API-key entry, STT/TTS execution, microphone or playback access, subprocess execution,
background workers, hidden indexing, telemetry, or update network checks.

## Product Information Architecture

Primary app surfaces are:

- Home: large chat-first workspace.
- Brain: memory, recall, context, summaries, continuity, and explainability.
- Agents: metadata-only agent catalog and dry-run planning visibility.

Settings is a floating modal/popup opened from gear, command palette, or companion entry points.
It is no longer treated as a primary dock page. Internal `Settings` navigation remains accepted as
a compatibility route so existing tray/menu code can open the modal.

User-facing diagnostic copy should say Advanced or Diagnostics. Internal
`developerModeEnabled` remains a compatibility flag for the read-only diagnostics gate.

## Home

Home is the primary product surface. The main chat area should take the largest share of the
window. Conversation browsing belongs in a collapsible sidebar with search/filter, pinned, recent,
and archived group structure. Provider, model, context, permission, profile, and agent indicators
must stay compact. The composer remains anchored and prominent.

Duplicate right-side AI Bridge or diagnostics panels should not reduce chat space in normal Home.
Diagnostics belong in Brain > Advanced, Agents > Advanced, or Settings > Advanced.

## Brain

Brain replaces product-facing Runtime/Memory language.

Brain sections:

- Overview.
- Memory.
- Recall.
- Context.
- Advanced.

Normal Brain UI should be concise: memory state, recall state, context availability, summaries,
continuity, and explainability. Long traces, budgets, provider internals, semantic/vector details,
and execution-boundary diagnostics belong in Advanced.

## Settings Modal

Settings categories:

- General.
- Appearance.
- AI.
- Models.
- Voice.
- Brain.
- Permissions.
- Tools.
- Agents.
- Workspace.
- Notifications.
- Updates.
- Advanced.

Normal settings should remain simple. Long explanations should be removed or moved to Advanced.
The modal preserves the app context behind it.

## Companion And Quick Panel

The companion should evolve from a tray menu into a mini AI surface:

- Menu bar/system tray quick chat panel.
- Quick input.
- Recent chats.
- Quick note placeholder.
- New chat.
- Open Sentinel.
- Check updates.
- Pause companion.
- Settings.

Global shortcut roadmap:

- macOS Option+Space opens the quick panel.
- Cmd+Shift+Space opens the main window.
- Esc closes the panel.
- Shortcuts become configurable later.

This phase does not implement global shortcut execution or background AI work.

## Theme Foundation

Theme choices are presentation metadata:

- Sentinel Dark.
- Midnight.
- Aurora.
- Graphite.
- System Adaptive.

Theme selection affects UI only. It must not change providers, permissions, models, tools, routing,
or runtime behavior.

## Model Library

Model Library is a professional metadata foundation for:

- Installed.
- Available.
- Recommended.
- Role Assignments.

Planned model sources include Ollama, llama.cpp server, LM Studio, OpenAI-compatible local
endpoints, Hugging Face, and future custom catalogs.

Model metadata should include provider/source, size, RAM estimate, context length, quantization,
capabilities, Turkish support, coding support, reasoning support, vision/audio support, and
performance estimate.

Phase 49.7 implements this as a metadata-only foundation in Settings > Models. Installed data may
come from existing safe local Ollama metadata; LM Studio, llama.cpp server, OpenAI-compatible local
endpoints, Hugging Face, and custom catalogs remain disabled/readiness placeholders. Sentinel does
not download, delete, update, scan, benchmark, probe, or fetch model catalogs in this foundation.

## Multi-Model Roles

Future role assignments:

- Primary Chat Model.
- Coding Model.
- Summarizer Model.
- Research Model.
- Fast Model.
- Voice Model.
- Embedding Model.

Role routing remains metadata/readiness only. No parallel agentic execution or automatic routing is
authorized by this phase. Future route decisions must remain explainable.

Phase 49.7 persists role-to-model ids in settings metadata only. Assigning a model role does not
change the send path or enable multi-model execution.

## Model Advisor

Sentinel should later include a local deterministic model recommendation assistant. Inputs include
device profile, RAM, CPU, GPU, platform, user goal, preferred language, and speed/quality
preference. Outputs include recommended local models, expected RAM/disk needs, and provider/source
suggestions.

The first version should use static metadata. No automatic downloads, hidden profiling, or hidden
network access are allowed. External tools such as llmfit require license review before use.

Phase 49.7 implements the first deterministic/static version and treats llmfit-style tooling as
future license-reviewed inspiration only.

## Notifications

Notification categories:

- Chat complete.
- Model ready.
- Model download complete.
- Update available.
- Export complete.
- Voice ready.
- Permission request.
- Agent plan ready.
- Tool approval required.
- Workspace blocked.
- Error/failure.
- Daily brief.
- Focus session complete.

Policies:

- Disabled.
- Important Only.
- All.
- Custom.

Platform targets are macOS Notification Center, Windows Toast, Linux desktop notifications, and a
future in-app notification center. This phase adds no background polling or hidden notifications.

## Updates And About

Update UX should expose Check for Updates, current version, build number, Qt version, and platform.

Policies:

- Never.
- Ask before checking.
- Weekly.
- On Startup.

Default is privacy-safe: no hidden update checks, no auto-download, no auto-install, no telemetry,
and no network call in this phase.

Future packaging notes: macOS Sparkle-like option, Windows installer/MSIX/WinSparkle-like option,
and Linux Flatpak/AppImage/RPM strategy.

## Chat Productivity

Roadmap actions:

- Copy.
- Edit.
- Regenerate.
- Pin.
- Delete.
- Export.

Roadmap attachments: PDF, image, code, and text. Roadmap exports: Markdown, JSON, TXT, PDF, and
Share Package.

## Focus And Session Modes

Focus and Session modes are future presentation and policy metadata:

- Coding.
- Writing.
- Research.
- Study.

They must not add hidden automation.

## Privacy Principle

Sentinel defaults:

- No telemetry by default.
- No hidden tracking.
- No silent cloud calls.
- No background uploads.
- No automatic indexing.
- User opt-in is required for anything external.

## Open-Source Ecosystem Note

Odysseus is added to the ecosystem review as architectural inspiration only because AGPL-3.0 code
must not be copied or imported without separate approval. Useful concepts are self-hosted
workspace posture, tool categories, agent planning, and workflow/recipe ideas.
