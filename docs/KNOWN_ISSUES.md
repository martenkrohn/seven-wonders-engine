# Known rule and interface limitations

The tagged legacy release is the existing simulator with selected downstream
fixes. The current development tree adds optimal science joker scoring; neither
is a fully verified implementation of either printed edition.

- Card declarations still match the pinned 2021 upstream. Known discrepancies
  include Aqueduct, Town Hall, Arsenal and University; second-edition card
  costs and wonder stages have not been migrated.
- Science jokers are scored optimally in the development tree (package 3);
  tagged legacy releases retain their greedy scoring. The separate old
  `CopyGuild()` candidate selection still estimates Scientists Guild value
  from the number of scientific cards, not its actual marginal science score.
  With no scientific cards it can reject copying a joker worth one point.
  Other guild-copy selection/idempotency issues are outside the scoring fix.
- Halikarnassos' discard selection through `extra` is incomplete and needs
  validation and correct discard-pile removal. The smoke client leaves it empty.
- Resolution remains sequential. Resetting selectable-production state between
  seats fixes one reproduced conflict, not every simultaneity/payment issue.
- Three players are hard-coded. There is no general two-to-seven-player API,
  payment-choice action space, or mid-game state restoration.

The RL repository contains the detailed evidence in
`docs/reference/EDITION_AUDIT.md` and the complete edition comparison. A
successful smoke game establishes protocol usability, not rule completeness.
See [SCIENCE_SCORING.md](SCIENCE_SCORING.md) for the tested scoring boundary
and the separate Python heuristic limitation before RL adoption.
Package 4a documents the future [decision/resolution contract](DECISION_PROTOCOL_DRAFT.md),
including two unresolved BGA timing/decline questions. It changes no runtime
behavior and does not close the discard or simultaneous-resolution limitations.
