# File protocol v2

The executable starts one three-player game. Arguments: `--seed UINT64`,
`--poll-us 0..1000000` (default 1000), and `--quiet`. Use one isolated working
directory per process. Create `io/`, `logs/` and `../logs/` first; inherited
match logging writes to `../logs/`. The smoke client demonstrates this layout.

1. Start the engine. Wait for `io/game_status.json` with `protocol_version=2`
   and `game.turn=0`.
2. Read the state and write all three `io/player_1.json` through
   `io/player_3.json`. Filenames are one-based; status player keys are
   zero-based (`"0"`, `"1"`, `"2"`). A command looks like:

   ```json
   {"turn":0,"command":{"subcommand":"discard","argument":"Lumber Yard","extra":""}}
   ```

   Commands are `build_structure`, `build_wonder`, `build_hand_free` and
   `discard`. The argument is the exact card name from `cards_hand`.
   `cards_playable` and the `can_build_*` flags describe available actions.
   `extra` is the inherited discard-pile selection string for Halikarnassos;
   that path has known limitations and is not exercised by the smoke client.
3. After all action files are closed, publish `io/ready.txt` with exactly
   three lines `ready\n`. The engine consumes and empties this signal.
4. Wait for a strictly larger `game.turn` before sending the next actions.
   Inspect `last_actions` for each seat's requested/executed command and
   `applied`, `skipped` or `fallback_discard` status. A fallback is not proof
   that the requested action was executed.
5. Stop submitting actions when `game.phase` is `finished`. The final state
   is turn 21, era 4, after military and guild scoring. Wait for process exit
   and read scores from `players[seat].points.total`.

Publish client files via a temporary sibling file and rename. The engine
publishes its status atomically. Apply timeouts and detect premature process
exit; a stale state is not a new transition. Seventh-card steps 6, 13 and 20
still require the three-seat handshake; seats without Babylon's ability are
acknowledged as skipped. Other status fields include the effective engine
seed, direction, wonder identity/stage, hands, played cards, production,
coins, shields, score categories and card counts.

This protocol exposes all hands to the coordinating client. It is not an
access-control boundary for competitive remote bots. There is no saved-game
restore API. Version 2 identifies the transport schema, not a board-game
edition; use `engine-contract.json` and the exact source commit as well.
