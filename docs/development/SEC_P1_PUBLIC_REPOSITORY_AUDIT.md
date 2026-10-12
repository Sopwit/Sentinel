# SEC-P1 — Public repository security, privacy and open-source audit

Audit date: **2026-10-12**. Baseline after concurrent Desktop D2.1 commits: **541b8db9**. Recommendation: **CONDITIONAL for continued public source development; NOT CLEARED for public binary distribution or a claim of completed privacy clearance.** No genuine exposed credential was confirmed. Verified privacy/cleanliness issues, dependency advisories and unresolved distribution obligations remain.

## 1. Scope, inventory and ownership

Personal-Brain's `brain` executable was unavailable (`command -v brain` returned no executable). No project brief or bounded Brain context could be retrieved. The user-supplied workspace and its `AGENTS.md`, architecture, Agent Runtime, security model, building and testing documents were used; no alternative workspace was guessed.

The initial tree had ten modified Desktop/UI/test files and concurrent D2.1 additions. These were not edited. The owning concurrent session committed them as `ea64f1b9`, `4ea26ca2` and `541b8db9` before audit mutations began. At that point only an existing untracked `tools/terminal/__pycache__/` remained; its file hash was recorded privately and verified unchanged. Runtime, UI, test sources, Cargo manifests/lockfile and canonical art have no audit diff. This audit made no Git commit, push, merge, tag, history rewrite or remote-setting change.

Inventory at baseline:

- **7,862 tracked files**, including C++/QML/Rust, seven workflows, packaging/build tools, documentation, review transcripts/diagnostics, canonical images and editable Blender sources.
- **487 commits** reachable from local refs. The repository is not shallow. Local branches, tags, cached remote-tracking refs and local checkpoint refs were included in object scanning. No fetch was performed; this is not a complete inventory of GitHub PR refs, unreachable server objects, forks or previous downloads.
- **11,507 reachable blobs**, **1,263,372,634 bytes**, inspected by the supplementary byte-pattern scan. Gitleaks scanned 486 patch-bearing commits and approximately 82.74 MB of patch content; object scanning additionally covered unchanged binary/build blobs.
- **2,758 tracked files / 703,223,935 bytes** under `docs/reviews/core-warden-rust-terminal/target/`; **nine tracked `.pyc` files**. The target tree is generated output from a review probe with its own Cargo manifest/lockfile, not production source. It was retained to preserve review ownership and evidence; ignore rules alone do not untrack it.
- No currently tracked `.env`, private-key/certificate, `.db` or `.sqlite` file was identified by the filename inventory. This does not establish absence of encoded or arbitrarily named data.

All tracked file contents were covered by automated inventory/pattern checks. Architecture-sensitive boundaries and build/release/dependency files were reviewed directly. This is not a claim of line-by-line human verification of every source, image, generated object or transcript.

## 2. Confirmed findings by severity

Severity is this audit's prioritization, not a CVSS score. Package advisory presence is distinguished from demonstrated application exploitability.

| ID | Severity | Finding and redacted evidence | Disposition |
| --- | --- | --- | --- |
| SEC-01 | High — distribution gate | Four Objective-C-family crates lack complete verified original copyright/permission notices; exact-revision upstream policy also records uncertainty about Apple SDK derivatives. GPL Corresponding Source, toolchain, Qt bundle and artwork provenance obligations are not fully established. | Two other missing MIT texts resolved; four and broader distribution obligations remain open. |
| SEC-02 | High — potential memory safety | Locked `lru 0.12.5` is in affected ranges for RUSTSEC-2026-0002 and RUSTSEC-2026-0253 through `ratatui 0.29.0`. No Sentinel exploit was demonstrated. | Compatibility/reachability remediation deferred; no runtime or large dependency migration in this phase. |
| SEC-03 | Medium — privacy | Supplementary scan found home-directory paths in **1,803 tracked baseline files**, 45,082 occurrences, including compiled probe artifacts, reports and live review records. Separate checks found machine-temporary paths in 114 tracked files and external-volume paths in two. Example evidence: `PRIVATE_REFERENCE_a279cf0cd5754ba3be12eeeb68246c88` contains a developer root under `/Users/<redacted>/…`. | Public build/TUI examples corrected; historical evidence and binary paths still require owner retention/redaction decisions. |
| SEC-04 | Medium — privacy | Review JSON/ANSI and screenshots contain recorded prompts, model output, session/run IDs and machine diagnostics. `docs/reviews/agent-v322-2026-10-10/B-edit-approval-0.ansi.png` visibly contains an OS temporary path, even though the task is synthetic. | Samples reviewed; no assertion that every image/transcript is privacy-cleared. Preserve originals pending deliberate sanitization and evidence-hash updates. |
| SEC-05 | Medium | Nested Cargo target output and bytecode are tracked; root-only Rust ignores missed the probe tree. | Nested targets/bytecode now ignored for new files; existing tracked artifacts retained and enumerated separately. |
| SEC-06 | Medium | Release collection flattened three `SHA256SUMS.txt` files, overwriting two. Linux output discovery could select a downloaded deployment tool rather than the generated application AppImage. | Unique manifests, verification, duplicate rejection and combined checksum file added; tools moved outside the output discovery directory. |
| SEC-07 | Medium | Two downloaded Linux deployment executables had named versions but no enforced content digest before execution. | Fixed SHA-256 checks added before execution. No upstream signature/provenance guarantee is inferred from these hashes. |
| SEC-08 | Medium | `actionlint` rejected a step `if` using the `secrets` context. Windows signing occurred after installer creation and signed a stage copy that CPack can reinstall from unsigned build inputs. Native command failures could be masked by later successful commands. | Secret presence handled inside the signing script; build/staged application payloads signed before CPack; installer signing checks exit codes; Windows packaging commands fail explicitly; temporary certificates cleaned in `finally`. Native signing remains unverified. |
| SEC-09 | Medium | Tag workflow automatically published artifacts despite unresolved release gates. | Workflow now creates a **draft**. Maintainer publication and protected release/tag policy remain manual. No workflow was dispatched. |
| SEC-10 | Medium — compatibility | Existing V4-B binary with SHA-256 `45087e81…` rejects current extended CW03B manifest with `Unexpected CW-03A manifest`. A new archive can pass byte-integrity checks while resource initialization fails. | Evaluation helper refuses this exact known incompatible binary/catalog combination before creating output. Protected old archive/binary unchanged. |
| SEC-11 | Low — maintenance | `paste 1.0.15` is unmaintained (RUSTSEC-2024-0436), pulled by Ratatui. Rust dependency update coverage was absent. | Cargo Dependabot coverage added; migration remains open. |
| SEC-12 | Low | Rust CI/labeler lacked timeouts; checkout credentials were left persistent; CodeQL lacked explicit workflow default permissions. Public security form encouraged vulnerability details despite private-reporting guidance. | Timeouts, credential persistence disabled, explicit defaults and documentation-only public security form added. |

No critical credential finding was established. No credential was tested against a live provider, revoked or rotated.

## 3. Security and privacy fixes applied

- `.gitignore`: nested Cargo targets, Python bytecode, local runtime directories, SQLite sidecars, root local databases, compiler caches, crash dumps, environment overrides, model files, signing containers and release staging. Existing canonical assets and reproducible inputs remain visible. The blanket root `*.txt` rule was removed because it could hide legitimate notices/source documentation.
- `.gitattributes`: LF for Rust/TOML/Python/lockfiles; binary handling for Blender masters, additional image/font formats and archives. No asset bytes changed.
- CI/release hardening detailed below. Existing full-SHA action pins preserved.
- `SECURITY.md`, the public security-question form and contribution guidance now separate private vulnerability reporting from public issue content. Actual private-reporting enablement is an administrator action.
- Public README/install/release guidance now distinguishes implemented services, incomplete AgentRuntime acceptance, experimental Plugin/MCP/Skills features, unavailable native Windows terminal IPC and conditional TUI evaluation.
- `BUILDING.md` uses ordinary executable/temp-path examples; the TUI V4-B report uses repository-relative paths and a caller-supplied repository root. Historical measured values and protected package evidence were retained, with a SEC-P1 addendum.
- Original upstream license supplements and provenance added. Known GPL, Tabler and font notices now install with Desktop on all configured platforms, inside the macOS bundle for drag-copy installations.

These fixes do not sanitize every historical report or already tracked generated object. They do not change AgentRuntime, provider routing, daemon behavior, permissions, storage, IPC contracts or presentation logic.

## 4. Git history and secret exposure assessment

Gitleaks **8.30.1** was downloaded into temporary storage only, verified against the release's checksum manifest, and run with `--redact`. No system-wide scanner was installed. Tool checksum verification is integrity checking against the same upstream release channel, not independent signature verification.

Commands used:

```sh
gitleaks git --redact --no-banner --log-opts=--all --report-format json --report-path <private-temp-report>
gitleaks dir <tracked-and-new-text-snapshot> --redact --no-banner --report-format json --report-path <private-temp-report>
```

The text snapshot excluded NUL-containing/big files; the separate all-blob pattern scan covered binary bytes too. No scanner report containing match values is included in this repository.

Both Gitleaks scans returned **five generic-api-key candidates**, reviewed as false positives:

| Locations | Redacted classification |
| --- | --- |
| `docs/reviews/agent-v32-2026-10-10/sentinel_v32_accept.py:4`, corresponding V321/V322 scripts, and `agent-v323-2026-10-10/reproduce/sentinel_v323_accept.py:4` | Literal README content explicitly describes a synthetic acceptance project and a synthetic project token. Four versions of fixture-generation code; no genuine authentication credential established. Values intentionally omitted. |
| `docs/reviews/agent-v323-2026-10-10/artifact-sha256.json:199` | Artifact path followed by a 64-hex SHA-256 digest; not an authentication token. |

The supplementary scan covered private-key headers, common GitHub/provider token forms, credential-bearing URLs, quoted credential assignments and personal paths. Its eleven historical assignment candidates were GitHub `${{ secrets… }}` references, not embedded secret values. A high-signal commit-message scan returned zero candidates. The only matched RFC1918 URL location was a runtime unit test, not a verified private deployment endpoint.

**Assessment:** no confirmed exposed credential, but substantial machine/path disclosure exists in the current tree and reachable history. Removing a path from a current document does not erase earlier commits. Cached refs and local checkpoint refs can contain data not necessarily published remotely; this audit did not establish remote exposure for each blob. Do not describe the history as clean or vulnerability-free. No history rewrite is authorized or performed.

Encrypted/encoded secrets, arbitrary binary data, all image pixels and deleted remote content are not exhaustively cleared by these checks. Two terminal screenshots were visually sampled; one visibly exposed a machine-temporary path. Whole-corpus visual/privacy clearance remains **NOT VERIFIED**.

If a real credential is later confirmed, stop publication, privately identify its file/location and history scope, and revoke/rotate with the issuer immediately. Destructive history remediation and rotation require explicit owner approval.

## 5. CI/workflow audit

| Workflow | Trigger / token boundary | Audit outcome |
| --- | --- | --- |
| `ci-linux.yml` | Push and `pull_request`; `contents: read`; three bounded jobs | Checkout credential persistence disabled. PR code does not receive configured signing/provider secrets. Existing quality, build and ASan jobs preserved. |
| `ci-macos.yml` | Push and `pull_request`; `contents: read`; 40-minute job | Credential persistence disabled; otherwise preserved. |
| `ci-windows.yml` | Push, PR and manual; read-only; bounded jobs | Credential persistence disabled. ARM64 remains experimental/continue-on-error; no terminal IPC support inferred. |
| `ci-rust.yml` | Push and PR; read-only | 25-minute timeout added; pinned Rust 1.94 checks preserved. Credential persistence disabled. Native matrix is Linux/macOS only. |
| `codeql-analysis.yml` | Push, PR and scheduled C++ analysis | Explicit workflow `contents: read`; analysis job retains narrowly scoped `security-events: write`, `actions: read`, `contents: read`. No configured signing secrets consumed. Fork restrictions remain subject to GitHub's token policy. |
| `labeler.yml` | Privileged `pull_request_target`; read contents/write PR metadata | Pinned metadata-only labeler, no checkout or PR code execution; five-minute timeout. Preserve that boundary. |
| `release.yml` | Tag pushes only | Read-only package jobs; publish job alone has `contents: write`. Downloads restricted to `*-release` artifacts from the current workflow run. Collection verifies checksums and rejects duplicate filenames. Draft creation replaces unattended publication. |

All **nine unique pinned action commits** resolved through their declared upstream GitHub repositories. This verifies pin identity/existence, not transitive action code or immutable external downloads inside each action. No `curl | sh`, direct PR-title/body interpolation into shell, privileged fork checkout or PR artifact promotion into a privileged release job was found in the reviewed YAML. The PR base SHA used by formatting is passed through an environment variable and quoted.

Remaining supply-chain limits: Qt uses wildcard patch versions, Ubuntu/Chocolatey/Homebrew packages are not fully pinned, `create-dmg` is installed dynamically, runner images evolve and composite-action internals were not exhaustively audited. Top-level SHA pins do not make every toolchain dependency reproducible. Dependabot already tracked Actions; `/cli` Cargo coverage is now added. The separate historical review-probe manifest is not a supported production workspace.

Linux deployment digest evidence, checked 2026-10-12:

- `linuxdeploy` release `1-alpha-20251107-1`: `c20cd71e3a4e3b80c3483cef793cda3f4e990aca14014d23c544ca3ce1270b4d` (upstream GitHub release-asset digest).
- `linuxdeploy-plugin-qt` release `1-alpha-20250213-1`: `15106be885c1c48a021198e7e1e9a48ce9d02a86dd0a1848f00bdbf3c1c92724` (hash of bytes retrieved from the pinned upstream release URL; no upstream signed checksum was supplied).

These tools were not executed locally. Action/workflow changes were statically checked, not dispatched on GitHub. Signing credentials were neither accessed nor used by this audit. Cancellation/hard runner loss can bypass script cleanup; ephemeral hosted runners and administrator credential policy remain relevant.

## 6. Dependency and licensing findings

The [dependency inventory](sec-p1/dependency-inventory.json) records all **135 external locked package/version entries** and registry checksums. Offline metadata for `aarch64-apple-darwin` supplies target-available license declarations; absent other-platform metadata is explicitly marked. The conservative macOS normal/build packaging graph contains **106 third-party crates**. This is an inventory, not an SPDX/CycloneDX binary SBOM or attribution proof.

CMake has no concrete external FetchContent dependency; its dependency helper is infrastructure. Qt 6.5+ modules are system dependencies (Core, Gui, Quick, Qml, Sql, Test, Network, Widgets, LinguistTools, Multimedia, Concurrent; optional Pdf). CI currently selects 6.11.*, with experimental Windows ARM64 on 6.8.*. Exact deployed Qt, platform DLLs/plugins and their transitive components are not covered by the Rust inventory.

### Verified advisories and compatibility

The public RustSec database was downloaded and compared **locally**, including every locked version when package names repeat. All 23 matching package/advisory pairs were evaluated against their patched/unaffected ranges. The limited numeric/caret comparator covered the ranges encountered; this is not `cargo-audit` certification. Database archive hash and unresolved findings are in [dependency-advisories.json](sec-p1/dependency-advisories.json).

- [`RUSTSEC-2026-0002`](https://rustsec.org/advisories/RUSTSEC-2026-0002.html): `lru 0.12.5` falls outside unaffected `<0.9.0` and patched `>=0.16.3`; iterator soundness issue.
- [`RUSTSEC-2026-0253`](https://rustsec.org/advisories/RUSTSEC-2026-0253.html): `lru 0.12.5` precedes patched `0.18.2`; panic-safety issue with conditions involving unwinding and potentially panicking key destructors.
- [`RUSTSEC-2024-0436`](https://rustsec.org/advisories/RUSTSEC-2024-0436.html): `paste 1.0.15` is unmaintained; this is a maintenance advisory, not a demonstrated Sentinel vulnerability.

`cargo tree -i` confirms Ratatui as the parent of both crates. The inspected Ratatui layout cache uses `get_or_insert` with `(Rect, Layout)` keys; the two reported APIs were not directly called there. This narrows one observed use site but does not establish full unreachability. Ratatui's `lru 0.12` constraint does not admit patched `0.18.2`. A lockfile-only bump is not a compatibility-backed fix. Resolve through a tested upstream migration/backport in a separately reviewed dependency change, including terminal/image APIs and performance. No manifest/lockfile or runtime changes were made here.

Qt/toolchain/C++ component vulnerability coverage remains **NOT VERIFIED**; no vulnerability-free claim follows from the Rust review.

### Original notices and open distribution obligations

Root GPL-3.0 text and GPL-3.0-or-later source/Cargo declarations are present. A marker inventory found 756 source files with SPDX identifiers; files without a per-file marker were not assigned invented authors or copyright. Repository-level licensing is not a substitute for third-party notices.

The six TUI V4-B text gaps were investigated using each locked crate's `.cargo_vcs_info.json`, not guessed release tags:

| Crates | Resolution |
| --- | --- |
| `base64-simd 0.8.0`, `vsimd 0.8.0` | **RESOLVED for original-text inclusion:** original MIT text from `Nugine/simd` commit `d74c030d9dc4f3cae02146d1f497ff62726ef09a`, including upstream copyright/permission notice. |
| `block2 0.6.2` | **UNRESOLVED:** upstream policy at `b4167b582b2f75f9a1be75495c41b765344fd03c`. |
| `dispatch2 0.3.1`, `objc2 0.6.4` | **UNRESOLVED:** upstream policy at `8852b424193ca41602281b3d7540d7c8ed51e49a`. |
| `objc2-encode 4.1.0` | **UNRESOLVED:** upstream policy at `8d214f5477365ffcbcbb7de058c86ed9a518efb7`. |

Original fetched bytes and SHA-256/source URLs are in [license provenance](../../third_party/licenses/provenance.json). The packager checks crate revision, path containment and supplement hash before copying. Upstream policy-only documents are included as evidence but do not clear missing-text gates. The policy describes Apple SDK derivative uncertainty; no legal interpretation or generic substitute copyright notice was invented.

[Third-party notices](../../THIRD_PARTY_NOTICES.md) also cover Tabler MIT, Inter/IBM Plex OFL, original Unicode/Apache/BSD/etc. notices, Qt and toolchain obligations. `icy_sixel` declares a `license_file` rather than SPDX metadata; its original file is preserved without inventing an SPDX choice. Exact chosen license alternatives and compatibility are not certified.

Qt provides multiple licensing routes and includes separately licensed third-party code; consult [Qt licensing](https://doc.qt.io/qt-6/licensing.html) and its [component inventory](https://doc.qt.io/qt-6/licenses-used-in-qt.html). Resolve applicable source, notice and relinking requirements for the actual deployed configuration. Bundling the project GPL alone is insufficient evidence of complete distribution compliance.

Core Warden reference provenance/redistribution permission, sound ownership, complete Rust toolchain notices and FA-3 external similarity/trademark review remain unresolved. No approved visual was modified. No third-party license conflict was positively proved; absence of a proved conflict is not legal clearance. Complete Corresponding Source and an actual compliant binary/source distribution mechanism remain gates.

## 7. Documentation corrections

README and installation instructions no longer imply all implemented infrastructure is acceptance-certified or that workflow output is already an approved published release. AgentRuntime still requires an accepted final answer for completion. Native Windows named-pipe CLI/TUI transport remains unavailable. Plugin/MCP/Skills features remain experimental. Desktop CI build targets are distinguished from native release acceptance.

SECURITY now describes pre-release support limits and links private disclosure, with a fallback to requesting a private contact without sensitive details. The public issue form is explicitly for security documentation questions. CONTRIBUTING adds synthetic fixture/path hygiene and Rust/workflow checks. Release instructions explain drafts and checksum manifests. TUI V4-B history stays conditional; the addendum closes only two text gaps.

No duplicate architecture/support guide or invented Code of Conduct enforcement promise was introduced. Maintainer community/governance policy can be added separately when its owner and reporting arrangements are defined.

## 8. Packaging and release assessment

Desktop CMake installation now carries project GPL, third-party disclosure, Tabler MIT and font notice files. A temporary native macOS staging install verified all five files under `sentinel-desktop.app/Contents/Resources/licenses`. Linux/Windows destination configuration is present but native installer execution is **NOT VERIFIED**.

Release checksum collection was tested with three synthetic platform artifacts: success preserved all manifests and combined entries; corrupt content and duplicate filenames both failed. Windows signs application source/build inputs before CPack reinstallation and stages, then installer files; native signing/installer acceptance needs a configured Windows runner. macOS temporary keychain/certificate cleanup and generated-password masking were added. Existing signing/notarization credentials are referenced only in workflow secrets; this audit did not read/use them.

The evaluation packager was exercised into a new temporary directory without changing the protected V4-B package. It copied available original notices plus supplements and reported four unresolved texts. **676 inventory entries**, internal checksum entries and archive member paths/types were checked; all **462 current canonical PNGs** matched their inputs, and FA-3 manifest hashes matched unchanged assets. No Blender/reference boards, application databases, provider configuration or models were included in that temporary archive's allowlisted stage.

A disconnected PTY smoke check used an explicitly nonexistent daemon socket and no live provider. The existing V4-B executable exited normally but reported asset initialization `Unexpected CW-03A manifest`: the newer CW03B catalog is incompatible with that preserved binary. The final helper now refuses this exact known binary digest with an extended catalog before creating output. **The exploratory temporary archive is not an accepted candidate or deliverable.** Earlier V4-B measurements remain about its protected old binary/assets only; license tests do not transfer them to a fresh archive. The lower-level `stage-release.sh` still only copies inputs and does not certify compatibility; build/validate a matching release first.

Versions remain `1.0.0-rc.8` for application/Cargo metadata, with numeric CMake installer version `1.0.0` and separate prerelease application identity. Tag-to-built-version consistency is not enforced by this audit and must be checked before release. Source archive availability, Qt deployment notices, Windows installers, macOS Developer ID/notarization/Gatekeeper, latest physical terminal acceptance and full reproducible-build identity remain open. Sorted normalized tar metadata is not proof of bit-for-bit reproducible compilation.

No public release was created/published. No personal signing credentials, global installation, user data deletion or live AgentRuntime benchmark was used.

## 9. Manual administrator and owner actions

These are recommendations; repository settings were not read or changed. Verify against current GitHub plan/policy; see [GitHub's secure use guidance](https://docs.github.com/en/actions/reference/security/secure-use).

1. Enable/verify secret scanning, push protection where available, Dependabot alerts/security updates and private vulnerability reporting. Review existing alerts privately; do not post raw scanner output.
2. Protect `main` and release tags with rulesets: block force-push/deletion, require review and successful Linux/macOS/Windows/Rust checks appropriate to the change, and require CodeQL when its path filters run. Avoid permanently required checks that path filters omit. CODEOWNERS currently identifies the single maintainer; define practical independent review/recovery arrangements rather than claiming nonexistent reviewers.
3. Require approval for untrusted external workflow runs; retain read-only default token settings and prohibit privileged PR checkout/execution. Review allowed Actions and transitive download/tool policy.
4. Put release signing secrets behind protected environments/reviewer policy, restrict tag creation and credential access, and separate draft generation from human publication approval. A draft flag alone is not an access-control boundary.
5. Decide ownership/retention for generated review target/bytecode and transcript/screenshots. Remove generated outputs from tracking only in a reviewed cleanup that preserves reproducible probe sources and needed original evidence. Sanitize selected public evidence deliberately and update its recorded hashes; do not silently rewrite historical acceptance.
6. Decide whether historical personal-path exposure warrants remediation. Any history rewrite, remote cleanup or deletion requires explicit approval and coordination; no such action was performed. Previously downloaded/forked data cannot be recalled by editing the latest tree.
7. Resolve four Objective-C notice/SDK questions, Qt/toolchain/artwork/sound/source-distribution obligations and dependency advisories before public binary distribution. Validate matching binary/assets and actual native platforms/signing/physical terminals before publishing.

An external OSV batch query was rejected by automatic approval review because it would disclose the repository-derived dependency inventory to that service. No inventory was sent. A public advisory database download and local comparison provided the safer alternative. No additional approval is needed for a service upload to finish this audit; the rejected query was not retried or bypassed.

## 10. Validation results

PASS describes the stated scope only. FAIL identifies an observed issue; BLOCKED describes an intentionally unmet gate; NOT VERIFIED means no suitable evidence was obtained.

| Validation | Result | Scope / evidence |
| --- | --- | --- |
| Project resolution through Brain | BLOCKED | `brain` executable unavailable; explicit workspace/local instructions used. |
| Working-tree and ownership preservation | PASS | Initial concurrent D2.1 work preserved; owning session committed it. Bytecode hash unchanged; no audit diff to runtime/UI/tests/Cargo/canonical assets. |
| Reachable object/pattern scan | PASS with privacy findings | 11,507 blobs across local refs; no confirmed credential. Home/machine-path exposure remains. |
| Gitleaks history and text snapshot | PASS after triage | Both scanner exits indicated five candidates, all reviewed as synthetic fixture tokens or an artifact hash. Not a zero-finding or complete privacy-clearance claim. |
| Screenshot/transcript whole-corpus clearance | NOT VERIFIED | Two screenshots sampled; machine path confirmed in one. No OCR/complete semantic corpus certification. |
| Git whitespace diff / workflow lint | PASS | `git diff --check`; `actionlint -no-color` after fixes. Before fixes actionlint FAIL included invalid secret context and unsafe checksum globs. |
| Action pin provenance | PASS (identity only) | Nine unique SHAs resolve in declared upstream repositories. Transitive action audit not complete. |
| Dependency/advisory review | FAIL — open findings | Local RustSec review: two affected `lru` advisories and unmaintained `paste`. Qt/toolchain vulnerability scan NOT VERIFIED. |
| License gap remediation | PASS / BLOCKED | Two verified MIT originals supplied; four incomplete-notice/SDK questions and broader distribution obligations remain BLOCKED. |
| CMake configure | PASS | `cmake --preset tests`. |
| CMake build | PASS | `CCACHE_DISABLE=1 cmake --build --preset tests -j6`. Initial default build was blocked by an external ccache directory; disabling ccache avoided that unrelated write. |
| Qt tests | PASS | Final `QT_QPA_PLATFORM=offscreen ctest --preset tests --output-on-failure`: **125/125**, 144.13 seconds. Existing fixture sockets required sandbox escalation; no live acceptance benchmark. |
| Rust formatting | PASS | `cargo +1.94.0 fmt --all --check`. |
| Rust static analysis | PASS | `cargo +1.94.0 clippy --locked --offline --all-targets -- -D warnings`. |
| Rust tests | PASS | `cargo +1.94.0 test --locked --offline`: **122 tests** (14 CLI, 11 IPC, 22 mascot, 75 TUI), zero failures. Initial sandbox run blocked local sockets; approved fixture execution passed. |
| IPC generated contract check | PASS | `python3 tools/ipc/generate.py --check`. No contract changes. |
| All-platform offline Cargo metadata | BLOCKED | Missing cached Windows-only `anstyle-wincon 3.0.11`; target-filtered macOS metadata passed. Complete lock inventory retained without pretending other-target metadata was available. |
| Deployment-tool downloads | PASS (pin review) / NOT VERIFIED (execution) | Fixed hashes enforced before execution; tools not run locally. |
| Release artifact aggregation | PASS | Three synthetic platforms, checksums combined, corrupt artifact and duplicate filename rejection. |
| Desktop notice installation | PASS (native macOS stage) | Five expected known notice files present inside the bundle. No global install. |
| Evaluation archive byte integrity | PASS (exploratory stage only) | Inventory/hashes/archive paths/types/current PNGs verified; not an acceptance claim. |
| Evaluation binary/resource compatibility | FAIL observed; PASS refusal guard | Old V4-B executable rejects CW03B manifest. Final helper exits 2 before output for that known combination. Rebuilding a compatible release remains outside this audit. |
| Evaluation destination protection | PASS | Existing output refused with exit 2; preserved package unchanged. |
| Native Linux/Windows packaging, signing and GitHub execution | NOT VERIFIED | Local macOS environment only; no workflow dispatch, secrets, release or remote setting mutation. |
| Full public release clearance | BLOCKED | Dependencies, privacy retention, four license gaps, Qt/source/artwork obligations and native/physical/signing gates remain. |

Temporary raw logs and redacted scanner reports remain outside the repository. Only sanitized inventories, original permitted license evidence and this report are added. No new suppression hides a real secret; no blanket scanner allowlist was introduced.

## 11. Remaining blockers and recommendation

Continued public source development is **conditional**, with documented residual privacy exposure in current/history evidence and outstanding dependency/artwork/licensing review. It must not be described as a completed clean-history certification. Public binary distribution is **NOT CLEARED**.

The concrete blockers are the affected `lru` dependency, unmaintained transitive tooling, four incomplete Objective-C notices/SDK questions, Corresponding Source/Qt/toolchain/resource obligations, deliberate evidence-retention/redaction decisions, compatible fresh binary/assets, and release-specific native platform, physical terminal and signing acceptance. Administrator protections also need confirmation outside repository files.

The hardening work ends with this report. No subsequent development phase, credential rotation, history remediation or publication begins automatically.
