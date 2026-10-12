---
name: Release check
about: Track release-candidate validation
title: "Release check: "
labels: release
assignees: ""
---

## Platform

- [ ] Linux
- [ ] Windows
- [ ] macOS

## Required Checks

- [ ] `git diff --check`
- [ ] `cmake --preset tests`
- [ ] `cmake --build --preset tests`
- [ ] `ctest --preset tests --output-on-failure`
- [ ] `cmake --build --preset tests --target sentinel-desktop`
- [ ] Release build
- [ ] Smoke launch
- [ ] Packaging metadata
- [ ] Privacy/security checklist
- [ ] Accessibility checklist

- [ ] All applicable [publication gates](https://github.com/Sopwit/Sentinel/blob/main/docs/release/RELEASE_PROCESS.md#required-publication-gates) have exact-candidate evidence and separate maintainer approval.

CI success and this checklist are not automatic release clearance. Keep sensitive findings private.

## Notes

