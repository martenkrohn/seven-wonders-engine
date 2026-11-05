"""Decision-based local human/bot session; rendering is separate."""
from sevenwonders_engine import DecisionClient


class DecisionSession:
    def __init__(self, engine, seed=42, bots=0, timeout=10):
        if bots not in range(4):
            raise ValueError("bots must be 0..3")
        self.bots = bots
        self.client = DecisionClient(engine, seed, timeout)

    @property
    def state(self):
        return self.client.state

    @property
    def pending(self):
        return self.client.pending

    def poll(self):
        return self.client.poll()

    def human_decision(self):
        if self.pending or not self.state:
            return None
        return next((d for d in self.state["decisions"] if d["actor"] >= self.bots), None)

    def submit_choice(self, actor, option_id):
        if actor < self.bots:
            raise ValueError("This seat is controlled by a bot")
        return self.client.submit({actor: option_id})

    def drive_bots(self):
        if self.pending or not self.state:
            return False
        choices = {}
        for d in self.state["decisions"]:
            if d["actor"] >= self.bots:
                continue
            legal = [o for o in d["options"] if o["selectable"]]
            option = next(o for kind in ("build_discard_free", "build_wonder", "build_structure", "discard")
                          for o in legal if o["kind"] == kind)
            choices[d["actor"]] = option["option_id"]
        if choices:
            self.client.submit(choices)
        return bool(choices)

    def close(self):
        self.client.close()
