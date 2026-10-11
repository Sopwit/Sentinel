# V3.2.5 reproduction

Use repository-local `cli/target/release/sentinel`, never the globally installed Cargo binary. Build daemon with `CCACHE_DISABLE=1 cmake --build --preset release`. Check localhost LM Studio catalog and refuse to alter user-loaded instances. Only the already-installed approved Qwen/Nemotron models may be loaded; use task-owned 8192 context/four-slot instances and restore the initial empty loaded state.

`live_readonly.py` imports the earlier isolated-profile IPC helpers; set SENTINEL_V32_DAEMON to a frozen rebuilt daemon, V323_OUT to a private evidence directory and V323_MODEL to the task-owned instance. QT_LOGGING_RULES enables provider, agent, classification and agent.loop diagnostics at info. The harness drives a real PTY TUI, creates synthetic workspaces, denies every write/process approval, and records every attempt plus fixture hashes and actual terminal events. Defaults: three A–E attempts, 150-second observer bound; separate cancel and active-session reconnect controls. V325_BOUND overrides normal journey bounds only; V325_ONLY filters comma-separated label prefixes; V325_ATTEMPTS changes repetition count. Those settings must be reported with results.

The first Nemotron batch used the frozen `evaluated.patch` source snapshot; later metadata and unused Claude/Gemini output-limit propagation are in `final-source.patch`. Binary digests distinguish the batches. Never claim they are identical source builds.

`classification_probe.py` reconstructs the production prompt text with a synthetic context/domain list to compare the local HTTP body format; it is a transport control, not an exact copy of a particular TUI conversation. The explicit QtTest live probe calls the actual policy/provider and built-in descriptors:

```
SENTINEL_LIVE_CLASSIFIER_MODEL=nvidia/nemotron-3-nano-4b \
QT_LOGGING_RULES='sentinel.provider.diagnostics.info=true;sentinel.classification.diagnostics.info=true' \
build/release/tests/test_observation_policy optInLiveClassification
```

Keep the model idle for independent probes. Default deterministic tests skip this opt-in test. Source contents/prompts/private reasoning are not logged by production diagnostics. Synthetic goals/results in the harness evidence are task-owned fixtures.
