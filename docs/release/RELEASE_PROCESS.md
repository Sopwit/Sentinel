# Release process

Releases are triggered by a Git tag matching `v*`. `-rc` and `-pre` tags are marked prerelease by the workflow. The workflow configures and builds `package-ready`, creates platform artifacts, generates SHA-256 checksum files, and creates a draft GitHub Release for maintainer review.

macOS signing/notarization runs only when the required Apple secrets are configured; otherwise the build is signed ad hoc. Windows signing runs only when the Windows certificate secrets are configured. Release credentials must remain GitHub secrets and never enter this repository.

Draft creation is not approval to publish. A maintainer must resolve licensing, Corresponding Source, artwork provenance, platform acceptance and signing policy before manually publishing. TUI V4-B remains conditional local evaluation only; the Desktop workflow does not build or clear the Rust package. No administrator settings are changed by repository files.

Each platform retains a unique `SHA256SUMS-<platform>.txt`. Collection verifies each checksum, refuses duplicate filenames and produces a combined `SHA256SUMS.txt`. Checksums detect corruption; they do not authenticate a compromised publisher. Verify signing/notarization separately.

Before tagging, run the relevant build/tests, `git diff --check`, and `packaging/release-qa.sh` where supported. Confirm release notes, artifact names, checksums, signing status, and installer smoke tests. This process does not make Flatpak, Snap, Homebrew, or Winget publication automatic.
