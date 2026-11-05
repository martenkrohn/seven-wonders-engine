"""Protocol-v2 coordinator for the legacy GUI, independent of Tk and RL."""
from __future__ import annotations

import json
from pathlib import Path
import subprocess
import tempfile
import time

RULESET = "legacy-dmag-with-downstream-fixes"
ACTIONS = ("build_structure", "build_wonder", "discard", "build_hand_free")


class SessionError(RuntimeError):
    pass


def needs_action(state, seat):
    """Legacy Babylon B gains its seventh card at stage 2 (engine wonder ID 8)."""
    if state["game"]["phase"] == "finished":
        return False
    player = state["players"][str(seat)]
    return state["game"]["turn"] % 7 != 6 or (
        player["wonder_id"] == 8 and player["wonder_stage"] >= 2)


def legal_actions(player, card):
    if card not in player["cards_hand"]:
        return ()
    return tuple(action for action, enabled in (
        ("build_structure", card in player["cards_playable"]),
        ("build_wonder", player["can_build_wonder"]),
        ("discard", True),
        ("build_hand_free", player["can_build_hand_free"] and card not in player["cards_played"])) if enabled)


def default_action(player):
    """Simple deterministic companion, not an RL agent or strength benchmark."""
    playable = player["cards_playable"]
    return ("build_structure", playable[0]) if playable else ("discard", player["cards_hand"][0])


def atomic_write(path, text):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(text)
    temporary.replace(path)


class GameSession:
    def __init__(self, engine, *, seed=42, bots=0, timeout=10):
        if bots not in range(4):
            raise ValueError("bots must be between 0 and 3")
        self.engine = Path(engine).resolve()
        self.seed, self.bots, self.timeout = seed, bots, timeout
        self.state = None
        self.process = None
        self._temporary = None
        self._errors = None
        self.expected_turn = 0
        self.pending = True
        self.name_to_id = {}
        self.played_era = {}

    def start(self):
        if self.process is not None:
            raise SessionError("This session has already started")
        self._temporary = tempfile.TemporaryDirectory(prefix="seven-wonders-gui-")
        root = Path(self._temporary.name)
        self.runtime = root / "runtime"
        self.exchange = self.runtime / "io"
        for folder in (self.exchange, self.runtime / "logs", root / "logs"):
            folder.mkdir(parents=True, exist_ok=True)
        self._errors = (self.runtime / "stderr.txt").open("w+")
        self.deadline = time.monotonic() + self.timeout
        try:
            self.process = subprocess.Popen(
                [str(self.engine), "--seed", str(self.seed), "--poll-us", "1000", "--quiet"],
                cwd=self.runtime, stdout=subprocess.DEVNULL, stderr=self._errors)
        except OSError as error:
            self.close()
            raise SessionError(f"Could not start {self.engine}: {error}") from error

    def _failure(self, message):
        self._errors.flush()
        self._errors.seek(0)
        return SessionError(f"{message}\n{self._errors.read()[-4000:]}")

    def poll(self):
        """Return a new state, or None while waiting; call from Tk's after()."""
        if not self.pending:
            return None
        path = self.exchange / "game_status.json"
        try:
            state = json.loads(path.read_text()) if path.exists() else None
        except (OSError, ValueError) as error:
            raise SessionError(f"Invalid engine status: {error}") from error
        if state is not None:
            if state.get("protocol_version") != 2:
                raise SessionError("The legacy GUI requires protocol v2")
            turn = state["game"]["turn"]
            if turn == self.expected_turn:
                played_age = self.state["game"]["era"] if self.state else state["game"]["era"]
                for seat, player in state["players"].items():
                    for card in player["cards_played"]:
                        self.played_era.setdefault((int(seat), card), played_age)
                self.state, self.pending = state, False
                for player in state["players"].values():
                    self.name_to_id.update(zip(player["cards_hand"], player["cards_hand_id"]))
                return state
            if turn > self.expected_turn:
                raise SessionError(f"Unexpected engine turn: {turn} > {self.expected_turn}")
        # Read final publication before checking exit, to avoid losing the final state.
        if self.process.poll() is not None:
            raise self._failure(f"Engine exited before turn {self.expected_turn}")
        if time.monotonic() > self.deadline:
            raise self._failure(f"Timed out waiting for turn {self.expected_turn}")
        return None

    def submit(self, choices):
        if self.pending or self.state is None or self.state["game"]["phase"] == "finished":
            raise SessionError("No active turn is ready for actions")
        commands = []
        for seat in range(3):
            player = self.state["players"][str(seat)]
            if not needs_action(self.state, seat):
                # The engine acknowledges this legal placeholder as skipped.
                action, card = default_action(player)
            elif seat < self.bots:
                action, card = default_action(player)
            else:
                if seat not in choices:
                    raise ValueError(f"Player {seat + 1}: select a card and action")
                action, card = choices[seat]
            if action not in legal_actions(player, card):
                raise ValueError(f"Player {seat + 1}: {action} is unavailable for {card}")
            commands.append({"turn": self.state["game"]["turn"], "command": {
                "subcommand": action, "argument": card, "extra": ""}})
        for seat, command in enumerate(commands):
            atomic_write(self.exchange / f"player_{seat + 1}.json", json.dumps(command))
        # Signal readiness only after all three complete, validated action files exist.
        atomic_write(self.exchange / "ready.txt", "ready\n" * 3)
        self.expected_turn = self.state["game"]["turn"] + 1
        self.deadline = time.monotonic() + self.timeout
        self.pending = True

    def close(self):
        if self.process is not None and self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=2)
        if self._errors is not None:
            self._errors.close()
            self._errors = None
        if self._temporary is not None:
            self._temporary.cleanup()
            self._temporary = None
