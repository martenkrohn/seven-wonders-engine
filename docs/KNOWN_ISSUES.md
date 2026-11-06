# Scope and remaining limitations

The accepted ruleset is `bga-second-edition`, protocol 3 / `decision-v1`,
release `v0.2.0-bga`. All target card effects and 14 wonder sides are active;
GUI and RL consume offered decisions. See [release acceptance](RELEASE_BGA.md).

- Exactly three players and the second-edition base game are supported.
  Four-to-seven-player quantities are preserved reference data, not a supported API.
  Expansions, promos and two-player rules are outside this release.
- The source is the frozen collected BGA definitions, not BGA server code.
  The user settled both special questions: no retroactive ordinary coin payout
  after discard construction, and no voluntary decline of a legal discard build.
  These are documented project decisions, not independently verified BGA timing.
- Payment allocation is canonical and minimal; clients do not select among
  equally priced trade allocations. Production is independently available to
  each buyer. East Trading Post applies right, West left, in public seat geometry.
- Local file transport assumes a trusted coordinator. It receives all hands;
  the GUI hides handover hands and RL policies receive only own/public features.
  There is no remote authentication or persistent restoration of a live game.
- Images are external and optional. Tk needs a display; missing or unverified
  artwork uses current text values. No downloaded artwork is distributed.
- Same-seed replay is checked within one toolchain. Cross-platform shuffle
  equivalence and learned playing strength are not claimed.
- Legacy protocol-2 tags, data and model contracts remain historical. Legacy
  guild-copy heuristics are inactive for all migrated wonder sides. Old models
  cannot initialize the new decision PPO without an explicit future migration.

The historical validation records describe their own commits. Their previously
open card/wonder/consumer items were closed by packages 5–9; frozen checksums
are not rewritten to make old reports describe a new implementation.
