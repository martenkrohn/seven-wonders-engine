# Known rule and interface limitations

The tagged legacy release is the existing simulator with selected downstream
fixes. The current development tree adds optimal science scoring, simultaneous
resolution and generated card tables; neither
is a fully verified implementation of either printed edition.

- All 80 card-age table rows now match the frozen target, including Aqueduct,
  Town Hall, Arsenal and University. Ludus (76) and Decorators Guild (78) retain
  their target metadata but contribute no new coin/scoring effect until package
  5-II. Decorators is already in the sampled guild pool; scores can therefore
  omit its conditional seven points. Ludus has no copy in a three-player deck.
  The contract/status explicitly mark card effects incomplete. See [CARD_DATA.md](CARD_DATA.md).
- Science jokers are scored optimally in the development tree (package 3);
  tagged legacy releases retain their greedy scoring. The separate old
  `CopyGuild()` candidate selection still estimates Scientists Guild value
  from the number of scientific cards, not its actual marginal science score.
  With no scientific cards it can reject copying a joker worth one point.
  Other guild-copy selection/idempotency issues are outside the scoring fix.
- Protocol 3 now resolves frozen regular actions together and offers mandatory
  discard decisions after them. Old V2 preselection and sequential settlement
  are removed from the active path. The GUI and RL consumers still require
  protocol 2 and have not been migrated; the GUI explicitly rejects version 3.
- Wonder definitions and ability stages are still legacy, including Babylon
  and Olympia's once-per-age free build. The status labels that mode explicitly
  rather than advertising future first/sixth-card powers.
- Three players are hard-coded. There is no general two-to-seven-player API,
  payment-choice action space, or mid-game state restoration.

The RL repository contains the detailed evidence in
`docs/reference/EDITION_AUDIT.md` and the complete edition comparison. A
successful smoke game establishes protocol usability, not rule completeness.
See [SCIENCE_SCORING.md](SCIENCE_SCORING.md) for the tested scoring boundary
and the separate Python heuristic limitation before RL adoption.
Package 4a documents the future [decision/resolution contract](DECISION_PROTOCOL_DRAFT.md).
The user settled both timing/decline questions on 5 November 2026: no retroactive
ordinary coin payout after discard construction, and no voluntary decline when
a legal discard build exists. This is a project rule decision, not independent
BGA server verification. See [SIMULTANEOUS_RESOLUTION.md](SIMULTANEOUS_RESOLUTION.md)
for the implemented resolution boundary and current protocol-3 checks.
