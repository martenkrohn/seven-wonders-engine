#!/usr/bin/env python3
"""Headless protocol-3 client; standard library only, no GUI or RL integration."""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import subprocess
import tempfile
import time


def atomic_write(path, value):
    temporary = path.with_suffix(path.suffix + '.tmp')
    temporary.write_text(value)
    temporary.replace(path)


class Client:
    def __init__(self, engine, seed=42, timeout_ms=0, grace_ms=100):
        self.temporary = tempfile.TemporaryDirectory(prefix='seven-wonders-v3-')
        self.root = Path(self.temporary.name)
        self.exchange = self.root / 'io'
        self.exchange.mkdir()
        (self.root / 'logs').mkdir()
        self.errors = (self.root / 'stderr.txt').open('w+')
        self.process = subprocess.Popen([str(Path(engine).resolve()), '--seed', str(seed),
            '--decision-timeout-ms', str(timeout_ms), '--terminal-grace-ms', str(grace_ms), '--quiet'],
            cwd=self.root, stdout=subprocess.DEVNULL, stderr=self.errors)
        self.sequence = 0
        self.state = self.wait(lambda s: True)
        if self.state.get('protocol_version') != 3 or self.state.get('contract_id') != 'decision-v1':
            self.close()
            raise RuntimeError('This headless client requires protocol 3 / decision-v1')

    def wait(self, predicate, seconds=10):
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            path = self.exchange / 'game_status.json'
            if path.exists():
                state = json.loads(path.read_text())
                if predicate(state):
                    self.state = state
                    return state
            if self.process.poll() is not None:
                self.errors.seek(0)
                raise RuntimeError(f'Engine exited {self.process.returncode}: {self.errors.read()}')
            time.sleep(.002)
        raise TimeoutError('No matching protocol-3 status before client timeout')

    def request(self, decision, option):
        self.sequence += 1
        return {'protocol_version': 3, 'game_id': self.state['game_id'],
            'request_id': f'client-{self.sequence}', 'actor': decision['actor'],
            'decision_id': decision['decision_id'], 'offered_revision': decision['offered_revision'],
            'action': {'kind': option['kind'], 'option_id': option['option_id'],
                       'card_instance_id': option['card']['instance_id']}}

    def bundle(self, requests, batch_id=None):
        self.sequence += 1
        identity = batch_id or f'batch-{self.sequence}'
        header = {'protocol_version': 3, 'game_id': self.state['game_id'], 'batch_id': identity}
        commands = {**header, 'requests': requests}
        # Capture previous acknowledgement even when redelivering the same ID.
        atomic_write(self.exchange / 'commands.json', json.dumps(commands))
        atomic_write(self.exchange / 'ready.json', json.dumps(header))
        return self.wait(lambda s: s.get('transport') is not None
            and s['transport']['batch_id'] == identity and not (self.exchange / 'ready.json').exists())

    def close(self):
        if self.process.poll() is None:
            self.process.terminate()
            try:
                self.process.wait(timeout=2)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=2)
        self.errors.close()
        self.temporary.cleanup()


def play(engine, seed, prefer_wonders=True, terminal_replay=False):
    client = Client(engine, seed, grace_ms=1000 if terminal_replay else 100)
    decisions = 0
    discard_decisions = 0
    turns = set()
    last_requests = []
    try:
        for _ in range(100):
            state = client.state
            turns.add(state['game']['turn'])
            physical = [card['instance_id'] for player in state['players'].values()
                        for field in ('hand_cards', 'built_cards') for card in player[field]]
            expected_cards = 21 * min(state['game']['age'], 3)
            represented = len(physical) + state['discard_count'] + sum(p['wonder_stage'] for p in state['players'].values())
            if len(physical) != len(set(physical)) or represented != expected_cards:
                raise RuntimeError('Physical card conservation/identity violated')
            if state['game']['phase'] == 'aborted':
                raise RuntimeError(f"Game aborted: {state.get('abort_reason')}")
            if state['game']['phase'] == 'finished':
                if state['game']['turn'] != 21 or not state['game']['scores_final']:
                    raise RuntimeError('Invalid terminal status')
                for seat in ('0', '1', '2'):
                    if state['result']['totals'][seat] != state['players'][seat]['points']['total']:
                        raise RuntimeError('Result differs from published player total')
                results = (client.root / 'results.txt')
                deadline = time.monotonic()+5
                while not results.exists() and time.monotonic()<deadline:
                    time.sleep(.001)
                expected = ''.join(f"Player {int(s)+1} score: {state['result']['totals'][s]}\n" for s in ('0','1','2'))
                if terminal_replay:
                    written = results.stat().st_mtime_ns
                    client.bundle([last_requests[0]])
                    if (client.state['revision'], client.state['events'], client.state['result']) != (
                            state['revision'], state['events'], state['result']):
                        raise RuntimeError('Terminal replay repeated resolution')
                    if results.stat().st_mtime_ns != written:
                        raise RuntimeError('Terminal replay rewrote results file')
                if results.read_text() != expected or client.process.wait(timeout=5) != 0:
                    raise RuntimeError('Final result/process mismatch')
                return {'seed': seed, 'decisions': decisions, 'discard_decisions': discard_decisions,
                        'turn_slots': sorted(turns), 'scores': [state['result']['totals'][str(s)] for s in range(3)]}
            if (client.root / 'results.txt').exists():
                raise RuntimeError('Results file appeared before all decisions finished')
            if state['waiting_for'] != [d['actor'] for d in state['decisions']]:
                raise RuntimeError('Waiting seats and decisions disagree')
            requests = []
            for decision in state['decisions']:
                legal = [o for o in decision['options'] if o['selectable']]
                order = ('build_discard_free', 'build_wonder', 'build_structure', 'discard') if prefer_wonders else (
                    'build_discard_free', 'build_structure', 'build_wonder', 'discard')
                option = next(o for kind in order for o in legal if o['kind'] == kind)
                requests.append(client.request(decision, option))
                decisions += 1
                discard_decisions += decision['kind'] == 'discard'
            if not requests:
                raise RuntimeError('No actionable offer in nonterminal stable status')
            client.bundle(requests)
            last_requests = requests
            for request in requests:
                receipt = next(r for r in client.state['receipts'] if r['request_id'] == request['request_id'])
                if receipt['status'] != 'applied':
                    raise RuntimeError(f'Offered action was not applied: {receipt}')
        raise RuntimeError('Game exceeded bounded base-game decision count')
    finally:
        client.close()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, required=True)
    parser.add_argument('--seed', type=int, default=42)
    args = parser.parse_args()
    print(json.dumps(play(args.engine, args.seed), indent=2))


if __name__ == '__main__':
    main()
