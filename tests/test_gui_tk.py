"""Actual Tk controls, hotseat privacy and verified second-edition artwork."""
import os
from pathlib import Path
import sys
import tempfile
import time
import unittest
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT))
from gui.bga_assets import BgaAssets
from gui.decision_session import DecisionSession
from gui.decision_view import DecisionApp
ASSETS=Path(os.environ.get('SEVENWONDERS_ASSETS',ROOT.parent/'references/bga-sevenwonders'))

class ArtworkTests(unittest.TestCase):
    def test_missing_artwork_has_current_text_values(self):
        with tempfile.TemporaryDirectory() as tmp:
            a=BgaAssets(tmp)
            self.assertIsNone(a.crop(a.cards[(18,1)]['sprite']))
            self.assertIn('3 Punkte',a.card_text(18,1))
            self.assertIn('vollständigem Wunder',a.card_text(78,3))
            self.assertIn('3 Münzen',a.card_text(76,3))

    def test_all_verified_sprites_and_grey_ages(self):
        if not ASSETS.exists():self.skipTest('External BGA artwork not supplied')
        try:from PIL import Image
        except ImportError:self.skipTest('Optional Pillow not installed')
        a=BgaAssets(ASSETS)
        for c in a.cards.values():
            crop=a.crop(c['sprite']);self.assertIsNotNone(crop);self.assertEqual(crop.size,(255,393))
        for w in a.wonders.values():self.assertEqual(a.crop(w['sprite']).size,(450,223))
        for identity in (15,16,17):
            self.assertEqual(a.crop(a.cards[(identity,1)]['sprite']).tobytes(),a.crop(a.cards[(identity,2)]['sprite']).tobytes())
        self.assertEqual(a.wonders[0]['sprite']['y'],0)
        self.assertFalse(a.missing)

class TkDecisionTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        import tkinter as tk
        cls.tk=tk
        try:r=tk.Tk();r.destroy()
        except tk.TclError as error:raise unittest.SkipTest(f'Actual Tk display required: {error}')

    def start(self,seed,bots,directory):
        root=self.tk.Tk();session=DecisionSession(ROOT/'build/7Wonders',seed,bots)
        app=DecisionApp(root,session,directory)
        self.addCleanup(lambda: app.close() if not app.closed else None)
        return root,session,app

    def pump(self,root,app,predicate):
        deadline=time.monotonic()+20
        while not predicate():
            self.assertLess(time.monotonic(),deadline,'GUI stalled')
            root.update();self.assertIsNone(app.error);time.sleep(.003)

    def test_hotseat_hides_next_human_hand_until_reveal(self):
        root,s,app=self.start(7,0,'/nonexistent')
        self.pump(root,app,lambda: app.reveal_button is not None)
        self.assertEqual(app.option_buttons,[])
        app.reveal_button.invoke();self.assertEqual(app.visible_actor,0)
        button=next(b for b,o in app.option_buttons if o['kind']=='discard')
        button.invoke();self.assertEqual(app.option_buttons,[])
        self.pump(root,app,lambda: app.reveal_button is not None)
        self.assertEqual(s.human_decision()['actor'],1)
        self.assertEqual(app.option_buttons,[])

    def test_complete_click_games_with_and_without_images(self):
        seen=set()
        for seed,directory in ((8,ASSETS),(19,Path('/nonexistent')),(3,ASSETS),(9,Path('/nonexistent'))):
            root,s,app=self.start(seed,2,directory)
            deadline=time.monotonic()+30;selections=0
            while not s.state or s.state['game']['phase']!='finished':
                self.assertLess(time.monotonic(),deadline,'GUI game stalled')
                root.update();self.assertIsNone(app.error)
                if app.reveal_button is not None:app.reveal_button.invoke()
                legal=[(b,o) for b,o in app.option_buttons if o['selectable']]
                if legal:
                    free=[(b,o) for b,o in legal if str(o.get('free_source','')).startswith('olympia_')]
                    b,o=free[0] if free else next((b,o) for kind in ('build_discard_free','build_wonder','build_structure','discard') for b,o in legal if o['kind']==kind)
                    seen.add(o.get('free_source'));seen.add(o['kind'])
                    b.invoke();selections+=1
                time.sleep(.003)
            root.update();self.assertGreater(selections,15)
            self.assertEqual(app.heading.cget('text'),'Spielende')
            self.assertEqual(app.option_buttons,[])
            app.close()
        self.assertTrue({'olympia_first_color','olympia_first_age_card','olympia_last_age_card','build_discard_free'}.issubset(seen),seen)
