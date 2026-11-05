"""Shared GUI/RL transport. All actions bind offered physical-card options."""
import json
import math
from pathlib import Path
import subprocess
import tempfile
import time


class EngineError(RuntimeError):
    pass


def atomic_json(path, value):
    temporary = path.with_suffix(path.suffix + ".tmp")
    temporary.write_text(json.dumps(value, allow_nan=False), encoding="utf-8")
    temporary.replace(path)


class DecisionClient:
    def __init__(self, engine, seed=42, timeout=10, decision_timeout_ms=0, grace_ms=100):
        if not 0 <= seed < 2**64 or not math.isfinite(timeout) or timeout <= 0:
            raise ValueError("Invalid seed or exchange timeout")
        self._temporary = tempfile.TemporaryDirectory(prefix="sevenwonders-decision-")
        self.root = Path(self._temporary.name)
        self.exchange = self.root / "io"
        self.exchange.mkdir()
        (self.root / "logs").mkdir()
        self._errors = (self.root / "stderr.log").open("w+")
        self.timeout, self.sequence, self.state = timeout, 0, None
        self.pending, self.expected_batch, self.closed = True, None, False
        self.deadline = time.monotonic() + timeout
        try:
            self.process = subprocess.Popen([str(Path(engine).resolve()), "--quiet", "--seed", str(seed),
                "--poll-us", "1000", "--decision-timeout-ms", str(decision_timeout_ms),
                "--terminal-grace-ms", str(grace_ms)], cwd=self.root,
                stdout=subprocess.DEVNULL, stderr=self._errors)
        except Exception:
            self._errors.close()
            self._temporary.cleanup()
            raise

    def poll(self):
        if self.closed:
            raise EngineError("Session is closed")
        path = self.exchange / "game_status.json"
        if path.exists():
            state = json.loads(path.read_text(encoding="utf-8"))
            if state.get("protocol_version") != 3 or state.get("contract_id") != "decision-v1":
                raise EngineError("Engine requires decision-v1 / protocol 3")
            if state.get("engine", {}).get("ruleset") not in {
                    "bga-second-edition-development", "bga-second-edition"}:
                raise EngineError("Engine ruleset is incompatible")
            if self.state is not None and state["game_id"] != self.state["game_id"]:
                raise EngineError("Engine game identity changed")
            acknowledged = self.expected_batch is None or (
                state.get("transport") is not None
                and state["transport"]["batch_id"] == self.expected_batch
                and not (self.exchange / "ready.json").exists())
            if acknowledged and (self.pending or state != self.state):
                self.state, self.pending = state, False
                return state
        if self.process.poll() is not None:
            if self.state and self.state["game"]["phase"] in {"finished", "aborted"}:
                return None
            self._errors.seek(0)
            raise EngineError(f"Engine exited ({self.process.returncode}): {self._errors.read()}")
        if self.pending and time.monotonic() > self.deadline:
            raise EngineError("Engine exchange timed out")
        return None

    def wait(self):
        while True:
            result = self.poll()
            if result is not None:
                return result
            time.sleep(.002)

    def request(self, actor, option_id):
        if type(actor) is not int or actor not in range(3):
            raise ValueError("Invalid actor")
        if self.pending or self.state is None:
            raise ValueError("No settled offer is ready")
        for decision in self.state["decisions"]:
            if decision["actor"] != actor:
                continue
            for option in decision["options"]:
                if option["option_id"] == option_id and option["selectable"]:
                    self.sequence += 1
                    return {"protocol_version": 3, "game_id": self.state["game_id"],
                        "request_id": f"request-{self.sequence}", "actor": actor,
                        "decision_id": decision["decision_id"], "offered_revision": decision["offered_revision"],
                        "action": {"kind": option["kind"], "option_id": option_id,
                                   "card_instance_id": option["card"]["instance_id"]}}
        raise ValueError("Option is absent, disabled, stale or belongs to another seat")

    def send(self, requests):
        if self.pending:
            raise ValueError("An exchange is already pending")
        if not requests:
            raise ValueError("Empty request bundle")
        self.sequence += 1
        identity = f"batch-{self.sequence}"
        header = {"protocol_version": 3, "game_id": self.state["game_id"], "batch_id": identity}
        atomic_json(self.exchange / "commands.json", {**header, "requests": requests})
        atomic_json(self.exchange / "ready.json", header)
        self.expected_batch, self.pending = identity, True
        self.deadline = time.monotonic() + self.timeout

    def submit(self, choices):
        # Validate the complete bundle before publishing either file.
        requests = [self.request(actor, option) for actor, option in choices.items()]
        self.send(requests)
        return requests

    def abort(self, reason="client_closed"):
        if self.state is None or self.pending or self.state["game"]["phase"] in {"finished", "aborted"}:
            return
        self.sequence += 1
        self.send([{"protocol_version": 3, "game_id": self.state["game_id"],
            "request_id": f"control-{self.sequence}", "control": "abort_game", "reason": reason}])

    def close(self):
        if self.closed:
            return
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=2)
        self._errors.close()
        self._temporary.cleanup()
        self.closed = True

    def __enter__(self):
        self.wait()
        return self

    def __exit__(self, *_):
        self.close()
