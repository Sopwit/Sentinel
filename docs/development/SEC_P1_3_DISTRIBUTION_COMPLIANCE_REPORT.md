# SEC-P1.3 — Licensing and distribution compliance

Date: 2026-10-12. Baseline HEAD: `541b8db91902066840093425109e9d9d93ae3b5c`.

Final recommendation: **BLOCKED**. Verified notice and packaging corrections were applied, but no public binary or blanket source/artwork licensing clearance is issued. SEC-P1.2's two lru advisories and paste maintenance advisory remain open. No dependency migration was restarted.

The SEC-P1, SEC-P1.1 and SEC-P1.2 reports were read before edits. Personal-Brain's `brain` executable was unavailable; the explicitly supplied repository and project instructions were used. Architecture/runtime/security/build/testing context was retained. Existing uncommitted work was preserved. This phase changed notices, installer license encoding and packaging disclosure only; it made no runtime, AgentRuntime, daemon, IPC, dependency or visual changes.

## 1. Licensing audit and scope

Sentinel declares GPL-3.0-or-later in the Rust workspace and source, with the GPL v3 text at the root. This is the project license, not permission for every external asset or bundled library. Existing attribution files and actual CMake resources were cross-checked. Rust metadata was evaluated offline with an external Cargo target directory; no private repository inventory was uploaded.

The lockfile still contains 135 external package/version entries. The macOS arm64 normal/build graph contains 106 external crates, including image/PNG/JPEG/Sixel libraries and build-time proc macros. This conservative graph is not a symbol-level binary SBOM. The [notice inventory](sec-p1-3/rust-notice-inventory.json) records license declarations, exact VCS revisions where supplied, original notice filenames and verified supplements. A filename or SPDX declaration alone is not legal clearance; alternative license branches, required NOTICE files, Unicode terms and actual linked code still need release review. Other-target packages and dev-only packages must be reconsidered if distributed.

CMake links project libraries and Qt; `SentinelDependencies.cmake` provides FetchContent infrastructure without declaring a concrete downloaded third-party project. SQLite is accessed through Qt SQL; a deployed SQLite plugin has its own dependency inventory. macOS system frameworks include Carbon, AppKit, UserNotifications, Security, CoreFoundation, Speech and AVFoundation. Windows platform libraries and MinGW runtimes, when used, need target-specific classification. No OS SDK or toolchain redistribution permission is inferred from the project GPL.

## 2. Four original-notice investigations

Exact versions and revisions were taken from the locked local crate archives' `.cargo_vcs_info.json`, not inferred from latest tags:

| Crate | Exact upstream revision | Finding / status |
| --- | --- | --- |
| block2 0.6.2 | `b4167b582b2f75f9a1be75495c41b765344fd03c` | **BLOCKED** — no complete verified original copyright/permission text. |
| dispatch2 0.3.1 | `8852b424193ca41602281b3d7540d7c8ed51e49a` | **BLOCKED** — same notice gap; do not assign an invented copyright holder. |
| objc2 0.6.4 | `8852b424193ca41602281b3d7540d7c8ed51e49a` | **BLOCKED** — same notice gap. |
| objc2-encode 4.1.0 | `8d214f5477365ffcbcbb7de058c86ed9a518efb7` | **BLOCKED** — same notice gap. |

All four published crate directories lack original license files. Public recursive Git trees were downloaded for all three immutable revisions; none was truncated. The only license-named document found is root `LICENSE.md`; similarly named copying tests are source tests, not notices. [Investigation receipts](sec-p1-3/original-notice-investigation.json) record response hashes and exact URLs. This is bounded file-tree/published-crate evidence, not proof that no notice exists anywhere in upstream correspondence or earlier history.

The retained exact-revision policy documents are already preserved under `third_party/licenses/madsmtm/objc2/`, with SHA-256 `7f976f7e9cb2d87df7230606feb932c3f21ac0e664045a775b600046ff850c54`. They describe MIT or alternative licensing and explicitly raise Apple SDK derivative uncertainty. They do not supply a complete original MIT copyright/permission notice. Linking to a generic MIT template would not establish the missing original holder/text. No substitute text, legal permission or guessed authorship was created.

For each crate, packaging must preserve verified applicable original copyright/permission text and any required attribution; including the policy is useful evidence but does not close that requirement. Obtain an upstream clarification tied to these revisions and a maintainer/legal decision on any Apple-derived portions and relevant Xcode SDK terms. macOS compilation with Xcode does not establish permission to redistribute derived code. No upstream author was contacted automatically.

The two SEC-P1 MIT supplements for base64-simd 0.8.0 and vsimd 0.8.0 remain byte-identical and valid against exact `Nugine/simd` revision `d74c030d9dc4f3cae02146d1f497ff62726ef09a`, SHA-256 `71674605ec4c087fe9eb534e3e4f9e26eb2e4aabcd76a29fd156c6a844d44b3d`. All six provenance records passed local revision/hash checks.

## 3. Safe corrections applied

- Added the unchanged original Lucide ISC/Feather MIT license from immutable upstream revision `5b44cea30b9df920d80ebe2e0a930772722ac107`, SHA-256 `b495047bd93a9b06913511076f504daba17d5bbeb3e0650f3bb53a4220329c57`, plus retrieval provenance. Five Lucide SVGs are embedded by Desktop CMake; previously only Tabler attribution was installed. CPack now includes the Lucide files alongside Tabler. The local SVG acquisition revision and modifications remain unverified, so this resolves notice availability, not full asset provenance. [Original source](https://raw.githubusercontent.com/lucide-icons/lucide/5b44cea30b9df920d80ebe2e0a930772722ac107/LICENSE).
- Added `resources/fonts/LICENSES/COPYRIGHTS.txt`, transcribed directly from unchanged OpenType name tables, including exact font hashes, copyright, trademarks, designer names and OFL references. Inter identifies version 4.001/git-9221beed3 and Copyright 2016 The Inter Project Authors; IBM Plex Mono identifies version 2.005 and Copyright 2017 IBM Corp. The existing OFL text and embedded notices remain intact. Existing directory installation automatically includes this supplement.
- Corrected `packaging/windows/installer/license.rtf`: the old file contained doubled RTF control backslashes and literal newline escapes. The replacement uses valid single RTF controls and round-trips to the complete unchanged root GPL text. Native WiX rendering remains unverified.
- Added the existing project `THIRD_PARTY_NOTICES.md` to low-level Rust staging under `share/licenses/sentinel/`, and updated that disclosure with the new notice supplements. This low-level script still does not collect every Rust license; the evaluation packager performs that additional collection.

No original asset, font, evidence, measurement or artwork hash was rewritten. The Lucide copyright text is an upstream original notice, not an assertion of ownership over the locally adapted SVGs.

## 4. Qt licensing route and actual linkage

The available native macOS debug Desktop links **shared Qt 6.11.2 frameworks**, verified with `otool -L`; `Qt6::Core` is an imported SHARED CMake target. QtPdf is actually linked in this build. The prefix stage still references externally installed Qt; it is not a self-contained deployed application. No static Qt build or native Linux/Windows binary was examined.

No commercial Qt entitlement or recorded per-module licensing election was found. The evidenced build uses open-source Homebrew Qt. A proposed compliant route is Sentinel under GPL-3.0-or-later, eligible Qt modules under their GPLv3 option, and modules without that option under an applicable compatible alternative. This route is **not yet selected and verified for the release payload**. In particular, Qt PDF 6.11.2 documents **LGPLv3 or GPLv2**, plus commercial terms; do not assume every Qt module offers GPLv3. Use the LGPLv3 route for PDF unless another verified permission applies. [Qt 6.11 licensing](https://doc.qt.io/qt-6.11/licensing.html), [Qt PDF 6.11.2 licensing](https://doc.qt.io/qt-6.11/qtpdf-licensing.html).

Qt PDF contains PDFium and third-party code; its official list includes Abseil, FreeType, Chromium, fast_float, ICU, JPEG, PNG and zlib terms. These are additional to Qt's license. Local Homebrew SBOMs exist for qtbase, qtdeclarative, qtmultimedia and qtwebengine and enumerate 36/37/42/44 package records respectively, including dependencies and tool/build records. They are useful leads, not the exact deployed subset or original notice bundle. The project currently does not collect this complete payload-specific attribution/source set. Do not bundle the whole WebEngine SBOM and claim it proves which PDF components were shipped.

Required release review:

1. Record exact Qt modules, versions, patches, build flags, plugins, QML imports and their actual licensing routes; retain original applicable copyright/license/NOTICE texts and prominent Qt-use disclosure. Include both GPL and LGPL texts where LGPL applies. Current install notices do not include a complete Qt notice/license set.
2. Establish Corresponding Source delivery for Sentinel and conveyed covered dependencies: exact source, patches, generated-interface inputs, build/install scripts and required assets, matched to the executable. For network distribution, arrange a GPL section 6 delivery method with accessible matching source and clear directions alongside the binary. A generic repository homepage or automatically generated GitHub snapshot alone has not been verified to satisfy this. Preserve notices and identify modifications. [GPL v3 terms, sections 1, 4–6](https://doc.qt.io/qt-6/gpl.html).
3. For LGPL libraries, preserve permitted modification/reverse-engineering rights and meet the applicable shared-library replacement route or supply suitable relinkable application code and Minimal Corresponding Source. Provide Installation Information where required. Dynamic linkage alone does not prove replacement works; signatures, loader paths and distribution terms must be checked. Conveyed library source obligations remain. [LGPL v3, sections 3–4](https://doc.qt.io/qt-6/lgpl.html).

The static alternative is unverified: it would require a separate module-license assessment and source/relink plan, with object material where the selected LGPL route requires it. No static conversion was attempted. Tools used only during build are distinguished from redistributed runtime components, but generated outputs/exceptions still require review.

## 5. Artwork, branding, fonts, icons and sounds

| Asset group | Verified technical provenance | Redistribution evidence / disposition |
| --- | --- | --- |
| Core Warden original reference | `assets/core-warden/reference/Core_Warden_Original_Reference.png` and historical modeling reports identify the reference. | Original creator, acquisition terms and derivative permission are not documented sufficiently. **BLOCKED**, including public source copies; owner confirmation required. |
| Blender masters and canonical PNGs | Retained editable masters, procedural build scripts, manifests and review hashes. Reports describe mesh/material generation without external texture sets. All pre-phase bytes preserved; 462 animation PNGs and two JSON runtime inputs match staged copies. | Procedural generation and integrity do not clear reference rights, author/contributor grants or any embedded data. **BLOCKED** pending ownership chain. No Blender or image edits. |
| Textures/other source assets | No external texture sets documented in inspected CW-01/02 reports/scripts. | Whole-file Blender dependency and ownership clearance was not independently repeated. **NOT VERIFIED**; review external/packed resources with the owner. |
| FA-3 branding | All 69 files pass the frozen inventory/geometry/hash guard. Manifest says PROVISIONAL LOCK / EXTERNAL EVIDENCE WAIVED. | Prior package origin and technical approval are not a copyright/trademark grant. Original authorship, redistribution terms and waived similarity review need owner disposition. **BLOCKED** for clearance. |
| Inter / IBM Plex Mono | Embedded exact metadata and hashes, original OFL text, new external notice supplement. Font bytes unchanged. | Notice preservation improved. Inter release-byte identity and complete acquisition records remain unverified; preserve OFL conditions/reserved-name restrictions for any future modification. |
| Tabler / Lucide functional icons | Tabler MIT text retained; Lucide original ISC/Feather MIT text newly preserved. | Exact acquisition/modification history for local SVGs still requires confirmation; no wholesale legal clearance based on directory names. |
| Three WAV sounds | `notification.wav`, `error.wav`, `success.wav` are embedded in Desktop CMake resources. | No adequate author/license record found in inspected resource/docs/tools context. **BLOCKED**; obtain original terms or owner authorship evidence. Do not silently remove/replace sounds. |

Source repositories also convey artwork, font and evidence files. Binary omission of a reference or Blender master does not retroactively clear those public source copies. SEC-P1.1's personal metadata/raw evidence concerns remain; no current edit removes historical exposure or recalls clones/downloads.

## 6. Packaging and source-distribution assessment

| Route | Verified behavior and paths | Remaining gates |
| --- | --- | --- |
| macOS Desktop | CMake prefix install places license/disclosure, Tabler, Lucide and font notices inside `sentinel-desktop.app/Contents/Resources/licenses`; canonical assets under `Contents/Resources/core-warden`. Native temporary install and hash checks pass. Release workflow uses macdeployqt and DMG tooling. | Final deployed frameworks/plugins/Qt notices/source and replaceability not inspected. No DMG, signing, notarization or publication run. Temporary debug stage is not a release artifact. |
| Linux Desktop | CMake common notices under `share/licenses/sentinel`, assets under `share/sentinel/core-warden`; workflow stages AppDir and runs pinned-digest linuxdeploy/Qt plugin. DEB/RPM also declare system Qt dependencies. | Actual AppImage payload, system-library boundaries, plugins, notices and matching source need native verification. AppImage-conveyed libraries differ from system-dependent packages. No Linux packaging tools executed. |
| Windows Desktop | CMake common notices use `share/licenses/sentinel`; WiX/NSIS project-license inputs reviewed; RTF fixed. windeployqt stages Qt dependencies. | Native installer rendering/payload and runtime notices/source unverified. Workflow runs windeployqt on `dist`, while CPack installs from build rules; do not assume staged DLLs/plugins automatically enter the installer. Inspect final installers and MinGW runtime permissions where applicable. |
| Rust CLI/TUI | Low-level stage includes GPL and project disclosure. macOS evaluator builds a conservative original-notice inventory, SHA-256 sums and normalized archive metadata, rejects symlinks and the exact known incompatible historical executable/catalog. | Four original texts, Apple-derived concerns, Rust/toolchain runtime attribution/source, artwork and SEC-P1.2 advisories remain. Linux needs its own target inventory. No new accepted TUI archive was assembled. |

The protected V4-B binary SHA-256 `45087e81132d23b934c88b152195f5f59f5dd0276c7639358ef8e370a690491c` cannot read the current CW03B ten-state catalog. The high-level evaluator returned exit 2 **before creating its requested output**. A separate low-level temporary copy was used only to verify file inclusion; it still contains that incompatible binary and is expressly **not accepted or runnable-package evidence**. The low-level script does not enforce the compatibility guard. Protected historical binary/archive acceptance was not reused. SEC-P1.2's separately built baseline executable is also test evidence, not an accepted distributable.

Version metadata: CMake and Cargo source identities are `1.0.0-rc.8`; numeric project/CPack/plist values are `1.0.0`, and the inspected staged plist has `CFBundleVersion=1.0.0`. This loses prerelease identity in native package metadata. The release workflow's tag is not proof that cached binaries, Cargo version, source and assets correspond. Require an explicit tag/source/build-version/asset digest receipt; native upgrade ordering and build numbers remain unverified. No version renumbering was guessed here.

Existing release checksum/duplicate protections from SEC-P1 remain. Hashes prove byte integrity, not copyright permission, matching source availability or runtime acceptance. A final release should include exact binary/assets/notices/source inventories and checksums, with a maintained source-download mechanism for the chosen GPL section 6 route and relevant library terms. Do not issue a guessed written offer on behalf of the maintainer.

## 7. Distribution readiness matrix

RESOLVED means the stated target's relevant gate is actually closed; NOT VERIFIED means no matching native evidence. A BLOCKED compliance cell is not an assertion of infringement.

| Target | Legal/compliance | Technical readiness | Overall |
| --- | --- | --- | --- |
| Public source repository | **BLOCKED** for complete clearance: artwork/reference/sounds ownership, tracked third-party generated binaries/evidence, incomplete original notices. GPL project declaration present. | **NOT VERIFIED** as a reproducible release source delivery; ordinary development may continue subject to existing audit caveats. | **BLOCKED** for blanket compliance approval. |
| macOS Desktop binary | **BLOCKED**: Qt/source payload, assets/sounds ownership. Notice install subset verified. | **NOT VERIFIED** for final deployment; native debug staging and focused test pass only. | **BLOCKED**. |
| Linux Desktop binary | **BLOCKED**: payload-specific Qt/third-party/source and asset obligations. | **NOT VERIFIED** natively. | **BLOCKED**. |
| Windows Desktop binary | **BLOCKED**: same obligations plus installer/deployed runtime inventory. | **NOT VERIFIED** natively; corrected RTF not installer acceptance. | **BLOCKED**. |
| macOS/Linux TUI binary | **BLOCKED**: macOS four-crate notices/Apple questions, target-specific original texts, artwork/source and open security advisories. | **BLOCKED** for protected old binary/current assets; fresh macOS baseline tests are prior-phase evidence only. Linux **NOT VERIFIED**. | **BLOCKED**. |
| Windows native CLI/TUI | **BLOCKED**: target inventory/source/asset/security gates. | **BLOCKED**: native Windows terminal IPC is unsupported; no Windows acceptance. | **BLOCKED**. |

Resolved subchecks include original inclusion of the two SEC-P1 MIT supplements, new Lucide notice retrieval, font metadata preservation, RTF text round-trip and macOS notice installation. No entire distribution target is RESOLVED. Desktop does not thereby inherit the Rust lru dependency; nevertheless the project's requested public binary release remains blocked by SEC-P1.2 regardless of licensing progress.

## 8. Validation and evidence

[Validation receipts](sec-p1-3/validation.json) contain portable source/staged paths and hashes. Private raw logs and upstream responses remain under `/tmp/sec-p1-3-*`; move necessary receipts to approved durable private storage before temporary storage expires.

- PASS: all six original provenance entries matched crate revision and retained text hash. The 106-package notice scan still identifies exactly four unresolved original-text gaps; existence checks do not certify complete legal terms.
- PASS: `cmake --preset tests` and `cmake --install build/tests --prefix /tmp/sec-p1-3-desktop-stage`. No global install or deployment/signing step.
- PASS: seven license/disclosure inputs copied byte-for-byte into the temporary native macOS bundle; 462 canonical PNGs and two runtime JSON inputs match source hashes.
- PASS: `QT_QPA_PLATFORM=offscreen ctest --test-dir build/tests -R test_core_warden --output-on-failure`: 1/1 test, 3.84 seconds, existing native debug test executable. No production runtime edit required a fresh broad behavioral suite. Prior full-suite results remain historical, not new release acceptance.
- PASS: `python3 tools/branding/check.py`: all 69 FA-3 files, geometry and hashes. No art export or image normalization.
- PASS: Windows RTF decoder round-trip equals complete unchanged `LICENSE`; native Windows rendering NOT VERIFIED.
- PASS: `sh -n tools/terminal/stage-release.sh`; temporary low-level license/disclosure inclusion; high-level known-incompatibility rejection before output creation. No accepted new archive/checksum manifest claimed.
- PASS: whitespace/diff checks and pre-phase file digest comparison, with only four intended existing-file corrections. All source/dependency/asset/evidence inputs and staged index remain unchanged. The tracked inventory remains 7,862; no untracking or deletion.

The checkout remains intentionally dirty with earlier SEC work and this phase; it is not described as clean. No fetch, commit, push, merge, tag, history rewrite, global installation, signing, notarization, publication or inventory upload occurred.

## 9. Remaining blockers and exact manual actions

1. Keep public binary publication blocked. Resolve SEC-P1.2 advisories in its own compatibility-reviewed change; do not treat a licensing test as a security waiver.
2. Obtain exact-revision original notices/clarifications for all four Objective-C-family crates and resolve Apple SDK derivative questions. Integrate verified originals with revision/hash receipts; never invent missing copyright. Preserve the two verified MIT supplements.
3. Confirm the Core Warden reference's original owner and derivative/redistribution permission; document contributors' permissions for masters/renders. Confirm FA-3 ownership, external review disposition and the sounds' author/license records. Review local icon acquisition/modification history and font acquisition evidence. Preserve approved bytes pending decisions.
4. Select/document per-module Qt license routes, including the actual QtPdf alternative. Build native deployed payload inventories and original notices from the exact Qt/source packages, plugins and linked libraries. Verify LGPL replacement/relink and any installation-information requirements; do not assume signing allows replacement.
5. Establish and test Corresponding Source delivery for each exact artifact: source revision and patches, required dependencies/assets, build/install instructions, notices and checksums. Decide the applicable GPL section 6 mechanism and maintain it for its required period. Do not substitute a generic GitHub URL or an unverified auto-source archive.
6. Inspect native macOS deployment/DMG, Linux AppImage/package and Windows installer contents. Check Windows dist-versus-CPack payload transfer, license rendering, Qt DLLs/plugins and runtime permissions. Reconcile prerelease/native version identity and upgrade behavior before tagging.
7. Build a fresh compatible TUI artifact only after its gates close. Repeat asset-enabled terminal/signal acceptance on that artifact and its exact assets; generate target-specific original notices, inventory and checksums. Do not reuse protected V4-B acceptance.
8. Resolve SEC-P1.1 retention/privacy decisions separately; no original evidence deletion or untracking is authorized here. Current fixes do not erase historical exposure or establish what forks/downloads retain.

Final recommendation: **BLOCKED**, with the verified notice/packaging subchecks above resolved. Stop after this report; no SEC-P1.4, SEC-P1.1B, dependency migration or Desktop development begins automatically.
