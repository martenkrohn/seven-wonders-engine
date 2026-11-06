# Generated cards, wonders and runtime contract

`data/bga-second-edition/targets.json` is the maintained source for card and
wonder values. Its audit compares the frozen definitions and stable identities.
Targets, raw references and `provenance.json` keep their historical hashes and
development metadata. The current release metadata is `engine-contract.json`.

```sh
python3 scripts/audit_bga_targets.py --check
python3 scripts/generate_card_data.py
python3 scripts/generate_card_data.py --check
```

The standard-library generator emits five deterministic checked-in artifacts:

- `include/card_data.generated.h`: 80 card/age rows, 77 types, 25 chain links.
- `include/wonder_data.generated.h`: all 14 sides and actual stage counts.
- `data/generated/cards-v1.json` and `wonders-v1.json`: neutral version-1
  data with the frozen target hash and current runtime ruleset/acceptance flag.
- `include/engine_contract.generated.h`: release, ruleset and acceptance flag
  from the same operational contract used to produce the JSON exports.

`--check` never writes, including on mismatch. Paths bind to the checkout,
independent of cwd. C++ builds use the checked-in headers; neither generator,
RL, Python, images nor runtime JSON files are required by the binary.
Python consumers copy the neutral exports byte-for-byte as package resources.

`Card::FromDefinition(id, age)` is the strict real-deck factory. Grey age
variants stay separate. Only synthetic test construction permits custom costs
and chains; one-row types may supply fixture effects at an artificial age.

Each three-player age has 21 cards. Age III samples five distinct guilds from
ten; Decorators 78 replaces retired Strategists 73 without reusing its identity.
Ludus 76 and Castrum 77 have no ordinary copy at three players; their definitions
and effects are tested separately. Physical card IDs survive location changes.

Production, choice, trade eligibility, costs, chains, science, shields,
discounts, fixed and count effects all use these tables. Ludus pays three coins
on construction and scores one point per own red card. Decorators scores seven
only when the actual own wonder is complete, including later completion.
All effects report complete, with an empty deferred-ID list.

Each wonder stage uses its exact generated cost and effect. Babylon B has two
stages and grants its seventh card at the first; Olympia exposes new explicit
free modes; Halikarnassos produces mandatory discard decisions. New wonder
sides grant no old guild-copy power.

Native/compiled-data tests compare every row and stage independently with
frozen targets. Generator tests check two regenerations, read-only checking and
failure on corrupted artifacts from an isolated directory. Additional native
rule and protocol tests are required; data equality alone is not rule acceptance.
