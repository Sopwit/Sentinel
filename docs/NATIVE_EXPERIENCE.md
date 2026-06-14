# Native Experience

Phase 49.6 upgrades Sentinel's native desktop experience while preserving local-first,
foreground-only behavior.

## Implemented Surfaces

- Home is chat-first with a large central chat area, collapsible conversation sidebar, fixed
  composer, conversation search/filter, pinned/recent/archived sections, New Chat, and message
  actions.
- Command Palette opens with `Ctrl+K` on Windows/Linux and `Cmd+K` on macOS.
- Settings opens as a floating modal from the gear, command palette, companion route, `Ctrl+,` on
  Windows/Linux, and `Cmd+,` on macOS.
- Advanced replaces user-facing Developer Mode. The internal `developerModeEnabled` setting
  remains for compatibility.
- Themes are Sentinel Dark, Midnight, Aurora, Graphite, and System Adaptive. Theme selection
  updates shared QML presentation tokens only.
- Brain contains Overview, Memory, Recall, Context, Continuity, and Advanced. Activity Timeline is
  local-only.
- First-run onboarding records use case and completion state. It starts no downloads.
- Recovery stores local draft text and offers to restore or discard it on next launch.
- Updates/About shows version/build/platform/Qt posture and a manual Check for Updates stub.
- Notification Center shows in-app category summaries and persists notification policy.
- Chat export supports Markdown, JSON, TXT, and PDF in the controlled app export directory.
- Backup/restore is represented as `.sentinelbackup` metadata foundation only.
- Phase 49.7 adds Local AI Ecosystem metadata in Settings > Models, compact Home model/role chips,
  Brain model-role summaries, Agents model-role planning readiness, and in-app notification
  categories for model/provider/download/benchmark/role events.
- Phase 50B adds workspace selection/status on Home, explicit browse/drag/drop/paste attachment
  controls, Settings workspace Local Knowledge Base opt-in, privacy/export summaries, and Brain
  workspace document/RAG summaries.

## Command Palette Actions

- Ask Sentinel.
- New Chat.
- Search Chats.
- Open Brain.
- Open Settings.
- Check Updates.
- Export Current Chat.
- Change Theme.
- Switch Model.
- Toggle Focus Mode.
- Universal Search.

## Safety Boundary

Phase 49.6 does not add Phase 50 execution authority. It does not add tool execution, autonomous
agents, hidden background work, filesystem scanning, cloud/provider calls, API-key submission,
model downloads, STT/TTS execution, update polling, telemetry, or silent indexing.

Phase 49.7 also keeps provider discovery, Model Advisor, Downloads Center, and Benchmark Hub
foreground metadata-only. It adds no hidden provider probes, catalog fetches, model pulls, update
workers, benchmarks, automatic routing, or native OS notifications.

Phase 50B keeps document interaction foreground-only. It adds no folder import, recursive
filesystem scan, background document processing, automatic embedding generation, cloud retrieval,
telemetry, autonomous agents, or hidden knowledge-base activation.

Check for Updates is explicit and network-stubbed. Global quick-panel shortcuts are not registered.
Backup restore controls are disabled metadata. Permanent delete remains refused; archive remains the
safe conversation lifecycle action.
