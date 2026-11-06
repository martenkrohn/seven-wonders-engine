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
ASSETS=Path(os.environ.get('SEVENWONDERS_ASSETS',ROOT/'images'))

class ArtworkTests(unittest.TestCase):
    def test_missing_artwork_has_current_text_values(self):
        with tempfile.TemporaryDirectory() as tmp:
            a=BgaAssets(tmp)
            self.assertIsNone(a.crop(a.cards[(18,1)]['sprite']))
            self.assertIn('3 points',a.card_text(18,1))
            self.assertIn('if your wonder is complete',a.card_text(78,3))
            self.assertIn('3 coins',a.card_text(76,3))

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

    def assert_perspective(self,app,state,actor):
        self.assertEqual(int(app.players[actor].frame.grid_info()['column']),1)
        self.assertIn('You',app.players[actor].summary.itemcget('seat-title','text'))
        for side,column in (('left',0),('right',2)):
            neighbor=state['players'][str(actor)]['neighbors'][side]
            self.assertEqual(int(app.players[neighbor].frame.grid_info()['column']),column)
            self.assertIn('Left' if side=='left' else 'Right',
                          app.players[neighbor].summary.itemcget('seat-title','text'))

    def test_hotseat_hides_next_human_hand_until_reveal(self):
        root,s,app=self.start(7,0,'/nonexistent')
        self.pump(root,app,lambda: app.reveal_button is not None)
        self.assert_perspective(app,s.state,0)
        self.assertEqual(app.option_buttons,[])
        app.reveal_button.invoke();root.update();self.assertEqual(app.visible_actor,0)
        frame=next(iter(app.card_widgets.values()))[0]
        frame.event_generate('<Button-1>');root.update()
        button=next(b for b,o in app.option_buttons if o['kind']=='discard')
        button.invoke()
        app.selected_detail();root.update()
        self.assertTrue(app.detail_window.winfo_exists())
        self.assertFalse(s.pending,'Choosing an action must not submit it')
        app.confirm_button.invoke();self.assertEqual(app.option_buttons,[])
        self.assertFalse(app.detail_window.winfo_exists(),'Private detail must close at handover')
        self.pump(root,app,lambda: app.reveal_button is not None)
        self.assertEqual(s.human_decision()['actor'],1)
        self.assertEqual(app.option_buttons,[])
        self.assert_perspective(app,s.state,1)

    def test_complete_click_games_with_and_without_images(self):
        seen=set();decision_kinds=set()
        # Corrected directional trade changes affordable paths. Seed 6 reaches
        # both night free modes; retain exact coverage assertions below.
        for seed,directory in ((8,ASSETS),(19,Path('/nonexistent')),(6,ASSETS),(9,Path('/nonexistent')),(39,ASSETS)):
            root,s,app=self.start(seed,2,directory)
            deadline=time.monotonic()+30;selections=0
            while not s.state or s.state['game']['phase']!='finished':
                self.assertLess(time.monotonic(),deadline,'GUI game stalled')
                root.update();self.assertIsNone(app.error)
                if app.reveal_button is not None:
                    app.reveal_button.invoke();root.update()
                legal=[o for _,options in app.card_widgets.values() for o in options if o['selectable']]
                if legal:
                    self.assert_perspective(app,s.state,2)
                    if app.decision['kind']!='discard':
                        frames=[frame for frame,_ in app.card_widgets.values()]
                        for left,right in zip(frames,frames[1:]):
                            self.assertEqual(right.winfo_rootx()-left.winfo_rootx()-left.winfo_width(),10)
                        center=(frames[0].winfo_rootx()+frames[-1].winfo_rootx()+frames[-1].winfo_width())/2
                        self.assertAlmostEqual(center,app.canvas.winfo_rootx()+app.canvas.winfo_width()/2,delta=1)
                    decision_kinds.add(app.decision['kind'])
                    free=[o for o in legal if str(o.get('free_source','')).startswith('olympia_')]
                    o=free[0] if free else next(o for kind in ('build_discard_free','build_wonder','build_structure','discard') for o in legal if o['kind']==kind)
                    seen.add(o.get('free_source'));seen.add(o['kind'])
                    frame=app.card_widgets[o['card']['instance_id']][0]
                    frame.event_generate('<Button-1>');root.update()
                    b=next(b for b,candidate in app.option_buttons if candidate['option_id']==o['option_id'])
                    b.invoke()
                    self.assertFalse(s.pending,'Only confirmation may submit a selection')
                    self.assertIsNotNone(app.selected_option)
                    app.confirm_button.invoke()
                    self.assertIsNone(app.selected_option)
                    selections+=1
                time.sleep(.003)
            root.update();self.assertGreater(selections,15)
            self.assertEqual(app.heading.cget('text'),'Game over')
            self.assertEqual(app.option_buttons,[])
            self.assertFalse(app.canvas.winfo_ismapped(),'Finished cities should use the hand area')
            app.close()
        self.assertTrue({'olympia_first_color','olympia_first_age_card','olympia_last_age_card','build_discard_free'}.issubset(seen),seen)
        self.assertEqual(decision_kinds,{'regular','seventh','discard'})

    def test_full_hand_fits_minimum_window_and_selection_survives_resize(self):
        root,s,app=self.start(8,2,ASSETS)
        self.pump(root,app,lambda: app.reveal_button is not None)
        app.reveal_button.invoke();root.geometry('1120x800');root.update()
        self.assertEqual(len(app.card_widgets),7)
        for frame,_ in app.card_widgets.values():
            self.assertEqual(int(frame.grid_info()['row']),0)
            self.assertLessEqual(frame.winfo_y()+frame.winfo_height(),app.canvas.winfo_height())
        frame=next(iter(app.card_widgets.values()))[0]
        frame.event_generate('<Button-1>');root.update()
        selected=app.selected_option['option_id']
        root.geometry('1500x1000');root.update()
        self.assertEqual(app.selected_option['option_id'],selected)
        self.assertFalse(s.pending)
        self.assertGreaterEqual(app.players[0].city.winfo_height(),100)

    def test_color_columns_stay_separate_with_combined_brown_and_grey_production(self):
        root,s,app=self.start(8,2,ASSETS)
        self.pump(root,app,lambda: app.reveal_button is not None)
        self.assert_perspective(app,s.state,2)
        # Render every real card definition to check grouping and image-only strips.
        player=dict(s.state['players']['2'])
        cards=[{'instance_id':f'column-{i}','type_id':c['engine_id'],'age':c['age'],'name':c['name']}
               for i,c in enumerate(app.assets.cards.values())]
        player['built_cards']=cards
        view=app.players[2];view.render(player,True,role='You',human=True);root.update()
        if not app.assets.missing:
            self.assertFalse(any(view.city.type(item)=='text' for item in view.city.find_all()),
                             'Built cards with artwork must not have names or column headings')
        for resource in ('wood','stone','ore','clay','papyrus','loom','glass'):
            self.assertFalse(view.summary.find_withtag('resource-'+resource))
        for tag in ('resource-coins','resource-shields','score-scientific','score-total'):
            self.assertTrue(view.summary.find_withtag(tag))
        by_color={}
        for card in cards:
            color=app.assets.cards[(card['type_id'],card['age'])]['color']
            box=view.city.bbox('card-'+card['instance_id'])
            self.assertIsNotNone(box)
            by_color.setdefault(color,[]).append(box)
        columns={color:{box[0] for box in boxes} for color,boxes in by_color.items()}
        self.assertTrue(all(len(xs)==1 for xs in columns.values()))
        self.assertEqual(columns['brown'],columns['grey'])
        positions=[next(iter(columns[color])) for color in ('brown','blue','yellow','red','green','purple')]
        self.assertEqual(positions,sorted(set(positions)))
        self.assertLess(max(box[3] for box in by_color['brown']),min(box[1] for box in by_color['grey']))
        wonder_box=view.city.bbox('wonder-production')
        self.assertEqual(wonder_box[0],next(iter(columns['brown'])))
        self.assertLess(wonder_box[3],min(box[1] for box in by_color['brown']))
        self.assertTrue(view.city.find_withtag('wonder-production-marker'))
        root.geometry('1120x800');root.update()
        self.assertGreater(float(view.city.cget('scrollregion').split()[3]),view.city.winfo_height())
        for card in cards:
            box=view.city.bbox('card-'+card['instance_id'])
            self.assertGreaterEqual(box[0],0)
            self.assertLessEqual(box[2],view.city.winfo_width())

    def test_crop_keeps_whole_effect_band_for_every_card_and_stages_match_boards(self):
        from gui.tableau import STAGE_CENTERS
        root,s,app=self.start(8,2,ASSETS)
        self.pump(root,app,lambda: app.reveal_button is not None)
        for c in app.assets.cards.values():
            full=app.assets.crop(c['sprite'])
            if full is not None:
                photo=app.assets.photo('built',c['engine_id'],c['age'],root,(255,84))
                # Compare pixels at both edges, including the far-right chain marks.
                for x in (0,50,200,254):
                    for y in (0,35,83):
                        self.assertEqual(tuple(map(int,root.tk.call(str(photo),'get',x,y))),full.getpixel((x,y+26)))
        for identity,w in app.assets.wonders.items():
            self.assertEqual(len(STAGE_CENTERS[identity]),len(w['stages']))
            self.assertTrue(all(0<x<450 for x in STAGE_CENTERS[identity]))
            player=dict(s.state['players']['2'],wonder_id=identity,built_cards=[])
            view=app.players[2];view.render(player,True,role='You',human=True);root.update()
            images=[item for item in view.city.find_withtag('wonder-production') if view.city.type(item)=='image']
            if app.assets.icon(w['starting_resource'],root) is not None:
                self.assertEqual(len(images),1)
                self.assertEqual(view.city.itemcget(images[0],'image'),str(app.assets.icon(w['starting_resource'],root)))

    def test_large_discard_layout_scrolls_and_duplicate_is_not_selectable(self):
        root,s,app=self.start(8,2,ASSETS)
        self.pump(root,app,lambda: app.reveal_button is not None)
        decision=dict(app.decision)
        # Layout fixture only: build 24 physical offers from actual card definitions.
        # No synthetic option is sent to the engine.
        options=[]
        for i,c in enumerate(list(app.assets.cards.values())[:24]):
            options.append({'option_id':f'layout-{i}','kind':'build_discard_free',
                'card':{'instance_id':f'layout-card-{i}','type_id':c['engine_id'],
                        'age':c['age'],'name':c['name']},
                'free_source':'discard_grant','payment':{'bank':0,'to_seats':{'0':0,'1':0,'2':0}},
                'selectable':i!=23})
        decision.update(kind='discard',options=options)
        app.visible_actor=2;app.show_decision(decision);root.update()
        self.assertTrue(app.scrollbar.winfo_ismapped())
        self.assertGreater(float(app.canvas.cget('scrollregion').split()[3]),app.canvas.winfo_height())
        app.canvas.yview_moveto(1);root.update()
        frame=app.card_widgets['layout-card-22'][0]
        frame.event_generate('<Button-1>');root.update()
        self.assertEqual(app.selected_option['option_id'],'layout-22')
        frame=app.card_widgets['layout-card-23'][0]
        frame.event_generate('<Button-1>');root.update()
        self.assertIsNone(app.selected_option)
        self.assertIn('disabled',app.confirm_button.state())
        self.assertFalse(s.pending)
