#!/usr/bin/env python3
"""Standalone acceptance games; no RL dependencies, artwork or runtime JSON."""
import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
from scripts.smoke_protocol import Client, play


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=ROOT / 'build/7Wonders')
    parser.add_argument('--games', type=int, default=60)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.games < 2:
        parser.error('At least two games are required')
    contract = json.loads((ROOT / 'engine-contract.json').read_text())
    client = Client(args.engine, seed=0)
    try:
        metadata = client.state['engine']
        for key in ('release', 'ruleset', 'second_edition_implemented'):
            if metadata[key] != contract[key]:
                raise RuntimeError('Binary/contract mismatch: ' + key)
        if client.state['protocol_version'] != 3 or not metadata['second_edition_implemented']:
            raise RuntimeError('Accepted BGA protocol-3 binary required')
    finally:
        client.close()
    games = [play(args.engine, seed, prefer_wonders=seed % 2 == 0)
             for seed in range(args.games)]
    replay = play(args.engine, 0, terminal_replay=True)
    if replay != games[0]:
        raise RuntimeError('Seed replay differs')
    result = {'engine': metadata, 'games': games, 'same_seed_and_terminal_replay': True,
              'checks': ['physical conservation and unique card instances', 'applied choices',
                         'extra-decision barriers', 'terminal totals/result agreement',
                         'normal exit', 'terminal replay does not repeat effects or result writes']}
    encoded = json.dumps(result, indent=2, allow_nan=False) + '\n'
    if args.output:
        args.output.write_text(encoded)
    print(json.dumps({'games': len(games),
                      'decisions': sum(g['decisions'] for g in games),
                      'discard_decisions': sum(g['discard_decisions'] for g in games),
                      'replay': True}))


if __name__ == '__main__':
    main()
