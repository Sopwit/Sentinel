# Security policy

## Reporting a vulnerability

Please report suspected security vulnerabilities privately through the [GitHub private vulnerability reporting](https://github.com/Sopwit/Sentinel/security/advisories/new) (requires the maintainer to enable it). Do not open a public issue containing credentials, private transcripts, sensitive paths, exploit details, or other confidential material.

Include a clear description, affected version or commit, reproduction steps, impact, and any suggested mitigation. Give maintainers a reasonable opportunity to investigate before public disclosure.

## Scope and release hygiene

Do not commit API keys, tokens, signing certificates, passwords, private update credentials, or generated release artifacts. Before release work, follow the [release process](docs/release/RELEASE_PROCESS.md).

Product security architecture is documented in the [security model](docs/security/SECURITY_MODEL.md), [permissions](docs/security/PERMISSIONS.md), [sandbox](docs/security/SANDBOX.md), and [threat model](docs/security/THREAT_MODEL.md).

## Support and exposure response

This project is pre-release. No version has a promised security-support window or a public-distribution clearance. Report issues against the current commit and include the affected platform.

If private reporting is unavailable, ask the maintainer for a private channel without posting sensitive details publicly. For an exposed credential, revoke/rotate it with its issuer immediately: removing it from the latest tree does not remove Git-history exposure. History rewriting, remote deletion, and rotation require explicit owner approval.

Local databases, diagnostics, screenshots, prompts and responses can contain private data. Review them before attaching to issues or committing review evidence. Use synthetic fixtures and repository-relative paths. Never upload unredacted scanner output. The [SEC-P1 audit](docs/development/SEC_P1_PUBLIC_REPOSITORY_AUDIT.md) records current limits and manual administrator actions.
