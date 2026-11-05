"""Resolve optional original artwork. No downloaded/new-edition artwork is bundled."""
from pathlib import Path


def asset_path(directory, category, name, era=1):
    # The original image suffixes are inconsistent for Glassworks.
    if category == "cards":
        if name == "Glassworks" and era == 1:
            name += "2"
        elif name in ("Loom", "Press") and era == 2:
            name += "2"
    return Path(directory) / category / f"{name}.png"
