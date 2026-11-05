"""Compare actual compiled card definitions/decks with the frozen target."""
from collections import Counter
import hashlib
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
ARTIFACTS = ("include/card_data.generated.h", "data/generated/cards-v1.json")
SOURCE = "data/bga-second-edition/targets.json"


class GeneratedCardTests(unittest.TestCase):
    def test_check_is_read_only_and_generation_is_deterministic(self):
        with tempfile.TemporaryDirectory(prefix="card-data-check-") as tmp:
            checkout = Path(tmp) / "checkout"
            for name in (*ARTIFACTS, SOURCE, "scripts/generate_card_data.py"):
                dest = checkout / name
                dest.parent.mkdir(parents=True, exist_ok=True)
                shutil.copy2(ROOT / name, dest)
            script = checkout / "scripts/generate_card_data.py"
            def run(*args):
                return subprocess.run([sys.executable, str(script), *args], cwd=tmp,
                                      capture_output=True, text=True, timeout=10)
            before = {name: ((checkout / name).read_bytes(), (checkout / name).stat().st_mtime_ns)
                      for name in ARTIFACTS}
            self.assertEqual(run("--check").returncode, 0)
            self.assertEqual(before, {name: ((checkout / name).read_bytes(), (checkout / name).stat().st_mtime_ns)
                                      for name in ARTIFACTS})
            for _ in range(2):
                for name in ARTIFACTS:
                    (checkout / name).unlink()
                result = run()
                self.assertEqual(result.returncode, 0, result.stderr)
                for name in ARTIFACTS:
                    self.assertEqual((checkout / name).read_bytes(), before[name][0])
            for name in ARTIFACTS:
                path = checkout / name
                path.write_bytes(path.read_bytes() + b"\n")
                corrupted = path.read_bytes(), path.stat().st_mtime_ns
                self.assertNotEqual(run("--check").returncode, 0)
                self.assertEqual(corrupted, (path.read_bytes(), path.stat().st_mtime_ns))
                path.write_bytes(before[name][0])

    def test_export_has_exact_target_rows_and_explicit_runtime_boundary(self):
        target = json.loads((ROOT / SOURCE).read_text())
        export = json.loads((ROOT / ARTIFACTS[1]).read_text())
        self.assertEqual(export["format_version"], 1)
        self.assertEqual(export["source"]["sha256"], hashlib.sha256((ROOT / SOURCE).read_bytes()).hexdigest())
        self.assertEqual(export["cards"], target["cards"])
        self.assertEqual(export["runtime_scope"]["deferred_effects"],
                         {"76": "ludus_count_effect", "78": "decorators_wonder_complete"})
        self.assertFalse(export["second_edition_implemented"])


class CompiledCardTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        binary = Path(os.environ.get("SEVENWONDERS_CARD_TEST", ROOT / "build/test_card_tables")).resolve()
        with tempfile.TemporaryDirectory(prefix="compiled-cards-") as tmp:
            completed = subprocess.run([str(binary), "--dump"], cwd=tmp, capture_output=True, text=True, timeout=20)
        if completed.returncode:
            raise AssertionError(completed.stderr)
        cls.actual = json.loads(completed.stdout)
        cls.target = json.loads((ROOT / SOURCE).read_text())

    def test_all_compiled_rows_match_frozen_target(self):
        self.assertEqual(self.actual["format_version"], 1)
        self.assertEqual(self.actual["source_sha256"], hashlib.sha256((ROOT / SOURCE).read_bytes()).hexdigest())
        expected = self.target["cards"]
        self.assertEqual(len(self.actual["cards"]), 80)
        for actual, target in zip(self.actual["cards"], expected):
            with self.subTest(card=target["key"]):
                actual = dict(actual)
                complete = actual.pop("runtime_effects_complete")
                self.assertEqual(complete, target["engine_id"] not in {76, 78})
                self.assertEqual(actual, {k: target[k] for k in actual})
        self.assertEqual(sum(len(c["chain_from"]) for c in self.actual["cards"]), 25)

    def test_actual_three_player_decks_and_physical_identities(self):
        guild_pool = {r["engine_id"] for r in self.target["cards"] if r["color"] == "purple"}
        seen_guilds = set()
        for seed, decks in enumerate(self.actual["decks"]):
            physical = []
            for age, deck in enumerate(decks, 1):
                with self.subTest(seed=seed, age=age):
                    self.assertEqual(len(deck), 21)
                    self.assertTrue(all(c["age"] == age for c in deck))
                    physical += [c["instance"] for c in deck]
                    actual = Counter(c["id"] for c in deck if c["id"] not in guild_pool)
                    expected = Counter({r["engine_id"]: r["counts_by_players"]["3"]
                                        for r in self.target["cards"] if r["age"] == age and r["color"] != "purple"
                                        and r["counts_by_players"]["3"]})
                    self.assertEqual(actual, expected)
                    guilds = [c["id"] for c in deck if c["id"] in guild_pool]
                    self.assertEqual(len(guilds), 5 if age == 3 else 0)
                    self.assertEqual(len(guilds), len(set(guilds)))
                    self.assertNotIn(73, [c["id"] for c in deck])
                    seen_guilds.update(guilds)
            self.assertEqual(len(physical), len(set(physical)))
            self.assertTrue(all(physical))
        self.assertEqual(seen_guilds, guild_pool)


if __name__ == "__main__":
    unittest.main()
