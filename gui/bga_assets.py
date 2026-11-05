"""Read verified BGA sprite rectangles; missing/old artwork becomes text."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
RESOURCE = {"wood": "Holz", "ore": "Erz", "clay": "Lehm", "stone": "Stein",
            "loom": "Stoff", "glass": "Glas", "papyrus": "Papyrus", "coins": "Münzen"}


class BgaAssets:
    def __init__(self, directory=None):
        self.directory = Path(directory) if directory is not None else ROOT / "images"
        cards = json.loads((ROOT / "data/generated/cards-v1.json").read_text())
        wonders = json.loads((ROOT / "data/generated/wonders-v1.json").read_text())
        self.source_hash = cards["source"]["sha256"]
        if wonders["source"]["sha256"] != self.source_hash:
            raise ValueError("Karten- und Wunderdaten passen nicht zusammen")
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

    def photo(self, kind, identity, age, master):
        key = kind, identity, age
        if key not in self.photos:
            spec = self.cards.get((identity, age)) if kind == "card" else self.wonders.get(identity)
            crop = self.crop(spec["sprite"]) if spec else None
            if crop is None:
                self.photos[key] = None
            else:
                from PIL import Image, ImageTk
                size = (153, 236) if kind == "card" else (300, 149)
                self.photos[key] = ImageTk.PhotoImage(crop.resize(size, Image.Resampling.LANCZOS), master=master)
        return self.photos[key]

    def card_text(self, identity, age):
        c = self.cards[(identity, age)]
        cost = ", ".join(f"{n} {RESOURCE[r]}" for r, n in c["cost"].items()) or "kostenlos"
        e = c["effects"]
        effect = []
        if "production" in e:
            effect.append("Produktion: " + ", ".join(f"{n} {RESOURCE[r]}" for r, n in e["production"].items()))
        if "choice" in e:
            effect.append("1 aus: " + "/".join(RESOURCE[r] for r in e["choice"]))
        if "vp" in e:
            effect.append(f"{e['vp']} Punkte" + (" bei vollständigem Wunder" if e.get("condition") else ""))
        if "coins" in e:
            effect.append(f"+{e['coins']} Münzen")
        if "shields" in e:
            effect.append(f"{e['shields']} Schilde")
        if "science" in e:
            effect.append("Wissenschaft: " + {"gear": "Zahnrad", "compass": "Zirkel", "tablet": "Tafel", "joker": "Joker"}[e["science"]])
        if "discount" in e:
            effect.append("Handelsrabatt: " + {"raw_right": "Rohstoffe rechts", "raw_left": "Rohstoffe links", "manufactured_both": "Manufakturwaren beidseits"}[e["discount"]])
        if "count" in e:
            what = {"brown": "braune Karte", "grey": "graue Karte", "yellow": "gelbe Karte", "green": "grüne Karte", "red": "rote Karte", "blue": "blaue Karte", "brown_grey_purple": "braune/graue/lila Karte", "wonder_stages": "Wunderstufe"}[e["count"]]
            where = {"self": "eigen", "neighbors": "beide Nachbarn", "self_and_neighbors": "eigene Stadt und beide Nachbarn"}[e["target"]]
            effect.append(" / ".join(f"{e[k]} {label}" for k, label in (("coins_per", "Münzen"), ("vp_per", "Punkte")) if k in e) + f" je {what} ({where})")
        return f"Kosten: {cost}\n" + "\n".join(effect)
