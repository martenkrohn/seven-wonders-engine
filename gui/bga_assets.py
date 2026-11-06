"""Read verified BGA sprite rectangles; missing/old artwork becomes text."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESOURCE = {"wood": "Wood", "ore": "Ore", "clay": "Clay", "stone": "Stone",
            "loom": "Loom", "glass": "Glass", "papyrus": "Papyrus", "coins": "coins"}


class BgaAssets:
    def __init__(self, directory=None):
        self.directory = Path(directory) if directory is not None else ROOT / "images"
        cards = json.loads((ROOT / "data/generated/cards-v1.json").read_text())
        wonders = json.loads((ROOT / "data/generated/wonders-v1.json").read_text())
        self.source_hash = cards["source"]["sha256"]
        if wonders["source"]["sha256"] != self.source_hash:
            raise ValueError("Card and wonder data do not match")
        self.cards = {(c["engine_id"], c["age"]): c for c in cards["cards"]}
        self.wonders = {w["engine_id"]: w for w in wonders["wonders"]}
        manifest = json.loads((ROOT / "data/bga-second-edition/reference/manifest.json").read_text())
        self.hashes = {f["path"]: f["sha256"] for f in manifest["files"]}
        self.sheets, self.photos, self.missing = {}, {}, set()

    def crop(self, sprite):
        filename = sprite["file"]
        if filename not in self.sheets:
            try:
                from PIL import Image
                path = self.directory / filename
                if not path.is_file():
                    path = self.directory / Path(filename).name
                if hashlib.sha256(path.read_bytes()).hexdigest() != self.hashes[filename]:
                    raise ValueError("Unverified artwork")
                with Image.open(path) as image:
                    image.load()
                    self.sheets[filename] = image.convert("RGB")
            except (ImportError, OSError, ValueError):
                self.sheets[filename] = None
                self.missing.add(filename)
        sheet = self.sheets[filename]
        if sheet is None:
            return None
        x, y, width, height = (sprite[k] for k in ("x", "y", "width", "height"))
        if x < 0 or y < 0 or x+width > sheet.width or y+height > sheet.height:
            raise ValueError("Invalid sprite rectangle")
        return sheet.crop((x, y, x+width, y+height))

    def photo(self, kind, identity, age, master, size=None):
        size = size or {"card": (130, 200), "built": (122, 40),
                        "detail": (255, 393), "wonder": (300, 149)}[kind]
        key = kind, identity, age, size, master
        if key not in self.photos:
            spec = self.wonders.get(identity) if kind == "wonder" else self.cards.get((identity, age))
            crop = self.crop(spec["sprite"]) if spec else None
            if crop is not None and kind == "built":
                # The complete effect band, including the right-hand chain symbols.
                # Costs below it belong to construction, not to the built city.
                crop = crop.crop((0, 26, 255, 110))
            if crop is None:
                self.photos[key] = None
            else:
                from PIL import Image, ImageTk
                self.photos[key] = ImageTk.PhotoImage(crop.resize(size, Image.Resampling.LANCZOS), master=master)
        return self.photos[key]

    def icon(self, name, master):
        """Use the verified second-edition icon sheet, with a text fallback."""
        rectangles = {"coins": (0, 0, 30, 30), "total": (30, 0, 30, 30),
                      "wood": (210, 30, 30, 30), "stone": (210, 0, 30, 30),
                      "clay": (240, 0, 30, 30), "loom": (240, 30, 30, 30),
                      "ore": (270, 0, 30, 30), "glass": (270, 30, 30, 30),
                      "papyrus": (270, 60, 30, 30)}
        key = "icon", name, master
        if key not in self.photos:
            rectangle = rectangles.get(name)
            crop = self.crop(dict(zip(("file", "x", "y", "width", "height"),
                                     ("img/icons_v2.png", *rectangle)))) if rectangle else None
            if name == "shields":
                # Reuse the actual military emblem from the verified Stockade card.
                card = self.crop(self.cards[(43, 1)]["sprite"])
                crop = card.crop((96, 32, 160, 96)) if card is not None else None
            if crop is None:
                self.photos[key] = None
            else:
                from PIL import Image, ImageTk
                self.photos[key] = ImageTk.PhotoImage(crop.resize((21, 21), Image.Resampling.LANCZOS), master=master)
        return self.photos[key]

    def chain_text(self, identity, age):
        card = self.cards[(identity, age)]
        by_key = {c["key"]: c["name"] for c in self.cards.values()}
        lines = []
        for field, label in (("chain_from", "Free after"), ("chain_to", "Unlocks")):
            names = [by_key[key] for key in card[field]]
            if names:
                lines.append(label + ": " + ", ".join(names))
        return "\n".join(lines)

    def wonder_name(self, identity):
        """English display names without changing the engine's canonical data."""
        wonder = self.wonders[identity]
        family = wonder["key"].rsplit("_", 1)[0]
        names = {"gizah": "Giza", "babylon": "Babylon", "olympia": "Olympia",
                 "rhodos": "Rhodes", "ephesos": "Ephesus", "alexandria": "Alexandria",
                 "halikarnassos": "Halicarnassus"}
        return names[family] + " · " + ("Day" if wonder["side"] == "a" else "Night")

    def card_text(self, identity, age):
        c = self.cards[(identity, age)]
        cost = ", ".join(f"{n} {RESOURCE[r]}" for r, n in c["cost"].items()) or "free"
        e = c["effects"]
        effect = []
        if "production" in e:
            effect.append("Production: " + ", ".join(f"{n} {RESOURCE[r]}" for r, n in e["production"].items()))
        if "choice" in e:
            effect.append("Choose 1: " + "/".join(RESOURCE[r] for r in e["choice"]))
        if "vp" in e:
            effect.append(f"{e['vp']} points" + (" if your wonder is complete" if e.get("condition") else ""))
        if "coins" in e:
            effect.append(f"+{e['coins']} coins")
        if "shields" in e:
            effect.append(f"{e['shields']} shields")
        if "science" in e:
            effect.append("Science: " + {"gear": "Gear", "compass": "Compass", "tablet": "Tablet", "joker": "Joker"}[e["science"]])
        if "discount" in e:
            effect.append("Trade discount: " + {"raw_right": "Raw materials from right neighbor", "raw_left": "Raw materials from left neighbor", "manufactured_both": "Manufactured goods from both neighbors"}[e["discount"]])
        if "count" in e:
            what = {"brown": "brown card", "grey": "gray card", "yellow": "yellow card", "green": "green card", "red": "red card", "blue": "blue card", "brown_grey_purple": "brown/gray/purple card", "wonder_stages": "wonder stage"}[e["count"]]
            where = {"self": "your city", "neighbors": "both neighbors", "self_and_neighbors": "your city and both neighbors"}[e["target"]]
            effect.append(" / ".join(f"{e[k]} {label}" for k, label in (("coins_per", "coins"), ("vp_per", "points")) if k in e) + f" per {what} ({where})")
        return f"Cost: {cost}\n" + "\n".join(effect)
