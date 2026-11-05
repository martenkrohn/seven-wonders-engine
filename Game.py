#!/usr/bin/env python3
"""Launch the restored original Tk interface against the standalone legacy engine."""
import argparse
import json
import math
from pathlib import Path

from gui.session import GameSession, RULESET

ROOT = Path(__file__).resolve().parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("num_bots", type=int, nargs="?", default=0, choices=range(4),
                        help="0: three human seats; 2: one human with two simple bots; 3: observe")
    parser.add_argument("--engine", type=Path, default=ROOT / "build/7Wonders")
    parser.add_argument("--assets-dir", type=Path, default=ROOT / "images",
                        help="Local original-edition images directory containing cards/, wonders/, resources/")
    parser.add_argument("--seed", type=int, default=42)
    parser.add_argument("--timeout", type=float, default=10)
    args = parser.parse_args()
    if not 0 <= args.seed < 2**64 or not math.isfinite(args.timeout) or args.timeout <= 0:
        parser.error("seed must fit UINT64 and timeout must be positive")
    contract = json.loads((ROOT / "engine-contract.json").read_text())
    if contract["ruleset"] != RULESET:
        parser.error("This GUI is for the legacy ruleset; its edition adapter needs updating")
    if not args.engine.is_file():
        parser.error(f"Engine missing: {args.engine}; build it with CMake or pass --engine")
    try:
        import tkinter
        from gui.legacy_view import run
    except ImportError as error:
        parser.exit(1, f"Tkinter is required for the optional GUI: {error}\n")
    session = GameSession(args.engine, seed=args.seed, bots=args.num_bots, timeout=args.timeout)
    try:
        run(session, args.assets_dir.resolve())
    except tkinter.TclError as error:
        parser.exit(1, f"Could not open the Tk GUI: {error}\n")
    finally:
        session.close()


if __name__ == "__main__":
    main()
