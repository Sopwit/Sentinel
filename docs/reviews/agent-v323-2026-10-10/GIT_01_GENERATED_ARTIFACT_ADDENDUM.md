# GIT-01 — Generated artifact and evidence addendum

Date: 2026-10-12. Applies to the current Git index cleanup, baseline HEAD `c35a9aedff5832bd0704bbb164f4b25f9a7bff2d`. No commit or history rewrite is part of this change.

Exactly 2,767 reproducible generated files were removed from the current index under the [approved SEC-P1.1 pathspec](../../development/sec-p1-1/untracking-paths.txt): 2,758 probe Cargo target outputs and nine bytecode files. Local originals are preserved and ignored. Source, manifests, lockfiles, diagnostic records, screenshots and acceptance measurements remain unchanged. See the [GIT-01 report](../../development/GIT_01_CURRENT_CLEANUP_REPORT.md).

## Generated log identity and correction to the earlier reference finding

The omitted generated file is `docs/reviews/core-warden-rust-terminal/target/release/build/zmij-7c2145943554e178/stderr`.

- Size: **0 bytes**.
- Original SHA-256: `e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855`.
- Original Git blob: `e69de29bb2d1d6434b8b29ae775ad8c2e48c5391`.
- Original [artifact-sha256.json](artifact-sha256.json) SHA-256: `b9fc25c508f590d2c5d8314cdf1cde47fbb4dc4f768bf5f80f8e15e2da9aa4ec`.

The earlier SEC-P1.1 candidate cross-check associated this log with the review manifest by digest equality. Reinspection establishes that the manifest **does not name this generated log**. Fifteen manifest entries for other empty review files share the standard empty-file SHA-256. Digest equality does not establish a path reference or dependency. Accordingly, the earlier reports' characterization of an explicit generated-log reference is superseded by this correction; those historical reports and the original reference receipt are retained unchanged.

No manifest entry was deleted, rehashed or replaced. Untracking this generated log therefore does not invalidate any named entry in this manifest. Its original identity is documented here for transparency, not as a new historical acceptance measurement. The original manifest and log were hash-verified in the private recovery snapshot; the local log also remains byte-identical. Existing Git HEAD/history still retains the log until any separately authorized commit/history action.

## Reproduction and verification limits

Retained probe `Cargo.toml`, `Cargo.lock`, source and reproduction scripts support rebuilding with an external Cargo target directory. A new build can emit different output paths/log bytes; it cannot replace historical measurements or attest that an old executable was reproduced bit-for-bit. Scripts expecting `target/release` require an explicit release rebuild or configured executable location.

Historical measurements, acceptance results and original review-file digests are not changed by this addendum. Private originals/receipts must stay outside the public repository; any future evidence redaction or historical filtering needs its own approved transformation and retention ledger. This addendum does not authorize that work or clear binary distribution.
