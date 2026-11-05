# Restored from dmag-ufsm/7Wonders Game.py at a0bc333 (2021-10-07).
# Downstream: standalone paths, protocol-v2 session/lifecycle, asset fallbacks.
# Original Tk canvas layout and rendering functions retained; see docs/GUI_AUDIT.md.
# Graphical Interface for 7 Wonders DMAG implementation
# required libs: tkinter

# Antes de executar esse arquivo, compile o jogo:
# $ mkdir build && cd build && cmake .. && make

# Remaining layout/rule limits are tracked in docs/GUI_AUDIT.md.

from tkinter import *
from tkinter import messagebox
from pathlib import Path
from .assets import asset_path
from .session import SessionError, needs_action

ASSETS_DIR = ''
session = None
missing_assets = set()
image_cache = {}

### variables
players = 3
w_width = 1300
w_height = 705
header_h = 38
pad = 5
bg = '#FFFFFF'
btn_bg = '#CCCCCC'
btn_bg_active = '#449944'
action_active = [0]*players
card_selected = [(-1, '')]*players
root = None
canvas = None
footer = None
play_button = None
label_infos = [None]
img_clockwise = [None]
img_wonder = [None]*players
img_wonder_stage = [None]*players
img_hand_cards = [None]*players
img_resources = [None]*players
lab_resources_v = [None]*players
img_points = [None]*players
lab_points_v = [None]*players
for i in range(len(lab_points_v)):
    lab_points_v[i] = [None] * 8
img_played_cards = [None]*players
img_buttons = [None]*players
buttons = [None]*players
actions = ['build_structure', 'build_wonder', 'discard', 'build_hand_free']
game_json = {}

def load_image(local, f, era=None):
    path = Path(local)
    era = era if era is not None else game_json.get('game', {}).get('era', 1)
    path = asset_path(path.parent.parent, path.parent.name, path.stem, era)
    key = (str(path), f)
    if key in image_cache:
        return image_cache[key]
    try:
        img = PhotoImage(file=str(path))
        img.placeholder = False
    except TclError:
        missing_assets.add(str(path))
        if path.parent.name == 'cards':
            width, height = 625, 266
        elif path.parent.name == 'wonders':
            width, height = (118, 32) if path.stem == 'wonder_stage' else (450, 198)
        else:
            width, height = (35, 35) if path.stem in actions else (95, 95)
        img = PhotoImage(width=width, height=height)
        img.put('#e6e0cf', to=(0, 0, width, height))
        img.placeholder = True
    placeholder = img.placeholder
    f_z, f_ss = f
    if f_z != 1:
        img = img.zoom(f_z)
    if f_ss != 1:
        img = img.subsample(f_ss)
    img.placeholder = placeholder
    img.caption = path.stem.replace('_', ' ')
    image_cache[key] = img
    return img


def draw_image(x, y, image, anchor=NW, caption=None):
    canvas.create_image(x, y, image=image, anchor=anchor)
    if image.placeholder:
        canvas.create_rectangle(x, y, x+image.width(), y+image.height(), outline='#a99c83')
        canvas.create_text(x+image.width()/2, y+image.height()/2,
                           text=caption or image.caption, width=max(15, image.width()-8),
                           font=('Verdana', 8 if image.width() > 100 else 6))


size = {
    'wonder' : (2, 3),
    'wonder_stage' : (2, 3),
    'hand_card' : (1, 4),
    'played_card' : (1, 7),
    'resource' : (1, 4),
    'button' : (1, 1),
    'clockwise' : (1, 1)
}
img = {}


def sort(cards):
    return sorted(cards, key=lambda c: (session.name_to_id.get(c, 999), c))


def load_wonder():
    global img_wonder
    for i in range(players):
        wonder_id = game_json['players'][str(i)]['wonder_id']
        img_wonder[i] = load_image(ASSETS_DIR+'/wonders/board_'+str(wonder_id)+'.png', size['wonder'])
        draw_image(pad + (w_width/players)*i, header_h + pad, image=img_wonder[i], anchor=NW, caption=game_json['players'][str(i)]['wonder_name'])

def load_wonder_stages():
    global img_wonder_stage
    for i in range(players):
        wonder_stage = game_json['players'][str(i)]['wonder_stage']
        img_wonder_stage[i] = [None]*wonder_stage
        for j in range(wonder_stage):
            img_wonder_stage[i][j] = load_image(ASSETS_DIR+'/wonders/wonder_stage.png', size['wonder_stage'])
            x = pad + (w_width/players)*i + img['wonder'].width()*0.065 + j*(img['wonder_stage'].width() + img['wonder'].width()*0.035)
            y = header_h + pad + img['wonder'].height()
            draw_image(x, y, image=img_wonder_stage[i][j], anchor=NW)

def hand_card_position(player, card):
    x = pad + (w_width/players)*player
    y = header_h + img['wonder'].height() + img['wonder_stage'].height() + pad*(card+2) + img['hand_card'].height()*card
    return (x, y)

def load_resources():
    p = {'wood' : 0, 'stone' : 1, 'ore' : 2, 'clay' : 3, 'papyrus' : 4, 'loom' : 5,
         'glass' : 6, 'coins' : 7, 'compass': -1, 'gear' : -1, 'tablet' : -1, 'shields' : 8}
    for i in range(players):
        resources = game_json['players'][str(i)]['resources']
        img_resources[i] = [None]*len(resources)
        lab_resources_v[i] = [None]*len(resources)
        j = 0
        for r in resources:
            if p[r] != -1:
                img_resources[i][j] = load_image(ASSETS_DIR+'/resources/' + r + '.png', size['resource'])
                x = pad*2 + (w_width/players)*i + img['hand_card'].width() + (5+img['resource'].width())*p[r]
                y = header_h + pad*2 + img['wonder'].height() + img['wonder_stage'].height()
                draw_image(x, y, image=img_resources[i][j], anchor=NW)

                lab_resources_v[i][j] = Label(canvas, bg=bg, text=resources[r], font=("Verdana", 11))
                x = pad*2 + (w_width/players)*i + img['hand_card'].width() + (5+img['resource'].width())*p[r]
                x = x if resources[r] >= 10 else x + img['resource'].width()*0.15
                y = header_h + pad*2 + img['wonder'].height() + img['wonder_stage'].height() + img['resource'].height()
                lab_resources_v[i][j].place(x=x, y=y)
                j = j + 1

def load_vps():
    global lab_points_v
    p = {'civilian' : 1, 'commercial' : 2, 'military' : 3, 'scientific' : 4, 'guild' : 5, 'wonder' : 6, 'total' : 8}

    for i in range(players):
        points = game_json['players'][str(i)]['points']
        img_points[i] = [None]*len(points)
        #lab_points_v[i] = [None] * (len(points)+1)
        j = 0
        for pt in points:
            img_points[i][j] = load_image(ASSETS_DIR+'/resources/vp_' + pt + '.png', size['resource'])
            x = pad*2 + (w_width/players)*i + img['hand_card'].width() + (5+img['resource'].width())*p[pt]
            y = header_h + pad*2 + img['wonder'].height() + img['wonder_stage'].height() + 2*img['resource'].height()
            draw_image(x, y, image=img_points[i][j], anchor=NW)

            x = pad*2 + (w_width/players)*i + img['hand_card'].width() + (5+img['resource'].width())*p[pt] + img['resource'].width()*0.45 + 1
            y = header_h + pad*2 + img['wonder'].height() + img['wonder_stage'].height() + 2.5*img['resource'].height() - 2
            canvas.delete(lab_points_v[i][j])
            lab_points_v[i][j] = canvas.create_text((x, y), text=points[pt], font=("Verdana", 11))
            j = j + 1

        vp_label = Label(canvas, bg=bg, text='VPs', font=("Verdana", 9))
        x = pad*2 + (w_width/players)*i + img['hand_card'].width() + (5+img['resource'].width())*0
        y = header_h + pad*2 + img['wonder'].height() + img['wonder_stage'].height() + 2*img['resource'].height() + 2
        vp_label.place(x=x, y=y)

def load_hand_cards():
    global img_hand_cards
    for i in range(players):
        hand = game_json['players'][str(i)]['cards_hand']
        img_hand_cards[i] = []
        for j, card in enumerate(hand):
            image = load_image(ASSETS_DIR+'/cards/'+card+'.png', size['hand_card'])
            img_hand_cards[i].append(image)
            x, y = hand_card_position(i, j)
            draw_image(x, y, image, caption=card)
            if i >= session.bots and needs_action(game_json, i):
                color = '#208344' if card_selected[i][0] == j else '#999999'
                canvas.create_rectangle(x, y, x+image.width(), y+image.height(),
                                        outline=color, width=3 if card_selected[i][0] == j else 1)

def load_played_cards():
    global img_played_cards
    for i in range(players):
        cards_played = sort(game_json['players'][str(i)]['cards_played'])
        img_played_cards[i] = [None]*len(cards_played)
        for j, c in enumerate(cards_played):
            img_played_cards[i][j] = load_image(ASSETS_DIR+'/cards/' + c + '.png', size['played_card'], session.played_era.get((i, c), 1))
            x = pad*2 + (w_width/players)*i + img['hand_card'].width()
            y = header_h + pad*3 + 3 + img['wonder'].height() + img['wonder_stage'].height() + 3*img['resource'].height() + (j%7)*img['played_card'].height()*0.8
            x += (j//7) * (pad + img['played_card'].width())
            draw_image(x, y, image=img_played_cards[i][j], anchor=NW)
            j = j + 1

def load_header():
    global label_infos, play_button
    game = game_json['game']
    text = ('Game finished' if game['phase'] == 'finished' else
            'Age: '+str(game['era'])+'   Turn: '+str(game['turn']%7+1)+'   '+('>>' if game['clockwise'] else '<<'))
    label_infos = Label(canvas, bg=bg, text=text, font=('Verdana', 11))
    label_infos.place(x=pad, y=pad*2)
    play_button = Button(canvas, width=20, text='Play selected actions', font=('Verdana', 11), command=play)
    play_button.place(x=w_width/2-60, y=5)
    if game['phase'] == 'finished':
        play_button['state'] = DISABLED
    for seat in range(players):
        label = 'Player '+str(seat+1)+(' (simple bot)' if seat < session.bots else ' (human)')
        if game['phase'] != 'finished' and not needs_action(game_json, seat):
            label += ' — seventh card skipped'
        canvas.create_text(pad+(w_width/players)*seat+img['wonder'].width()/2,
                           header_h+img['wonder'].height()-8, text=label,
                           font=('Verdana', 8), fill='#111111')


def change_action(player, action):
    if session.pending or player < session.bots or not needs_action(game_json, player):
        return
    p = game_json['players'][str(player)]
    if actions[action] == 'build_wonder' and not p['can_build_wonder']:
        return
    if actions[action] == 'build_hand_free' and not p['can_build_hand_free']:
        return
    action_active[player] = action
    redraw()


def load_action_buttons():
    global img_buttons, buttons
    for i in range(players):
        img_buttons[i] = [None]*4
        buttons[i] = [None]*4
        for j in range(len(actions)):
            img_buttons[i][j] = load_image(ASSETS_DIR+'/resources/' + actions[j] + '.png', size['button'])
            x = pad*3 + (w_width/players)*i + img['wonder'].width()
            y = header_h + pad + j*(img['button'].height() + 2*pad)
            bg_color = btn_bg_active if action_active[i] == j else btn_bg
            width = img['button'].width()*2.5

            if game_json['players'][str(i)]['can_build_hand_free']:
                if actions[j] == 'build_structure':
                    width = width*0.5
                if actions[j] == 'build_hand_free':
                    x = pad*3 + (w_width/players)*i + img['wonder'].width() + width*0.5
                    y = header_h + pad + 0*(img['button'].height() + 2*pad)
                    width = width*0.5
            elif actions[j] == 'build_hand_free':
                continue

            captions = ('Build card', 'Build wonder', 'Discard', 'Free build')
            buttons[i][j] = Button(canvas, image=img_buttons[i][j], bg=bg_color, width=width,
                text=captions[j] if img_buttons[i][j].placeholder else '', compound=CENTER,
                wraplength=75, font=('Verdana', 7), command=lambda p=i, a=j : change_action(p, a))
            buttons[i][j].place(x=x, y=y, anchor=NW)
            if (i < session.bots or not needs_action(game_json, i) or
                    (actions[j] == 'build_wonder' and not game_json['players'][str(i)]['can_build_wonder'])):
                buttons[i][j]['state'] = DISABLED

def mouse_clicked(event):
    if session.pending:
        return
    for seat in range(session.bots, players):
        if not needs_action(game_json, seat):
            continue
        for slot, card in enumerate(game_json['players'][str(seat)]['cards_hand']):
            x, y = hand_card_position(seat, slot)
            if x <= event.x <= x+img['hand_card'].width() and y <= event.y <= y+img['hand_card'].height():
                card_selected[seat] = (slot, card)
                redraw()
                return


def redraw():
    for child in canvas.winfo_children():
        child.destroy()
    canvas.delete('all')
    load_wonder()
    load_wonder_stages()
    load_action_buttons()
    load_hand_cards()
    load_resources()
    load_vps()
    load_played_cards()
    load_header()
    notices = [f'Legacy rules | seed {session.seed} | All hands visible | Discard-pile choice unavailable']
    if missing_assets:
        notices.append('Missing artwork: labeled placeholders')
    fallbacks = [str(int(seat)+1) for seat, ack in game_json.get('last_actions', {}).items()
                 if ack['status'] == 'fallback_discard']
    if fallbacks:
        notices.append('Engine replaced requested action with discard for player '+', '.join(fallbacks))
    if game_json['game']['phase'] == 'finished':
        notices.append('Final scores: '+', '.join(str(game_json['players'][str(i)]['points']['total']) for i in range(3)))
    footer['text'] = ' | '.join(notices)


def play():
    if session.pending or game_json['game']['phase'] == 'finished':
        return
    choices = {seat: (actions[action_active[seat]], card_selected[seat][1])
               for seat in range(session.bots, players) if needs_action(game_json, seat)}
    try:
        session.submit(choices)
    except ValueError as error:
        messagebox.showinfo('Choose an available action', str(error), parent=root)
        return
    except (SessionError, OSError) as error:
        fail(error)
        return
    play_button['state'] = DISABLED
    footer['text'] = 'Waiting for engine…'


def fail(error):
    footer['text'] = str(error)
    if play_button is not None:
        play_button['state'] = DISABLED
    session.close()


def poll():
    global game_json, card_selected, action_active
    try:
        state = session.poll()
        if state is not None:
            game_json = state
            card_selected = [(-1, '')]*players
            action_active = [0]*players
            redraw()
            if state['game']['phase'] != 'finished' and all(
                    seat < session.bots or not needs_action(state, seat) for seat in range(3)):
                turn = state['game']['turn']
                root.after(80, lambda: play() if game_json['game']['turn'] == turn else None)
    except (SessionError, OSError) as error:
        fail(error)
        return
    root.after(30, poll)


def create_window(game_session, assets_dir):
    global session, ASSETS_DIR, root, canvas, footer, img, image_cache, missing_assets
    session, ASSETS_DIR = game_session, str(assets_dir)
    root = Tk()
    root.wm_title(f'7 Wonders — Legacy | seed {session.seed} | {session.bots} simple bots')
    footer = Label(root, text='Starting engine…', anchor=W, justify=LEFT,
                   wraplength=w_width-20, bg='#f0eee7')
    footer.pack(side=BOTTOM, fill=X)
    canvas = Canvas(root, bg=bg, highlightthickness=0)
    canvas.pack(expand=YES, fill=BOTH)
    image_cache, missing_assets = {}, set()
    img = {
        'wonder': load_image(ASSETS_DIR+'/wonders/board_0.png', size['wonder']),
        'wonder_stage': load_image(ASSETS_DIR+'/wonders/wonder_stage.png', size['wonder_stage']),
        'hand_card': load_image(ASSETS_DIR+'/cards/Mine.png', size['hand_card']),
        'played_card': load_image(ASSETS_DIR+'/cards/Mine.png', size['played_card']),
        'resource': load_image(ASSETS_DIR+'/resources/wood.png', size['resource']),
        'button': load_image(ASSETS_DIR+'/resources/discard.png', size['button']),
    }
    canvas.bind('<Button-1>', mouse_clicked)
    root.bind('<Return>', lambda event: play() if game_json else None)
    root.protocol('WM_DELETE_WINDOW', close)
    root.geometry(f'{w_width}x{w_height+30}')
    root.minsize(w_width, w_height+30)
    session.start()
    root.after(0, poll)
    return root


def close():
    session.close()
    root.destroy()


def run(game_session, assets_dir):
    window = create_window(game_session, assets_dir)
    try:
        window.mainloop()
    finally:
        game_session.close()
