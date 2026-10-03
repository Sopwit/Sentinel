# Provider certification invocation contract

This opt-in live helper requires `SENTINEL_CERTIFICATION_WORKSPACE_ROOT` before any provider setup. Set it to the canonical absolute path of the exact existing readable fixture workspace you authorize. Positional provider/model/prompt/mode arguments are unchanged. There is no cwd, HOME, repository, or personal-path fallback. Missing, relative, nonexistent, file, symlink/aliased, filesystem-root and whole-home inputs fail closed with exit 2.

The validated canonical directory is passed unchanged as `workspaceContext.rootPath`. Existing tool restrictions, approval, sandbox and external-directory authorization remain authoritative. Setting this variable grants no additional external access. Use the same canonical fixture root to preserve the prior workspace boundary; this does not certify a different fixture's contents or a live provider's behavior. Run local root/gate regression tests before a separately authorized live certification.
