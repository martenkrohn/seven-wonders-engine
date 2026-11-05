"""Active protocol-3 GUI session tests, independent of display and images."""
from pathlib import Path
import sys
import time
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from sevenwonders_engine import DecisionClient
from gui.decision_session import DecisionSession
ENGINE=ROOT/'build/7Wonders'

class DecisionGuiTests(unittest.TestCase):
    def test_sessions_are_isolated_and_close_reaps_process(self):
        a,b=DecisionClient(ENGINE,7),DecisionClient(ENGINE,7)
        try:
            self.assertNotEqual(a.wait()['game_id'],b.wait()['game_id'])
            self.assertNotEqual(a.root,b.root)
        finally:a.close();b.close()
        self.assertIsNotNone(a.process.poll());self.assertFalse(a.root.exists())

    def test_invalid_human_option_publishes_nothing(self):
        s=DecisionSession(ENGINE,bots=2)
        try:
            s.client.wait()
            with self.assertRaises(ValueError):s.submit_choice(2,'forged')
            self.assertFalse((s.client.exchange/'ready.json').exists())
            with self.assertRaises(ValueError):s.submit_choice(0,s.state['decisions'][0]['options'][0]['option_id'])
        finally:s.close()

    def test_complete_games_all_human_one_human_and_bots(self):
        discards=0
        for bots,seed in ((0,8),(2,7),(3,0),(2,8),(0,19),(3,42)):
            s=DecisionSession(ENGINE,seed,bots)
            try:
                s.client.wait();steps=0
                while s.state['game']['phase']!='finished':
                    self.assertLess(steps,100);steps+=1
                    if not s.drive_bots():
                        d=s.human_decision();self.assertIsNotNone(d)
                        discards+=d['kind']=='discard'
                        legal=[o for o in d['options'] if o['selectable']]
                        o=next(o for kind in ('build_discard_free','build_wonder','build_structure','discard') for o in legal if o['kind']==kind)
                        s.submit_choice(d['actor'],o['option_id'])
                    s.client.wait()
                    self.assertTrue(all(r['status'] in ('accepted','applied','skipped_no_options') for r in s.state['receipts']))
                deadline=time.monotonic()+2
                while not (s.client.root/'results.txt').exists() and time.monotonic()<deadline:time.sleep(.002)
                self.assertEqual(s.state['game']['turn'],21)
                text=(s.client.root/'results.txt').read_text()
                for seat in range(3):self.assertIn(str(s.state['result']['totals'][str(seat)]),text)
            finally:s.close()
        self.assertGreater(discards,0)

    def test_abort_has_no_normal_result(self):
        with DecisionClient(ENGINE) as c:
            c.abort('GUI test');state=c.wait()
            self.assertEqual(state['game']['phase'],'aborted')
            self.assertFalse((c.root/'results.txt').exists())
