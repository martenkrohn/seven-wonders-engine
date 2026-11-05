"""Migration contract regressions; these do not exercise migrated game rules."""

import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location("audit_bga_targets", ROOT / "scripts/audit_bga_targets.py")
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


class BgaTargetTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory(prefix="bga-target-test-")
        self.addCleanup(self.temp.cleanup)
        self.directory = Path(self.temp.name) / "data"
        shutil.copytree(AUDIT.DATA, self.directory)
        self.path = self.directory / "targets.json"
        self.target = AUDIT.read_json(self.path)

    def save(self):
        self.path.write_text(json.dumps(self.target), encoding="utf-8")

    def card(self, key):
        return next(r for r in self.target["cards"] if r["key"] == key)

    def rejected(self, message):
        self.save()
        with self.assertRaisesRegex(ValueError, message):
            AUDIT.validate_package(self.directory)

    def test_complete_inventory_and_known_new_effects(self):
        report = AUDIT.validate_package(self.directory)
        self.assertEqual((report["card_rows"], report["card_types"], report["wonder_sides"]), (80, 77, 14))
        self.assertEqual(self.card("ludus_age3")["effects"],
                         dict(coins_per=3, vp_per=1, count="red", target="self"))
        self.assertEqual(self.card("decorators_guild_age3")["effects"],
                         dict(vp=7, condition="wonder_complete"))
        self.assertEqual(self.card("altar_age1")["chain_to"], ["pantheon_age3"])
        self.assertEqual(self.card("forum_age2")["chain_from"],
                         ["east_trading_post_age1", "west_trading_post_age1"])

    def test_missing_row_and_duplicate_bga_id(self):
        removed = self.target["cards"].pop()
        self.rejected("Incomplete inventory")
        self.target["cards"].append(removed)
        removed["bga_id"] = 1
        self.rejected("Duplicate/missing BGA card ID")

    def test_strategists_id_cannot_be_reused_for_decorators(self):
        row = self.card("decorators_guild_age3")
        self.assertEqual((row["engine_id"], row["comparison_engine_id"]), (78, 73))
        row["engine_id"] = 73
        self.rejected("New ID allocation")

    def test_grey_variants_cannot_be_collapsed_or_swapped(self):
        first, second = self.card("glassworks_age1"), self.card("glassworks_age2")
        self.assertEqual(first["engine_id"], second["engine_id"])
        self.assertNotEqual(first["bga_id"], second["bga_id"])
        self.assertNotEqual(first["counts_by_players"], second["counts_by_players"])
        first["bga_id"], second["bga_id"] = second["bga_id"], first["bga_id"]
        self.rejected("Name/age mapping")

    def test_aliases_and_legacy_ids(self):
        self.assertEqual(self.card("well_age1")["legacy_name"], "Pawnshop")
        self.assertEqual(self.card("apothecary_age1")["bga_name"], "Apothocary")
        self.assertEqual(self.card("craftsmens_guild_age3")["bga_name"], "Craftsmen Guild")
        self.card("well_age1")["engine_id"] = 19
        self.rejected("Legacy mapping")

    def test_missing_and_wrong_chain_predecessors(self):
        row = self.card("forum_age2")
        for predecessors in (["east_trading_post_age1"], ["ludus_age3"], ["missing_age1"]):
            with self.subTest(predecessors=predecessors):
                row["chain_from"] = predecessors
                self.rejected("[Cc]hain")

    def test_legacy_name_alias_remains_historical_after_generator_migration(self):
        self.card("well_age1")["legacy_name"] = "Well"
        self.rejected("Legacy constructor name alias")

    def test_four_corrected_costs_cannot_revert_to_csv(self):
        old = {"town_hall_age3": dict(stone=3, glass=1), "arena_age3": dict(clay=2, ore=1),
               "circus_age3": dict(clay=3, ore=1), "craftsmens_guild_age3": dict(stone=2, ore=1)}
        for key, incorrect in old.items():
            with self.subTest(key=key):
                row = self.card(key)
                correct = row["cost"]
                row["cost"] = incorrect
                self.rejected("Cost")
                row["cost"] = correct

    def test_balanced_deck_change_is_rejected_even_with_same_total(self):
        self.card("lumber_yard_age1")["counts_by_players"]["3"] += 1
        self.card("stone_pit_age1")["counts_by_players"]["3"] -= 1
        self.rejected("Deck quantities")

    def test_wrong_sprite_and_wrong_babylon_stage_count(self):
        row = self.card("lumber_yard_age1")
        row["sprite_index"] = row["bga_id"]
        self.rejected("Sprite")
        row["sprite_index"] = 74
        babylon = next(r for r in self.target["wonders"] if r["key"] == "babylon_b")
        self.assertEqual(len(babylon["stages"]), 2)
        babylon["stages"].pop()
        self.rejected("Wonder stages")

    def test_frozen_source_tampering_fails_checksum(self):
        path = self.directory / "reference/card_types.json"
        path.write_bytes(path.read_bytes() + b"\n")
        with self.assertRaisesRegex(ValueError, "Frozen input checksum mismatch"):
            AUDIT.validate_package(self.directory)

    def test_audit_works_in_isolated_checkout_and_unrelated_cwd(self):
        checkout = Path(self.temp.name) / "engine"
        for relative in ("scripts/audit_bga_targets.py", "include/card.h", "include/wonder.h", "main.cpp"):
            destination = checkout / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(ROOT / relative, destination)
        shutil.copytree(AUDIT.DATA, checkout / "data/bga-second-edition")
        completed = subprocess.run([sys.executable, str(checkout / "scripts/audit_bga_targets.py"), "--check"],
                                   cwd=self.temp.name, capture_output=True, text=True, timeout=10)
        self.assertEqual(completed.returncode, 0, completed.stderr)
        self.assertEqual(json.loads(completed.stdout)["card_rows"], 80)


if __name__ == "__main__":
    unittest.main()
