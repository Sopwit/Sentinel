# Release process

Releases are triggered by a Git tag matching `v*`. `-rc` and `-pre` tags are marked prerelease by the workflow. The workflow configures and builds `package-ready`, creates platform artifacts, generates SHA-256 checksum files, and publishes the collected artifacts to GitHub Releases.

macOS signing/notarization runs only when the required Apple secrets are configured; otherwise the build is signed ad hoc. Windows signing runs only when the Windows certificate secrets are configured. Release credentials must remain GitHub secrets and never enter this repository.

Before tagging, run the relevant build/tests, `git diff --check`, and `packaging/release-qa.sh` where supported. Confirm release notes, artifact names, checksums, signing status, and installer smoke tests. This process does not make Flatpak, Snap, Homebrew, or Winget publication automatic.
