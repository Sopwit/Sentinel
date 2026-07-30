# Security Policy

Sentinel is built around the principle of **explicit user authority** and zero-trust defaults for automated actions.

## Security Guarantees & Posture

- **Zero Telemetry**: No tracking, telemetry, or analytics collection.
- **No Background Execution**: No silent background update checks, background polling, or unapproved tool executions.
- **No Hidden Cloud Calls**: No undisclosed API requests, cloud activation, or third-party tracking.
- **Explicit Operator Action**: All file reads, local RAG indexing, controlled tasks, and tool executions require explicit foreground user initiation and consent.
- **Metadata-Only Tool Boundaries**: Workspace permissions and task planning models operate strictly as isolated metadata representations and do not grant unapproved filesystem, terminal, clipboard, browser, or network execution.

## Phase & Release Security Boundaries

- Packaging metadata and build artifacts do not grant runtime authority.
- Signing, notarization credentials, and release keys must never be stored in the codebase repository.
- Diagnostics exports are strictly local and user-initiated, containing safe build metadata only.

## Reporting Security Vulnerabilities

If you discover a security vulnerability in Sentinel, please report it privately:

- **Do not** file public GitHub issues containing credentials, private transcripts, API keys, or sensitive filesystem paths.
- Contact the maintainers directly via private security notification or repository security disclosures.

## Pre-Release Verification Checklist

Before releasing any Sentinel binary distribution:
1. Run the release checklist in `docs/release/release-checklist.md`.
2. Confirm update checks remain manual-only and default to disabled.
3. Verify that no credentials, tokens, or private build paths are embedded in binary builds.
4. Confirm diagnostic exports write strictly to user-designated local export paths.
