# Seven Wonders Engine

A standalone C++17 simulator for three-player **7 Wonders**, forked from
[DMAG/UFSM's 7Wonders](https://github.com/dmag-ufsm/7Wonders). It exposes a
JSON file protocol for bots and other clients. No RL framework is required.

**Development tree: `v0.2.0-dev`, ruleset `bga-second-edition-development`.**
Science jokers are scored optimally. Package 4b-II implements simultaneous
settlement and mandatory discard decisions with stable physical card identities,
request deduplication and end-of-age barriers. The active [protocol is 3](docs/PROTOCOL.md),
contract `decision-v1`; the [engine contract](engine-contract.json) distinguishes
transport from rules. Card/wonder definitions and activation stages still use
the legacy data; `second_edition_implemented=false`. See the
[resolution validation](docs/SIMULTANEOUS_RESOLUTION.md) for scope and evidence.

The tagged `v0.1.1-legacy` release preserves the restored original Tk GUI and
the simulator used by [Seven Wonders RL](https://github.com/martenkrohn/seven-wonders-rl).
Its exact RL pin remains unchanged; this development tree has not been released
or adopted by the RL project.

## Build and check

Linux/Ubuntu (including WSL), CMake and a C++17 compiler are supported.
Python 3.10+ is optional and enables the standalone protocol smoke test.

```sh
git clone https://github.com/martenkrohn/seven-wonders-engine.git
cd seven-wonders-engine
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 scripts/audit_simulator.py --check
python3 scripts/smoke_protocol.py --engine build/7Wonders
```

The smoke client plays a complete match in a temporary directory using only
Python's standard library. To connect your own client, create `io/` and `logs/`
in its working directory, then start the executable there:

```sh
/absolute/path/to/build/7Wonders --seed 42 --poll-us 1000 --quiet
```

See the [protocol](docs/PROTOCOL.md) for the handshake, action schema, final
status and isolation requirements. Independent matches need separate working
directories. Identical seeds are reproducible within the same toolchain;
cross-platform shuffle equivalence is not guaranteed.

## Optional graphical client

The original Tk interface remains in this repository for the retained legacy
release. It requires protocol 2 and explicitly rejects the current development
binary. Its migration follows in package 7; use the standalone headless client
above for protocol 3. For the restored legacy GUI, use a separate checkout of
`v0.1.1-legacy`; the current RL pin also remains on that release.

Python 3.10+, Tkinter and a display are required for that old GUI. Artwork is
external; missing images use labeled placeholders. See [GUI.md](docs/GUI.md).

## Development

The [migration work plan](docs/MIGRATION_PLAN.md) separates the remaining work
into bounded packages. Its [next-agent task](docs/NEXT_AGENT_TASK.md) covers
only the next package. The [GUI audit](docs/GUI_AUDIT.md) records the restored
baseline and the remaining interface/artwork work.

The [BGA target contract](data/bga-second-edition/README.md) now preserves all
80 checked card/age rows, 14 wonder sides, source checksums and the complete ID
mapping in this repository. Runtime migration has begun with science scoring;
the executable still uses the legacy card and wonder definitions.

Keep engine changes and C++ regressions here. The RL repository consumes an
exact engine commit as a Git submodule. Follow [DEVELOPMENT.md](docs/DEVELOPMENT.md)
for releases and the planned BGA migration. Known rule gaps are recorded in
[KNOWN_ISSUES.md](docs/KNOWN_ISSUES.md).

The inherited implementation and downstream changes are documented in
[ORIGIN.md](docs/ORIGIN.md) and [SIMULATOR_CHANGES.md](docs/SIMULATOR_CHANGES.md),
including a checked patch against upstream. No RL code, training results or
downloaded BGA artwork is included in the current source tree. Historical
upstream commits remain in the fork's history.

## License and attribution

GPL-3.0-only; the original [LICENSE](LICENSE) is retained. See [NOTICE](NOTICE)
for upstream attribution and the historical upstream README/license discrepancy.
Vendored nlohmann/json headers retain their original MIT notices. This is an
unofficial project, with no publisher or Board Game Arena affiliation.
