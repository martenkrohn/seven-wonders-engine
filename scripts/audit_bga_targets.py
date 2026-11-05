#!/usr/bin/env python3
"""Audit the frozen migration target independently of the generated runtime tables.

Standard library only. All default paths are relative to this script, not cwd.
--workspace additionally checks the original, external comparison and inputs.
"""

import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "bga-second-edition"
RESOURCES = dict(W="wood", O="ore", C="clay", S="stone", L="loom",
                 G="glass", P="papyrus")
COLORS = dict(raw="brown", man="grey", civ="blue", com="yellow", mil="red",
              sci="green", gui="purple")
NAMES = {"Pawnshop": "Well", "Apothocary": "Apothecary",
         "Craftsmen Guild": "Craftsmens Guild", "Bazaar": "Bazar"}
NEW_IDS = {"ludus": 76, "castrum": 77, "decorators": 78}
CORRECTED_COSTS = {
    "town_hall_age3": dict(stone=2, ore=1, glass=1),
    "arena_age3": dict(stone=2, ore=1),
    "circus_age3": dict(stone=3, ore=1),
    "craftsmens_guild_age3": dict(stone=2, ore=2),
}


def require(condition, message):
    if not condition:
        raise ValueError(message)


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def enum_ids(path, namespace):
    text = re.sub(r"//[^\n]*|/\*.*?\*/", "", path.read_text(), flags=re.S)
    match = re.search(r"namespace\s+" + namespace + r"\s*\{\s*enum[^\{]*\{([^}]+)", text)
    require(match is not None, f"Missing {namespace} enum in {path}")
    names = [name.strip() for name in match[1].split(",") if name.strip()]
    require(all(re.fullmatch(r"\w+", name) for name in names),
            f"Unsupported enum syntax in {path}; review identity audit")
    return dict(enumerate(names))


def normalized_name(raw):
    return NAMES.get(raw["name"], raw["name"])


def card_key(raw):
    name = normalized_name(raw).lower().replace(" ", "_")
    return f"{name}_age{raw['age']}"


def cost(raw):
    result = dict(Counter(RESOURCES[r] for r in raw.get("cost", [])))
    if raw.get("coincost", 0):
        result["coins"] = raw["coincost"]
    return result


def card_effects(raw):
    # Same public vocabulary as the reviewed comparison, checked independently
    # against the retained BGA definitions. Unknown values fail rather than default.
    if "ressmode" in raw:
        if raw["ressmode"] == "or":
            return dict(choice=sorted({RESOURCES[r] for r in raw["ress"]}),
                        tradable=raw["category"] in {"raw", "man"})
        require(raw["ressmode"] == "all", "Unknown production mode")
        return dict(production=dict(Counter(RESOURCES[r] for r in raw["ress"])),
                    tradable=raw["category"] in {"raw", "man"})
    if "discount" in raw:
        d = raw["discount"]
        return dict(discount="raw_" + d["direction"] if d["type"] == "raw"
                    else "manufactured_both")
    if raw.get("shield"):
        return dict(shields=raw["shield"])
    if "science" in raw:
        return dict(science={"1": "tablet", "2": "gear", "3": "compass",
                             "?": "joker"}[str(raw["science"])])
    result = {}
    counts = dict(raw="brown", man="grey", civ="blue", com="yellow", mil="red",
                  sci="green", rawmangui="brown_grey_purple", wonder="wonder_stages")
    directions = dict(all="self_and_neighbors", both="neighbors", none="self")
    for field, label in (("points", "vp"), ("coins", "coins")):
        if field not in raw:
            continue
        value = raw[field]
        if value["type"] in {"simple", "decorators"}:
            result[label] = value["qt"]
            if value["type"] == "decorators":
                result["condition"] = "wonder_complete"
        else:
            result[label + "_per"] = value["qt"]
            result["count"] = counts[value["type"]]
            result["target"] = directions[value["direction"]]
            if value["type"] in {"com", "rawmangui"} and value["direction"] == "none":
                result["includes_self"] = True
    return result


def stage(raw):
    result = dict(cost=cost(raw), vp=raw.get("points", {}).get("qt", 0), effects=[])
    effects = result["effects"]
    for field, kind in (("shield", "add_shields"), ("coins", "add_coins")):
        if raw.get(field):
            amount = raw[field]["qt"] if field == "coins" else raw[field]
            effects.append(dict(type=kind, amount=amount))
    if "science" in raw:
        require(raw["science"] == "?", "Unexpected wonder science")
        effects.append(dict(type="add_science"))
    if "ressmode" in raw:
        require(raw["ressmode"] == "or", "Unexpected wonder production")
        choices = {frozenset("CWOS"): "choose_raw_nontradable",
                   frozenset("GLP"): "choose_manuf_nontradable"}
        effects.append(dict(type=choices[frozenset(raw["ress"])]))
    for flag, kind in (("pickDiscarded", "discard_build_for_free"),
                       ("sevenCard", "play_seventh_card"),
                       ("buildFirstEachColor", "first_color_free"),
                       ("buildFirst", "first_age_card_free"),
                       ("buildLast", "last_age_card_free")):
        if raw.get(flag):
            effects.append(dict(type=kind))
    return result


def sprite(raw, wonder=False):
    index = raw if wonder else raw["img"]
    return dict(file="img/boards_v2.jpg" if wonder else "img/cards_v2.jpg",
                x=0 if wonder else index % 10 * 255,
                y=index * 223 if wonder else index // 10 * 393,
                width=450 if wonder else 255, height=223 if wonder else 393)


def validate_package(directory=DATA, engine_root=ROOT, workspace=None):
    target = read_json(directory / "targets.json")
    provenance = read_json(directory / "provenance.json")
    require(target["format_version"] == provenance["format_version"] == 1,
            "Unsupported target schema")
    require(target["ruleset"] == "bga-second-edition-development"
            and target["second_edition_implemented"] is False,
            "Target must remain an unimplemented development ruleset")
    for name in ("card_types.json", "wonders.json", "manifest.json"):
        require(sha256(directory / "reference" / name) == provenance["bga_input_sha256"][name],
                f"Frozen input checksum mismatch: {name}")
    raw_cards = read_json(directory / "reference/card_types.json")
    raw_wonders = read_json(directory / "reference/wonders.json")
    manifest = read_json(directory / "reference/manifest.json")
    require(manifest["data_edition"] == "second edition (game_edition=1)", "Wrong BGA edition")
    cards, wonders = target["cards"], target["wonders"]
    require(len(cards) == len(raw_cards) == 80 and len(wonders) == len(raw_wonders) == 14,
            "Incomplete inventory: expected 80 cards and 14 wonder sides")
    require(len({r["key"] for r in cards + wonders}) == 94, "Duplicate target key")
    require({r["bga_id"] for r in cards} == set(range(1, 81)), "Duplicate/missing BGA card ID")
    require({r["bga_id"] for r in wonders} == set(range(1, 15)), "Duplicate/missing BGA wonder ID")
    legacy = target["legacy_card_ids"]
    require(len(legacy) == 76 and [r["id"] for r in legacy] == list(range(76)),
            "Incomplete legacy identity inventory")
    card_ids = enum_ids(engine_root / "include/card.h", "CARD_ID")
    for entry in legacy:
        require(card_ids.get(entry["id"]) == entry["symbol"], "Legacy card ID changed")
    for symbol, identity in NEW_IDS.items():
        require(card_ids.get(identity) == symbol,
                f"Appended ID {identity} reused")
    # Historical constructor inputs are checksum-bound in provenance.json and
    # optionally checked by --workspace. Current main.cpp uses generated tables.
    by_key = {r["key"]: r for r in cards}
    used_ids = {}
    for row in cards:
        raw = raw_cards[str(row["bga_id"])]
        key = row["key"]
        require(key == card_key(raw) and row["name"] == normalized_name(raw), f"Name/age mapping: {key}")
        require(row["age"] == raw["age"] and row["color"] == COLORS[raw["category"]], f"Category/age: {key}")
        require(row["bga_name"] == raw["name"], f"Source name: {key}")
        require(row["cost"] == cost(raw), f"Cost: {key}")
        require(row["effects"] == card_effects(raw), f"Effects: {key}")
        quantities = {str(n): raw["qt"][str(n)] for n in range(3, 8)}
        require(row["counts_by_players"] == quantities, f"Deck quantities: {key}")
        marks, previous = [], 0
        for n in range(3, 8):
            quantity = quantities[str(n)]
            require(type(quantity) is int and quantity >= previous, f"Invalid deck quantity: {key}")
            marks.extend([n] * (quantity - previous))
            previous = quantity
        require(row["copies"] == marks, f"Copy thresholds: {key}")
        for field, raw_field in (("chain_from", "freeif"), ("chain_to", "canbuild")):
            expected = sorted(card_key(raw_cards[str(i)]) for i in raw[raw_field])
            require(row[field] == expected, f"Chain mapping: {key} {field}")
        for prerequisite in row["chain_from"]:
            require(prerequisite in by_key and by_key[prerequisite]["age"] < row["age"],
                    f"Missing/late chain predecessor: {key}")
            require(key in by_key[prerequisite]["chain_to"], f"Nonreciprocal chain: {key}")
        for successor in row["chain_to"]:
            require(successor in by_key and key in by_key[successor]["chain_from"], f"Nonreciprocal chain: {key}")
        require(row["sprite_index"] == raw["img"] and row["sprite"] == sprite(raw), f"Sprite: {key}")
        require(0 <= row["sprite_index"] < 80, f"Sprite out of bounds: {key}")
        identity, symbol = row["engine_id"], row["engine_symbol"]
        if symbol in NEW_IDS:
            expected_key = {"ludus": "ludus_age3", "castrum": "castrum_age3",
                            "decorators": "decorators_guild_age3"}[symbol]
            require(key == expected_key and identity == NEW_IDS[symbol]
                    and row["legacy_engine_id"] is None and row["legacy_name"] is None,
                    f"New ID allocation: {key}")
        else:
            require(identity == row["legacy_engine_id"] and identity not in (0, 73)
                    and legacy[identity]["symbol"] == symbol, f"Legacy mapping: {key}")
            expected_legacy_name = {"Well": "Pawnshop", "Craftsmens Guild": "Craftmens Guild"}.get(row["name"], row["name"])
            require(row["legacy_name"] == expected_legacy_name, f"Legacy constructor name alias: {key}")
            old_name = NAMES.get(row["legacy_name"], row["legacy_name"])
            old_name = {"Craftmens Guild": "Craftsmens Guild"}.get(old_name, old_name)
            require(old_name == row["name"], f"Legacy name alias: {key}")
        if identity in used_ids:
            other = used_ids[identity]
            require(symbol == other["engine_symbol"] and row["color"] == other["color"] == "grey"
                    and {row["age"], other["age"]} == {1, 2}, f"Duplicate engine card ID: {key}")
        used_ids[identity] = row
    require(set(used_ids) == (set(range(1, 76)) - {73}) | set(NEW_IDS.values()), "Missing engine identity")
    require(target["retired_cards"] == [dict(engine_id=73, engine_symbol="strategists",
                                             legacy_name="Strategists Guild", reason="Absent from BGA second-edition base game; ID reserved")],
            "Strategists must remain reserved, not become Decorators")
    require(by_key["decorators_guild_age3"]["comparison_engine_id"] == 73,
            "Comparison position for Decorators must remain traceable")
    for key, expected in CORRECTED_COSTS.items():
        require(by_key[key]["cost"] == expected, f"Reviewed cost regressed: {key}")
    require(len(used_ids) == 77 and len({c["sprite_index"] for c in cards}) == 77,
            "Expected 77 logical card types and 77 sprite positions")
    for symbol in ("loom", "glassworks", "press"):
        rows = [r for r in cards if r["engine_symbol"] == symbol]
        require(len(rows) == 2 and {r["age"] for r in rows} == {1, 2}, f"Grey variants: {symbol}")
    guilds = [r for r in cards if r["color"] == "purple"]
    require(len(guilds) == 10 and all(r["age"] == 3 for r in guilds), "Wrong guild pool")
    require(target["deck_policy"] == dict(players_implemented=3, player_counts_reference=[3, 4, 5, 6, 7],
                                         guild_pool_size=10, guilds_selected="players + 2",
                                         cards_per_player_per_age=7), "Wrong deck policy")
    for players in range(3, 8):
        for age in (1, 2, 3):
            total = sum(r["counts_by_players"][str(players)] for r in cards
                        if r["age"] == age and r["color"] != "purple")
            total += players + 2 if age == 3 else 0
            require(total == players * 7, f"Wrong deck size: {players} players, age {age}: {total}")
    wonder_ids = enum_ids(engine_root / "include/wonder.h", "WONDER_ID")
    require({r["engine_id"] for r in wonders} == set(range(14)), "Wonder identity coverage")
    for row in wonders:
        identity = row["engine_id"]
        raw = raw_wonders[str(row["bga_id"])]
        require(row["bga_id"] == identity + 1 and row["key"] == wonder_ids[identity], "Wonder mapping")
        require(row["engine_symbol"] == row["key"] and row["side"] == ("a" if identity < 7 else "b"), "Wonder side")
        require(row["bga_name"] == raw["name"], "Wonder source name")
        require(raw["ressmode"] == "all" and len(raw["ress"]) == 1, "Wonder starting production")
        require(row["starting_resource"] == RESOURCES[raw["ress"][0]], f"Wonder resource: {row['key']}")
        expected = [stage(raw["stages"][str(n)]) for n in range(1, len(raw["stages"]) + 1)]
        require(row["stages"] == expected, f"Wonder stages: {row['key']}")
        require(row["sprite_index"] == identity and row["sprite"] == sprite(identity, wonder=True), "Wonder sprite")
    require(provenance["source_discrepancies"] == [
        dict(key=key, name=by_key[key]["name"], field="cost",
             previous=previous, bga=expected,
             resolution="BGA definitions; differing card costs also checked on corresponding v2 sprite artwork")
        for key, previous, expected in (
            ("town_hall_age3", dict(stone=3, glass=1), CORRECTED_COSTS["town_hall_age3"]),
            ("arena_age3", dict(clay=2, ore=1), CORRECTED_COSTS["arena_age3"]),
            ("circus_age3", dict(clay=3, ore=1), CORRECTED_COSTS["circus_age3"]),
            ("craftsmens_guild_age3", dict(stone=2, ore=1), CORRECTED_COSTS["craftsmens_guild_age3"]))],
        "Reviewed source discrepancy inventory changed")
    if workspace is not None:
        verify_workspace(workspace, provenance, target)
    return dict(card_rows=len(cards), card_types=len(used_ids), wonder_sides=len(wonders),
                chain_edges=sum(len(r["chain_from"]) for r in cards),
                deck_sizes={str(n): [7 * n] * 3 for n in range(3, 8)},
                second_edition_implemented=target["second_edition_implemented"])


def verify_workspace(workspace, provenance, target):
    for relative, expected in provenance["external_input_sha256"].items():
        path = workspace / relative
        require(path.is_file(), f"Missing external input: {path}")
        require(sha256(path) == expected, f"External input checksum mismatch: {relative}")
    comparison = read_json(workspace / "docs/reference/edition-comparison/comparison.json")
    require(comparison["bga_input_sha256"] == provenance["bga_input_sha256"], "BGA provenance differs from comparison")
    require(comparison["source_discrepancies"] == provenance["source_discrepancies"], "Cost review provenance")
    cards = {r["key"]: r for r in target["cards"]}
    for row in comparison["cards"]:
        actual = cards[row["key"]]
        definition = {k: actual[k] for k in ("name", "cost", "effects", "copies")}
        for field in ("chain_from", "chain_to"):
            definition[field] = sorted(cards[k]["name"] for k in actual[field])
        require(definition == row["new"], f"Reviewed card target differs: {row['key']}")
        require(actual["comparison_engine_id"] == row["engine_id"] and actual["bga_id"] == row["bga_id"]
                and actual["sprite"] == row["new_sprite"], f"Reviewed card mapping differs: {row['key']}")
    wonders = {r["key"]: r for r in target["wonders"]}
    for row in comparison["wonders"]:
        actual = wonders[row["key"]]
        require({k: actual[k] for k in ("name", "starting_resource", "stages")} == row["new"],
                f"Reviewed wonder target differs: {row['key']}")
        require(actual["engine_id"] == row["engine_id"] and actual["bga_id"] == row["bga_id"]
                and actual["sprite"] == row["new_sprite"], f"Reviewed wonder mapping differs: {row['key']}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Read-only audit (also the default)")
    parser.add_argument("--data-dir", type=Path, default=DATA)
    parser.add_argument("--workspace", type=Path, help="Also verify the original RL workspace inputs")
    args = parser.parse_args()
    try:
        print(json.dumps(validate_package(args.data_dir, ROOT, args.workspace), indent=2))
    except (ValueError, KeyError, IndexError, TypeError, OSError) as error:
        print(f"BGA target audit failed: {error}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
