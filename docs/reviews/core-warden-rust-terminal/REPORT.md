# Core Warden isolated Rust terminal investigation

## Finding

The keyboard-dependent stall was reproduced in Ghostty by the user and in a controlled PTY. Rust stdout, raw mode, alternate screen, Ratatui drawing, and the existing mascot Player each presented the image without input in the baseline suite. The failing combination was the existing capability probe followed by Crossterm input polling.

The shared capability detector sends the upstream query ending in `CSI 5n` (device status request), but returns when graphics support and cell size are known. A separately arriving `CSI 0n` status response can therefore remain in the terminal input stream. Crossterm 0.28.1 does not parse that response into an input event. Its Unix input source then performs another read on the blocking terminal descriptor, so the requested poll timeout does not bound this inner read. A keyboard event releases it. The main loop cannot consume the completed background image load while blocked.

## Physical evidence supplied by the user

All seven baseline stages passed without input: stdout, stdout-alt, raw, raw-alt, ratatui-before, ratatui-after, and player. Records are in `physical-baseline/`.

| Extended stage | Visible without input |
| --- | --- |
| player-probe | Yes |
| player-poll | Yes |
| player-probe-poll | No; textual `[ S ]` remained until a key |
| player-probe-poll-no-status | Yes |

Records are in `physical-extended/`. In the failing trace, `event.poll.begin` occurred at 7,871 microseconds with a requested 50,000-microsecond timeout; it completed at 16,163,262 microseconds after keyboard input. Player presentation followed at 16,181,002 microseconds. The poll blocked approximately 16.155 seconds.

With only the final status query removed, polling ran from 9,733 to 60,779 microseconds (51.046 milliseconds), returned without an input event, and Player presentation occurred at 79,164 microseconds.

These are physical observations reported by the user, correlated with child-process monotonic traces; they are not an agent-observed production Ghostty pass.

## Controlled byte and operation evidence

`fragmented_probe_check.py` varies response fragmentation while keeping the image and rendering path constant. Probe alone, polling alone, and coalesced probe replies passed. A separately delivered status reply stalled the combined probe/poll path: no image upload was emitted before the controlled synthetic key; upload followed the released poll. Removing the status request passed without a synthetic key, including a late device-attributes response.

Earlier tests that delivered all responses in one write concealed the defect: the capability reader consumed the status reply along with the required responses. Successful upload/placement/flush traces from other runs do not establish that the blocked run reached those boundaries without input.

The isolated controls record successful stdout writes, Kitty operations, flush completion, lifecycle transitions, and owned-image cleanup. They do not log keystroke contents or private session data. Baseline observation holds produce no presentation writes during the five-second interval. The hold is an observation window, not a proposed rendering delay.

## Smallest proposed correction — not applied

Remove only the final `CSI 5n` from the shared capability query in `cli/crates/sentinel-mascot/src/capability.rs`. Keep actual graphics acknowledgment, device-attributes and cell-size queries, the existing 300-millisecond/4096-byte limits, and unsupported-terminal fallbacks. The local isolated candidate physically passed. `PROPOSED_FIX.patch` contains the reviewable change with a guarded suffix check that falls back if the upstream query format changes.

This avoids generating the stray status response rather than introducing timers, screen clearing, extra image protocols, or global redraws. Both production and standalone consumers use the common capability layer. No production change was applied, as requested by the isolated investigation mission. Production regression tests and a fresh physical production acceptance remain necessary if the proposal is implemented.

## Validation and scope

- Isolated release build: passed.
- Isolated Rust unit tests: 2 passed.
- Isolated Clippy, all targets with warnings denied: passed.
- Baseline PTY checks: 7 passed, including output ordering, owned cleanup, terminal restoration and no-input presentation.
- Fragmented-response checks: 5 passed, including the expected controlled failure and candidate correction.
- Physical baseline: 7 stages passed, reported by the user.
- Physical extended comparison: failing combination and passing candidate confirmed by the user.

Artifacts and source changes for this investigation are confined to this directory. Production TUI, daemon, AgentRuntime, Desktop and Blender assets were not modified. No commit, push or merge was performed. The standalone result does not claim production animation, layout, overlay or terminal acceptance.
