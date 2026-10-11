# V4-B local evaluation evidence

Build/package procedure (repository root):

```sh
cargo build --manifest-path cli/Cargo.toml --workspace --release --locked
python3 tools/terminal/package-evaluation.py NEW_OUTPUT_DIRECTORY
```

The packaging helper refuses existing output and never builds/installs/signs/publishes.
The retained local artifact is under ignored `artifacts/`; `package-summary.json` and
`artifact-SHA256SUMS` identify it. Sorted inventory does not establish independent
bit-for-bit build reproducibility.

The PTY wrappers point to this phase's packaged executable. Compatibility (29),
interaction (14), mascot (15), real signals (18), extracted-package relocation (7)
and production performance (4 samples) retain results/logs here. Unix socket fixtures
need permission in restricted environments. Simulated terminal responses are not
physical terminal acceptance. No real inference runs are performed.

Licensing is OPEN: read the artifact's THIRD_PARTY_NOTICES.md,
DISTRIBUTION_GATES.md and dependency-licenses.json. Six exact-version original
license texts are absent locally; Corresponding Source/toolchain/resource provenance
and publication obligations are not cleared. Artifact contents are for evaluation,
not a claim of public redistribution permission.
