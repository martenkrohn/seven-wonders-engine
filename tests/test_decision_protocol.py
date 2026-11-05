"""Real process/file transport checks for protocol 3; standard library only."""
import json
import os
from pathlib import Path
import sys
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
ENGINE = Path(os.environ.get('SEVENWONDERS_TEST_ENGINE', ROOT / 'build/7Wonders'))
sys.path.insert(0, str(ROOT))
from scripts.smoke_protocol import Client, atomic_write, play


@unittest.skipUnless(ENGINE.is_file(), 'Build engine first')
class DecisionProtocolTests(unittest.TestCase):
    def client(self, **kw):
        c = Client(ENGINE, **kw)
        self.addCleanup(c.close)
        return c

    def action(self, c, actor=0):
        d = next(d for d in c.state['decisions'] if d['actor'] == actor)
        return c.request(d, next(o for o in d['options'] if o['kind'] == 'discard'))

    def receipt(self, c, request):
        for reply in c.state.get('transport', {}).get('responses', []):
            if reply['request_id'] == request['request_id'] and reply['status'] == 'rejected':
                return reply
        return next(r for r in reversed(c.state['receipts']) if r['request_id'] == request['request_id'])

    def abort(self, c, reason='test_complete'):
        request = {'protocol_version': 3, 'game_id': c.state['game_id'],
                   'request_id': 'abort', 'control': 'abort_game', 'reason': reason}
        c.bundle([request])
        self.assertEqual(c.state['game']['phase'], 'aborted')
        self.assertIsNone(c.state['result'])
        self.assertFalse((c.root / 'results.txt').exists())
        self.assertEqual(c.process.wait(timeout=5), 2)

    def test_partial_receipts_replay_and_rejected_input(self):
        c = self.client()
        original = c.state
        request = self.action(c)
        c.bundle([request])
        self.assertEqual(self.receipt(c, request)['status'], 'accepted')
        self.assertEqual(c.state['players'], original['players'])
        self.assertEqual(c.state['waiting_for'], [1, 2])
        revision = c.state['revision']
        c.bundle([request])
        self.assertEqual(c.state['revision'], revision)
        conflict = json.loads(json.dumps(request))
        conflict['action']['card_instance_id'] = 'other'
        c.bundle([conflict])
        self.assertEqual(self.receipt(c, conflict)['code'], 'request_id_conflict')
        second = self.action(c, 1)
        stale = {**second, 'request_id': 'stale', 'offered_revision': 0}
        c.bundle([stale])
        self.assertEqual(self.receipt(c, stale)['code'], 'stale_offer')
        wrong_game = {**second, 'request_id': 'old-game', 'game_id': 'earlier-game'}
        c.bundle([wrong_game])
        self.assertEqual(self.receipt(c, wrong_game)['code'], 'wrong_game')
        override = {**second, 'request_id': 'override', 'payment': {'bank': 0}}
        c.bundle([override])
        self.assertEqual(self.receipt(c, override)['code'], 'malformed_request')
        c.bundle([second, self.action(c, 2)])
        self.assertEqual(c.state['game']['turn'], 1)
        self.assertEqual(self.receipt(c, request)['status'], 'applied')
        self.assertEqual(self.receipt(c, second)['status'], 'applied')
        revision = c.state['revision']
        events = c.state['events']
        c.bundle([request])
        self.assertEqual(c.state['revision'], revision)
        self.assertEqual(c.state['events'], events)
        self.abort(c)

    def test_bundle_identity_conflict_is_checked_before_requests(self):
        c = self.client()
        first = self.action(c)
        c.bundle([first], batch_id='same-batch')
        baseline = c.state
        c.bundle([self.action(c, 1)], batch_id='same-batch')
        self.assertEqual(c.state['transport']['code'], 'batch_id_conflict')
        self.assertEqual(c.state['revision'], baseline['revision'])
        self.assertEqual(c.state['waiting_for'], [1, 2])
        c.bundle([first], batch_id='same-batch')
        self.assertEqual(c.state['transport']['status'], 'processed')
        self.assertEqual(c.state['revision'], baseline['revision'])
        self.abort(c)

    def test_mismatch_incomplete_json_and_legacy_files_do_not_execute(self):
        c = self.client()
        initial = c.state
        request = self.action(c)
        header = {'protocol_version': 3, 'game_id': initial['game_id'], 'batch_id': 'one'}
        atomic_write(c.exchange / 'commands.json', json.dumps({**header, 'requests': [request]}))
        atomic_write(c.exchange / 'ready.json', json.dumps({**header, 'batch_id': 'two'}))
        atomic_write(c.exchange / 'ready.txt', 'ready\n' * 3)
        for seat in (1, 2, 3):
            atomic_write(c.exchange / f'player_{seat}.json', json.dumps({'turn': 0, 'command': {'subcommand': 'discard', 'argument': 'Altar', 'extra': ''}}))
        time.sleep(.04)
        self.assertEqual(json.loads((c.exchange / 'game_status.json').read_text())['revision'], initial['revision'])
        atomic_write(c.exchange / 'commands.json', '{')
        atomic_write(c.exchange / 'ready.json', json.dumps(header))
        time.sleep(.04)
        self.assertEqual(json.loads((c.exchange / 'game_status.json').read_text())['revision'], initial['revision'])
        atomic_write(c.exchange / 'commands.json', json.dumps({**header, 'requests': [request]}))
        c.wait(lambda s: s.get('transport') is not None and s['transport']['batch_id'] == 'one')
        self.assertEqual(c.state['waiting_for'], [1, 2])
        self.abort(c)

    def test_timeout_cancels_accepted_request_without_normal_result(self):
        c = self.client(timeout_ms=400)
        original = c.state['players']
        request = self.action(c)
        c.bundle([request])
        c.wait(lambda s: s['game']['phase'] == 'aborted')
        self.assertEqual(c.state['abort_reason'], 'decision_timeout')
        self.assertEqual(c.state['players'], original)
        self.assertEqual(self.receipt(c, request)['status'], 'cancelled')
        self.assertIsNone(c.state['result'])
        self.assertFalse((c.root / 'results.txt').exists())
        self.assertEqual(c.process.wait(timeout=5), 2)

    def test_game_identity_is_separate_from_seed(self):
        a, b = self.client(seed=7), self.client(seed=7)
        self.assertNotEqual(a.state['game_id'], b.state['game_id'])
        old = self.action(a)
        b.bundle([old])
        self.assertEqual(self.receipt(b, old)['code'], 'wrong_game')
        self.assertEqual(b.state['waiting_for'], [0, 1, 2])
        self.abort(a)
        self.abort(b)

    def test_full_games_with_real_wonder_and_discard_choices(self):
        results = [play(ENGINE, seed) for seed in (0, 7, 8, 19, 42)]
        self.assertTrue(any(r['discard_decisions'] for r in results), results)
        again = play(ENGINE, 7, terminal_replay=True)
        self.assertEqual(again, results[1])


if __name__ == '__main__':
    unittest.main()
