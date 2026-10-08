# Current audit evidence index

Captured 2026-10-08 from current working tree; inventory excludes subsequently created audit artifacts. No source was cleaned or committed. TSVs are lexical source inventories, not claims that every candidate was manually certified.

- files.tsv/inventory.json: full 1129-file inventory and mechanical classifications.
- markers.tsv: exact 1060 marker occurrences; heuristic categories, many SPDX/placeholderText/intentional unsupported rejections. Confirmed product defects are in report A01–A18; ambiguous candidates are not certified harmless.
- ui-actions.tsv:259 lexical handlers; complete production action-flow validation remains partial.
- settings.tsv:68 UI schema keys/references; references do not prove consumption or persistence.
- tools.tsv:35 descriptors, schema/gateway/source/test mapping; 32 enabled in doctor. Real per-tool Agent E2E incomplete.
- storage-schema.tsv, environment-flags.tsv, platform-paths.tsv: exact schema/config/platform occurrences.
- test-skips.tsv and test-skips-expanded.tsv:34 lexical skip/conditional sites with surrounding multiline reasons. Current run six skips described in report; remaining platform branches unexecuted.
- unprojected-ui-fields.tsv: omission candidates. Some non-eager fields synthesized separately; only demonstrated missing consumers treated as defects.
- orphan-candidates.tsv: Windows filename-based false positive resolved active through composition; no dead-code deletion authorized.

Runtime ledger: isolated `/private/tmp/sentinel-system-audit`, portable daemon profile SentinelSystemAudit20261008, custom Unix socket, Desktop preferences directory. Synthetic audit workspace/memory only. Agent runs/grants under the matching separate QStandardPaths profile. Existing installed qwen2.5:3b used; no download. Logs and JSON outputs retained in `build/tests/system-audit-evidence/` (local ignored evidence) with original `/private/tmp/sentinel-audit-*.log` paths as available.

Manual current-host observations: visible Desktop launched; seven onboarding steps completed; Tab moved focus in onboarding; Models incorrectly marked Qwen14B/32B Installed; Inspector displayed red Agent history unavailable error; Memory summary values blank; Desktop recovered after daemon restart. Full responsive, keyboard and native acceptance not completed. TUI80x24 connected, restored transcript, Ctrl-P command palette and Ctrl-O session picker worked, Escape closed them and idle Ctrl-C exited cleanly. No screenshot artifact fabricated.

Real requests: chat/multiturn/stdin, models/doctor/status/sessions; custom workspace create/select; bounded file scan; cancel/attach; restart/resume; remember action + read-only SQLite readback; settings LocalOnly/Ask Every Time/Dark restart persistence. Agent greeting and three read-planning attempts failed. Edit and allow/deny not reached. Voice unavailable/no ready STT. MCP/plugin not configured; fixture integration evidence only.

Final checks: git diff --check exit0. Audit-owned daemon shutdown accepted; temporary Desktop and Ollama server terminated. Evidence retained, no product source changes by audit.
