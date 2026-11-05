#!/usr/bin/env python3
"""Exercise the actual Tk window, clicks and scored final display (requires a display)."""
import argparse
import json
from pathlib import Path
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))

from gui import legacy_view as view
from gui.session import GameSession, default_action, needs_action


def check(engine, assets, bots, seed, mixed=False):
    def unexpected_dialog(title, message, **kwargs):
        raise AssertionError(f'Unexpected GUI dialog: {title}: {message}')
    view.messagebox.showinfo = unexpected_dialog
    session = GameSession(engine, bots=bots, seed=seed)
    window = view.create_window(session, assets)
    seen = set()
    maximum_widgets = 0
    used_actions = set()
    callback_errors = []
    window.report_callback_exception = lambda *error: callback_errors.append(error)
    try:
        deadline = time.monotonic()+45
        while time.monotonic() < deadline:
            window.update()
            assert not callback_errors, callback_errors
            maximum_widgets = max(maximum_widgets, len(view.canvas.winfo_children()))
            state = session.state
            if state is None or session.pending:
                time.sleep(.003)
                continue
            turn = state['game']['turn']
            seen.add(turn)
            assert not any(ack['status'] == 'fallback_discard'
                           for ack in state.get('last_actions', {}).values()), state['last_actions']
            if state['game']['phase'] == 'finished':
                assert turn == 21
                assert view.label_infos['text'] == 'Game finished'
                assert str(view.play_button['state']) == 'disabled'
                assert 'Final scores:' in view.footer['text']
                assert session.process.wait(timeout=5) == 0
                return {'seed': seed, 'bots': bots, 'states': len(seen),
                        'maximum_widgets': maximum_widgets,
                        'human_action_types': sorted(used_actions),
                        'missing_assets': sorted(view.missing_assets),
                        'scores': [state['players'][str(i)]['points']['total'] for i in range(3)]}
            for seat in range(bots, 3):
                if not needs_action(state, seat):
                    continue
                player = state['players'][str(seat)]
                action, card = default_action(player)
                if mixed:
                    free_cards = [c for c in player['cards_hand'] if c not in player['cards_played']]
                    if player['can_build_hand_free'] and free_cards:
                        action, card = 'build_hand_free', free_cards[0]
                    elif player['can_build_wonder'] and turn % 2 == 0:
                        action, card = 'build_wonder', player['cards_hand'][0]
                    elif turn % 6 == 5:
                        action, card = 'discard', player['cards_hand'][0]
                used_actions.add(action)
                slot = player['cards_hand'].index(card)
                x, y = view.hand_card_position(seat, slot)
                view.canvas.event_generate('<Button-1>', x=int(x+5), y=int(y+5))
                window.update()
                assert view.card_selected[seat] == (slot, card)
                view.buttons[seat][view.actions.index(action)].invoke()
            view.play_button.invoke()
        raise RuntimeError('GUI did not reach the scored final display within 45 seconds')
    finally:
        session.close()
        window.destroy()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine', type=Path, default=ROOT/'build/7Wonders')
    parser.add_argument('--assets-dir', type=Path, default=ROOT/'images')
    parser.add_argument('--bots', type=int, choices=(0, 2, 3), default=2)
    parser.add_argument('--seed', type=int, default=42)
    parser.add_argument('--mixed-actions', action='store_true')
    args = parser.parse_args()
    print(json.dumps(check(args.engine.resolve(), args.assets_dir.resolve(), args.bots, args.seed, args.mixed_actions), indent=2))
