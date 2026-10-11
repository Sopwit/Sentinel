# V3.3-C evidence

All terminal journeys use the actual release executable with disposable typed IPC fixtures. They are not physical Ghostty or live inference acceptance. Text snapshots replay terminal cells, not screenshots. No real daemon is started or modified.

From the repository root:

```sh
python3 docs/reviews/tui-v33c/interaction.py
python3 docs/reviews/tui-v33c/cw045_regression.py
python3 docs/reviews/tui-v33c/performance.py docs/reviews/tui-v33c/baseline/sentinel-v33b docs/reviews/tui-v33c/baseline/performance.json
python3 docs/reviews/tui-v33c/performance.py cli/target/release/sentinel docs/reviews/tui-v33c/after/performance.json
```

Run CPU comparisons sequentially on macOS, without concurrent builds or tests. The preserved B executable is local and ignored by Git. The harness uses proc_pid_rusage plus mach_timebase_info and cross-checks cumulative CPU against wait4. Percent means percent of one CPU core; terminal-emulator work is excluded. Parent output receipt is an automated latency proxy, not physical display latency.

`tests.log`, `fmt.log`, `clippy.log`, `debug-build.log`, `release-build.log` record final Rust checks. `layout/` contains 48 TestBackend snapshots across six sizes. `baseline/scroll-regression.log` records the demonstrated pre-fix resize failure. Earlier exploratory logs in baseline are superseded by the final performance.json and performance.log. The initial optional layout export used an incorrect relative directory; absolute-path export passed. TestBackend assertions themselves passed in the full suite.
