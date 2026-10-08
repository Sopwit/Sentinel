# Agent conversation and tool validation — 2026-10-08

Status: PARTIAL. Conversational Agent completion verified with installed qwen2.5:3b; general tool-task completion remains unreliable with that model. No claim that every provider or every tool has live E2E acceptance.

## Changes

- Preserve registered descriptor lifetime during model-selected tool resolution.
- Keep planner protocol deltas private; only AgentLoop accepted finals become assistant content.
- Explicit conversational instructions across native and JSON planner paths. Conversation-only classification excludes tool contracts from textual prompt; live/action requests retain tool contracts and evidence gates.
- Contextual native finals do not falsely require verified external evidence.
- Accept a single JSON fence and exact registered-tool action spelling, while rejecting prose-wrapped objects, invented tools and heuristic shell planning.
- Validate model-selected arguments against authoritative descriptor schemas and return contract repair feedback before execution. Gateway validation, authorization, approvals and sandbox remain mandatory.
- Show recent actual tool results clearly so the model can continue to a final answer; repeated-call protections retained.
- Remove the provider availability/footer explanation under Home chatbar for all providers. Disabled-send state and model selection remain intact.

## Current validation

Configure/build tests preset completed. Latest production-source full CTest:114/114 PASS (97.69 seconds). Added planner repair regression subsequently rebuilt and passed focused planner test. Registry tests now check handlers for every registered descriptor; planner tests cover every exposed descriptor's identity/risk/mode with schema-valid inputs, native conversation, brief greeting, fenced decision and invalid-argument repair. Existing execution, permission, resource, filesystem, MCP and plugin fixture targets pass in the full suite. Generated contracts, branding and diff-check passed. Changed QML lint exits0 with existing context-injection warnings.

Real isolated daemon/installed qwen smoke: conversational Turkish greeting completed without tools. File-read handler succeeded during one run, but model repeated it and doom-loop detection terminated. Later runs included invalid arguments and rejected final grounding; the final repaired planner still failed a real safe-read request. **Agent tool workflows therefore remain a current defect, not fixed or deferred.** No safe edit/allow/deny E2E certification. No new models or dependencies installed. Raw protocol deltas absent from new Agent event streams.

UI acceptance limit: active user Sentinel instance blocked launching the newly built isolated Desktop; CUA app selection also timed out. Existing user process was not stopped. Full responsive/keyboard/loading/error acceptance of changed production UI remains unverified. Use the newly built `build/tests/apps/sentinel-desktop/sentinel-desktop.app` and matching rebuilt daemon after restart; currently running processes do not hot-reload these fixes.

## Complete built-in tool review

35 descriptors,32 enabled registrations (local-plan-summary and todo-write/read disabled). Every row below passed source schema/registration review and applicable registry/planner regression coverage. Handler/runtime fixtures are distinct from live model E2E. External and destructive tools were not invoked indiscriminately.

| Tool | Schema/registration | Execution evidence | Live acceptance |
| --- | --- | --- | --- |
| local-plan-summary | Reviewed; regression PASS | Disabled metadata/internal descriptor; not a model-exposed production workflow | Not live E2E validated |
| list-directory | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| read-file | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Handler succeeded in real run; accepted final workflow failed |
| write-file | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| edit-file | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| grep | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| glob | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| delete-file | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| move-file | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| apply-patch | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| list-code-definitions | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| run-command | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| app-launch | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| app-quit | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| open-url | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| system-notify | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| clipboard-read | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| clipboard-write | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| system-info | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| process-list | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| current-time | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| set-alarm | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| list-alarms | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| cancel-alarm | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| todo-write | Reviewed; regression PASS | Disabled metadata/internal descriptor; not a model-exposed production workflow | Not live E2E validated |
| todo-read | Reviewed; regression PASS | Disabled metadata/internal descriptor; not a model-exposed production workflow | Not live E2E validated |
| memory-search | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| history-search | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| spawn-agent | Reviewed; regression PASS | Agent scheduler/subagent fixtures; real provider delegation not exercised | Not live E2E validated |
| browser-screenshot | Reviewed; regression PASS | Browser helper/error fixtures; no live browser export smoke | Not live E2E validated |
| browser-pdf | Reviewed; regression PASS | Browser helper/error fixtures; no live browser export smoke | Not live E2E validated |
| web-fetch | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| web-search | Reviewed; regression PASS | Existing gateway/real-executor fixtures and source handler trace | Not live E2E validated |
| voice-transcribe | Reviewed; regression PASS | Speech fixtures; physical microphone/configured STT/TTS unavailable | Not live E2E validated |
| voice-speak | Reviewed; regression PASS | Speech fixtures; physical microphone/configured STT/TTS unavailable | Not live E2E validated |

Dynamic MCP/plugin tools: discovery and registered schemas/gateway execution covered by current stdio/plugin fixtures. No configured production external server/plugin smoke; not counted in the35 built-ins.

Evidence retained locally under `build/tests/system-audit-evidence/agent-fix/`; contains synthetic-profile logs/JSON only. Earlier system-wide audit remains the dated discovery baseline. Remaining model/tool compatibility needs further closure; this report does not overwrite its issues as PASS.
