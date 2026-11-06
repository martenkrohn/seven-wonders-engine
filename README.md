# Seven Wonders Engine

Standalone C++17 simulator for the three-player **7 Wonders second-edition
base game**, forked from [DMAG/UFSM](https://github.com/dmag-ufsm/7Wonders).
No RL framework is required. Release contract: **`v0.2.0-bga`**, ruleset
**`bga-second-edition`**, protocol **3**, **`decision-v1`**.

All 80 card/age rows, 25 chain links and 14 wonder sides use the frozen BGA
definitions. Ludus, Decorators Guild, optimal science jokers, new Olympia
free-build modes, Babylon's seventh card and mandatory Halikarnassos choices
are implemented. Regular choices settle from a frozen round-start economy;
extra decisions finish before military resolution and final scoring.

`second_edition_implemented=true` records acceptance of this defined scope.
This is an unofficial implementation against collected definitions, not a copy
of BGA server rules. See [limitations](docs/KNOWN_ISSUES.md) and
[release acceptance](docs/RELEASE_BGA.md) for evidence and remaining boundaries.
The legacy tags preserve protocol-2 experiments; old checkpoints are incompatible.

## Build and check

Linux/Ubuntu (including WSL), CMake and a C++17 compiler are supported.
Python 3.10+ enables standalone protocol checks.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 scripts/audit_simulator.py --check
python3 scripts/audit_bga_targets.py --check
python3 scripts/generate_card_data.py --check
python3 scripts/smoke_protocol.py --engine build/7Wonders --seed 8
```

The binary needs no source-tree JSON files or artwork at runtime. Matches use
separate working directories. Identical seeds reproduce within one toolchain;
cross-platform shuffle equivalence is not guaranteed.

[Protocol 3](docs/PROTOCOL.md) documents versioned offers, physical card IDs,
request receipts, isolated file exchange, final status and abort handling.
The shared standard-library client is `sevenwonders_engine.DecisionClient`:

```sh
python3 -m pip install -e .
```

## Graphical client

Python 3.10+, Tk and a display are required. Pillow enables optional artwork.

```sh
python3 -m pip install -e '.[gui]'
python3 Game.py 2 --seed 8
```

Zero bots means three local humans, two bots one human, three bots an observed
match. Offers explicitly include free modes and mandatory discard selections.
Hotseat handover hides the next hand until revealed. Current names, costs and
effects remain readable without images. Externally supplied BGA artwork is
checked against the frozen manifest; it is not included in the repository.
See [GUI instructions](docs/GUI.md).

## Development and provenance

[Development](docs/DEVELOPMENT.md), [generated data](docs/CARD_DATA.md),
[migration history](docs/MIGRATION_PLAN.md) and
[simulator changes](docs/SIMULATOR_CHANGES.md) describe the implementation.
Frozen references and historical validation files retain their original hashes
and development metadata. Current runtime/export metadata comes from
`engine-contract.json`. [Seven Wonders RL](https://github.com/martenkrohn/seven-wonders-rl)
consumes an exact commit, neutral data exports and the shared process client.

GPL-3.0-only; see [LICENSE](LICENSE), [NOTICE](NOTICE) and [origin](docs/ORIGIN.md).
Vendored nlohmann/json retains its MIT notices. No publisher or BGA affiliation.
