# Engine development and releases

This repository owns the simulator, protocol, C++ tests and provenance. The
[RL project](https://github.com/martenkrohn/seven-wonders-rl) owns agents,
features, exported Python databases and training/evaluation tooling.

Before a release, build in Release mode, run CTest, update
`docs/SIMULATOR_CHANGES.md`, and regenerate/check the exact upstream audit:

```sh
python3 scripts/audit_simulator.py
python3 scripts/audit_simulator.py --check
```

Regeneration needs upstream commit `a0bc333a7a7f21f83156de8a2d522e7baa913f0a`
locally; a normal clone retains it. `--check` also works with a shallow clone.
Update `engine-contract.json` when changing the release, ruleset or protocol.
Commit the tested state and create an annotated release tag. The consumer
updates its submodule to that exact commit, updates its engine lock, and tests
the bridge plus any derived card/wonder databases before merging the update.
Engine CI needs no Gymnasium, NumPy or PyTorch.

## Next ruleset: BGA second edition

Use the previously collected BGA reference as the target source of truth.
The complete card/wonder comparison remains in the RL project's
`docs/reference/edition-comparison/`. Migration has not started here.

Implement independently reviewable changes with rule regressions: data and
costs; wonder abilities; discard selection and simultaneity; science scoring.
Settle the action/protocol changes for extra decisions before adapting agents.
Give the migrated ruleset a new identifier and a new release. Keep
`v0.1.0-legacy` available to reproduce older experiments. Do not silently
reuse old checkpoints or exported data as evidence for the new ruleset.

`upstream` should refer to `https://github.com/dmag-ufsm/7Wonders.git`;
`origin` is the maintained fork. The fork retains upstream ancestry, original
license and notices. The extracted engine's game source is byte-for-byte
identical to the RL snapshot tagged `pre-engine-split-2026-11-05`.
