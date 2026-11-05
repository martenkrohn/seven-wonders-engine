"""Legacy GUI contracts and real-engine exchanges; no display or RL dependencies."""
import hashlib
import json
from pathlib import Path
import sys
import time
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from gui.assets import asset_path
from gui.session import GameSession, SessionError, default_action, legal_actions, needs_action
from scripts.smoke_protocol import play


def wait_state(session):
    while True:
        state = session.poll()
        if state is not None:
            return state
        time.sleep(.001)


class GuiContracts(unittest.TestCase):
    def test_free_build_cannot_spend_ability_on_an_already_owned_card(self):
        player = dict(cards_hand=['Baths', 'Temple'], cards_playable=[],
                      cards_played=['Baths'], can_build_wonder=False, can_build_hand_free=True)
        self.assertEqual(legal_actions(player, 'Baths'), ('discard',))
        self.assertIn('build_hand_free', legal_actions(player, 'Temple'))

    def test_seventh_card_requires_the_legacy_babylon_ability(self):
        state = {"game": {"phase": "playing", "turn": 6}, "players": {
            "0": {"wonder_id": 8, "wonder_stage": 1},
            "1": {"wonder_id": 8, "wonder_stage": 2},
            "2": {"wonder_id": 7, "wonder_stage": 4}}}
        self.assertEqual([needs_action(state, seat) for seat in range(3)], [False, True, False])
        for turn in (13, 20):
            state['game']['turn'] = turn
            self.assertTrue(needs_action(state, 1))
        state['game'].update(turn=21, phase='finished')
        self.assertFalse(any(needs_action(state, seat) for seat in range(3)))

    def test_original_manufactured_artwork_tracks_age(self):
        expected = {'Glassworks': ('Glassworks2', 'Glassworks'),
                    'Loom': ('Loom', 'Loom2'), 'Press': ('Press', 'Press2')}
        for name, filenames in expected.items():
            for age, filename in enumerate(filenames, 1):
                self.assertEqual(asset_path('/assets', 'cards', name, age),
                                 Path('/assets/cards') / (filename + '.png'))


@unittest.skipUnless((ROOT / 'build/7Wonders').is_file(), 'Build the engine first')
class GuiEngineContracts(unittest.TestCase):
    def create_session(self, **kwargs):
        session = GameSession(ROOT / 'build/7Wonders', **kwargs)
        self.addCleanup(session.close)
        session.start()
        return session

    def test_human_and_bot_modes_match_the_existing_full_game_client(self):
        reference = play(ROOT / 'build/7Wonders', 42)
        for bots in (0, 2, 3):
            with self.subTest(bots=bots):
                session = self.create_session(seed=42, bots=bots)
                history = [wait_state(session)]
                while history[-1]['game']['phase'] != 'finished':
                    choices = {seat: default_action(session.state['players'][str(seat)])
                               for seat in range(bots, 3) if needs_action(session.state, seat)}
                    session.submit(choices)
                    history.append(wait_state(session))
                fingerprint = hashlib.sha256(json.dumps(history, sort_keys=True).encode()).hexdigest()
                self.assertEqual(fingerprint, reference['history_sha256'])
                self.assertEqual(len(history), 22)
                self.assertEqual(session.process.wait(timeout=5), 0)
                for (seat, card), age in session.played_era.items():
                    appeared = next(i for i, state in enumerate(history) if card in state['players'][str(seat)]['cards_played'])
                    self.assertEqual(age, history[appeared-1]['game']['era'])
                session.close()

    def test_invalid_or_missing_human_action_does_not_publish_ready(self):
        session = self.create_session(bots=2)
        wait_state(session)
        for choices in ({}, {2: ('build_structure', 'not in hand')}):
            with self.assertRaises(ValueError):
                session.submit(choices)
            self.assertFalse((session.exchange / 'ready.txt').exists())
            self.assertFalse(list(session.exchange.glob('player_*.json')))
            self.assertFalse(session.pending)
        session.submit({2: default_action(session.state['players']['2'])})
        self.assertEqual(wait_state(session)['game']['turn'], 1)

    def test_sessions_are_isolated_and_close_reaps_the_engine(self):
        first = self.create_session(seed=7)
        second = self.create_session(seed=19)
        self.assertNotEqual(first.exchange, second.exchange)
        self.assertEqual(wait_state(first)['game']['engine_seed'], 7)
        self.assertEqual(wait_state(second)['game']['engine_seed'], 19)
        directory = first.runtime
        first.close()
        self.assertIsNotNone(first.process.poll())
        self.assertFalse(directory.exists())
        self.assertIsNone(second.process.poll())

    def test_engine_exit_is_reported_instead_of_accepting_stale_status(self):
        session = self.create_session(bots=3)
        wait_state(session)
        session.process.kill()
        session.process.wait(timeout=5)
        session.submit({})
        with self.assertRaisesRegex(SessionError, 'exited before turn 1'):
            session.poll()

    def test_waiting_on_a_stale_status_times_out(self):
        session = self.create_session()
        wait_state(session)
        session.expected_turn, session.pending = 1, True
        session.deadline = time.monotonic()-1
        with self.assertRaisesRegex(SessionError, 'Timed out waiting for turn 1'):
            session.poll()


if __name__ == '__main__':
    unittest.main()
