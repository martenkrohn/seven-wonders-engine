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

## Accepted BGA second edition

Current contract: `v0.2.0-bga` / `bga-second-edition`, protocol 3,
`second_edition_implemented=true`, exactly three players. Both generated data
and native runtime metadata come from the operational engine contract; frozen
reference metadata remains historical. Run generator checks after any change:

```sh
python3 scripts/audit_bga_targets.py --check
python3 scripts/generate_card_data.py --check
python3 -m unittest discover -s tests -p 'test_bga*.py' -v
xvfb-run -a python3 -m unittest discover -s tests -p 'test_gui*.py' -v
```

Engine CI runs CTest, target/generator/audit checks and actual Tk sessions.
The RL consumer separately runs its full historical/BGA Python suite, package
checks and a fresh protocol-3 PPO smoke. Old checkpoints are rejected by the
new feature/data/rules contract. See [release acceptance](RELEASE_BGA.md).

Prepare and validate both repositories locally before publication. Publish the
engine commit/tag before the consumer commit/tag so its exact pin is fetchable.
Remote publication requires the explicitly approved destinations. Do not force
push or modify retained legacy tags. The locally prepared pin does not imply
that its engine commit has already been published.

`upstream` should refer to `https://github.com/dmag-ufsm/7Wonders.git`;
`origin` is the maintained fork. The fork retains upstream ancestry, original
license and notices. At extraction, the engine's game source was byte-for-byte
identical to the RL snapshot tagged `pre-engine-split-2026-11-05`. Subsequent
source changes are recorded in the simulator audit and change log.
