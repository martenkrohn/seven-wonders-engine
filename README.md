# Seven Wonders Engine

A standalone C++17 simulator for three-player **7 Wonders**, forked from
[DMAG/UFSM's 7Wonders](https://github.com/dmag-ufsm/7Wonders). It exposes a
JSON file protocol for bots and other clients. No RL framework is required.

**Current release: `v0.1.1-legacy`.** This restores the optional original Tk GUI
and preserves the simulator previously
used by [Seven Wonders RL](https://github.com/martenkrohn/seven-wonders-rl),
including its existing corrections and limitations. The second-edition/BGA
migration is planned; it is **not implemented** in this release. The versioned
[engine contract](engine-contract.json) identifies the ruleset and protocol.

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

The original Tk interface is available again. It supports local human seats
and simple companion bots, using the existing legacy rules:

```sh
python3 Game.py 2 --assets-dir /path/to/local/images --seed 42
```

Python 3.10+, Tkinter and a display are required. Artwork is optional and
remains external; missing images use labeled placeholders. No BGA artwork
is bundled. See [GUI.md](docs/GUI.md) for controls, all-human mode, setup and
known limitations. `v0.1.0-legacy` preserves the original extraction without
the GUI; the C++ game/build files are unchanged in `v0.1.1-legacy`.

## Development

The [migration work plan](docs/MIGRATION_PLAN.md) separates the remaining work
into bounded packages. Its [next-agent task](docs/NEXT_AGENT_TASK.md) covers
only the next package. The [GUI audit](docs/GUI_AUDIT.md) records the restored
baseline and the remaining interface/artwork work.

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
