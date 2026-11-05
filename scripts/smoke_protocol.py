#!/usr/bin/env python3
"""Play a complete protocol-v2 match without an RL/Python package dependency."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import tempfile
import time


def atomic_write(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(value)
    temporary.replace(path)


def play(engine, seed):
    history = []
    with tempfile.TemporaryDirectory(prefix="seven-wonders-smoke-") as temporary:
        root = Path(temporary)
        runtime = root / "runtime"
        for folder in (root / "logs", runtime / "logs", runtime / "io"):
            folder.mkdir(parents=True, exist_ok=True)
        exchange = runtime / "io"
        with (runtime / "stderr.txt").open("w+") as errors:
            process = subprocess.Popen(
                [str(engine), "--seed", str(seed), "--poll-us", "1000", "--quiet"],
                cwd=runtime, stdout=subprocess.DEVNULL, stderr=errors)
            try:
                for turn in range(22):
                    deadline = time.monotonic() + 10
                    while True:
                        status_path = exchange / "game_status.json"
                        state = json.loads(status_path.read_text()) if status_path.exists() else None
                        if state is not None and state["game"]["turn"] == turn:
                            break
                        if process.poll() is not None or time.monotonic() > deadline:
                            errors.seek(0)
                            raise RuntimeError(f"No status for turn {turn}; exit={process.poll()}: {errors.read()}")
                        time.sleep(0.001)
                    if state["protocol_version"] != 2 or state["game"]["engine_seed"] != seed:
                        raise RuntimeError("Protocol version or effective seed mismatch")
                    history.append(state)
                    if turn:
                        for seat in range(3):
                            acknowledgement = state["last_actions"][str(seat)]
                            if acknowledgement["status"] not in ("applied", "skipped"):
                                raise RuntimeError(f"Unexpected action acknowledgement: {acknowledgement}")
                    if turn == 21:
                        if state["game"]["phase"] != "finished" or state["game"]["era"] != 4:
                            raise RuntimeError("Missing scored final status")
                        break
                    for seat in range(3):
                        player = state["players"][str(seat)]
                        playable = player["cards_playable"]
                        command = "build_structure" if playable else "discard"
                        card = (playable or player["cards_hand"])[0]
                        action = {"turn": turn, "command": {
                            "subcommand": command, "argument": card, "extra": ""}}
                        atomic_write(exchange / f"player_{seat + 1}.json", json.dumps(action))
                    atomic_write(exchange / "ready.txt", "ready\n" * 3)
                if process.wait(timeout=10) != 0:
                    errors.seek(0)
                    raise RuntimeError(f"Engine failed: {errors.read()}")
            finally:
                if process.poll() is None:
                    process.kill()
                    process.wait()
    fingerprint = hashlib.sha256(json.dumps(history, sort_keys=True).encode()).hexdigest()
    return {"seed": seed, "states": len(history), "history_sha256": fingerprint,
            "scores": [history[-1]["players"][str(seat)]["points"]["total"] for seat in range(3)]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--engine", type=Path, required=True)
    parser.add_argument("--seed", type=int, default=42)
    args = parser.parse_args()
    print(json.dumps(play(args.engine.resolve(), args.seed), indent=2))


if __name__ == "__main__":
    main()
