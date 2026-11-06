"""Original-style graphical table, full portrait hand and explicit confirmation."""
import tkinter as tk
from tkinter import ttk

from .bga_assets import BgaAssets
from .tableau import Tableau, COLORS

ACTION = {"build_structure": "Build structure", "build_wonder": "Build wonder",
          "discard": "Sell (+3)", "build_discard_free": "Build from discard"}
FREE = {"chain": "Construction chain", "olympia_first_color": "First color free",
        "olympia_first_age_card": "First card of the age free",
        "olympia_last_age_card": "Sixth card of the age free", "discard_grant": "Free"}
HAND_CARD_WIDTH = 134
HAND_CARD_GAP = 10


class DecisionApp:
    def __init__(self, root, session, assets_dir=None):
        self.root, self.session = root, session
        self.assets = BgaAssets(assets_dir)
        self.closed, self.error, self.visible_actor = False, None, None
        self.decision_key, self.decision = None, None
        self.selected_card, self.selected_option = None, None
        self.option_buttons, self.card_widgets, self.reveal_button = [], {}, None
        self.detail_window = None
        self.focus_seat = None
        root.title("7 Wonders – Second Edition (2020)")
        root.geometry("1320x940")
        root.minsize(1120, 800)
        root.configure(bg="white")
        root.rowconfigure(1, weight=1)
        root.columnconfigure(0, weight=1)
        header = tk.Frame(root, bg="white")
        header.grid(row=0, column=0, sticky="ew", padx=12, pady=8)
        self.heading = tk.Label(header, bg="white", font=("Verdana", 13, "bold"))
        self.heading.pack(side="left")
        self.direction = tk.Label(header, bg="white", font=("Verdana", 10))
        self.direction.pack(side="right")
        self.status = tk.StringVar(value="Click a building to view its full card and construction chains.")
        table = tk.Frame(root, bg="white")
        table.grid(row=1, column=0, sticky="nsew", padx=8)
        table.rowconfigure(0, weight=1)
        self.players = []
        for seat in range(3):
            table.columnconfigure(seat, weight=1, uniform="seat")
            self.players.append(Tableau(table, seat, self.assets, self.show_detail, self.status))
        hand = tk.Frame(root, bg="white")
        hand.grid(row=2, column=0, sticky="ew", padx=12, pady=(10, 0))
        self.prompt = tk.Label(hand, bg="white", font=("Verdana", 11, "bold"), anchor="w")
        self.prompt.pack(fill="x", pady=(0, 4))
        self.canvas = tk.Canvas(hand, bg="white", height=248, highlightthickness=0)
        self.canvas.pack(fill="x")
        self.scrollbar = ttk.Scrollbar(hand, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=self.scrollbar.set)
        self.options = tk.Frame(self.canvas, bg="white")
        self.window = self.canvas.create_window((0, 0), window=self.options, anchor="n")
        self.options.bind("<Configure>", lambda _: self.canvas.configure(
            scrollregion=(0, 0, self.canvas.winfo_width(), self.options.winfo_reqheight())))
        self.canvas.bind("<Configure>", self.resize_hand)
        self.canvas.bind("<MouseWheel>", lambda e: self.canvas.yview_scroll(-int(e.delta/120), "units"))
        self.canvas.bind("<Button-4>", lambda _: self.canvas.yview_scroll(-1, "units"))
        self.canvas.bind("<Button-5>", lambda _: self.canvas.yview_scroll(1, "units"))
        controls = self.controls = tk.Frame(hand, bg="white")
        controls.pack(fill="x", pady=5)
        self.actions = tk.Frame(controls, bg="white")
        self.actions.pack(side="left")
        self.confirm_button = ttk.Button(controls, text="Confirm turn ↵", command=self.confirm)
        self.confirm_button.pack(side="right")
        self.confirm_button.state(["disabled"])
        self.details_button = ttk.Button(controls, text="View card", command=self.selected_detail)
        self.details_button.pack(side="right", padx=8)
        self.details_button.state(["disabled"])
        self.payment = tk.Label(hand, bg="white", anchor="w", font=("Verdana", 9), height=2, justify="left")
        self.payment.pack(fill="x")
        self.asset_note = tk.Label(root, bg="white", fg="#8c6630", anchor="w", font=("Verdana", 8))
        self.asset_note.grid(row=3, column=0, sticky="ew", padx=12)
        tk.Label(root, textvariable=self.status, bg="#f1f1ef", anchor="w", font=("Verdana", 8)).grid(
            row=4, column=0, sticky="ew", ipady=4)
        root.bind("<Return>", lambda _: self.confirm())
        root.bind("<Configure>", self.resize_table)
        root.protocol("WM_DELETE_WINDOW", self.close)
        self.timer = root.after(1, self.tick)

    def resize_table(self, event):
        if event.widget is self.root:
            for player in self.players:
                player.draw_summary()

    def resize_hand(self, event):
        self.canvas.coords(self.window, event.width/2, 0)
        if self.decision and self.visible_actor == self.decision["actor"] and self.card_widgets:
            self.layout_cards(event.width)
        else:
            self.canvas.itemconfigure(self.window, width=event.width)

    def clear_options(self):
        # A detail window may contain the outgoing human's private hand.
        if self.detail_window is not None and self.detail_window.winfo_exists():
            self.detail_window.destroy()
        for widget in self.options.winfo_children():
            widget.destroy()
        for widget in self.actions.winfo_children():
            widget.destroy()
        self.option_buttons, self.card_widgets, self.reveal_button = [], {}, None
        self.selected_card, self.selected_option, self.decision = None, None, None
        self.confirm_button.state(["disabled"])
        self.details_button.state(["disabled"])
        self.payment.configure(text="")
        self.scrollbar.pack_forget()
        self.canvas.yview_moveto(0)
        self.canvas.itemconfigure(self.window, width=self.canvas.winfo_width())

    def show_decision(self, decision):
        self.clear_options()
        self.decision = decision
        actor, kind = decision["actor"], decision["kind"]
        self.prompt.configure(text=f"Player {actor+1} · " + (
            "Discard pile – choose a building to construct for free" if kind == "discard" else
            "Seventh card · choose a card and action" if kind == "seventh" else
            "Hand · choose a card and action"))
        if self.visible_actor != actor:
            self.reveal_button = ttk.Button(self.options, text=f"Show hand for Player {actor+1}",
                                           command=lambda: self.reveal(decision))
            self.reveal_button.pack(pady=75)
            return
        cards = {}
        for option in decision["options"]:
            cards.setdefault(option["card"]["instance_id"], []).append(option)
        for instance, offered in cards.items():
            card = offered[0]["card"]
            spec = self.assets.cards[(card["type_id"], card["age"])]
            frame = tk.Frame(self.options, bg="white", highlightthickness=2,
                             highlightbackground=COLORS[spec["color"]], cursor="hand2",
                             width=HAND_CARD_WIDTH, height=242)
            frame.pack_propagate(False)
            title = tk.Label(frame, text=card["name"], bg="white", font=("Verdana", 9), wraplength=124, height=2)
            title.pack()
            photo = self.assets.photo("card", card["type_id"], card["age"], self.root)
            picture = tk.Label(frame, image=photo, bg="white", bd=0) if photo else tk.Label(
                frame, text=self.assets.card_text(card["type_id"], card["age"]), bg="#f3efe5",
                wraplength=132, width=18, height=12, font=("Verdana", 9), justify="left")
            picture.pack()
            for widget in (frame, title, picture):
                widget.bind("<Button-1>", lambda _, instance=instance: self.choose_card(instance))
                widget.bind("<Button-3>", lambda _, card=card: self.show_detail(card))
                widget.bind("<Double-Button-1>", lambda _, card=card: self.show_detail(card))
                widget.bind("<MouseWheel>", lambda e: self.canvas.yview_scroll(-1 if e.delta > 0 else 1, "units"))
                widget.bind("<Button-4>", lambda _: self.canvas.yview_scroll(-1, "units"))
                widget.bind("<Button-5>", lambda _: self.canvas.yview_scroll(1, "units"))
            self.card_widgets[instance] = (frame, offered)
        self.layout_cards(max(self.canvas.winfo_width(), 1000))
        self.payment.configure(text="Select a card, choose an action, then confirm your turn.")

    def layout_cards(self, width):
        cols = min(len(self.card_widgets), max(1, (width+HAND_CARD_GAP)//(HAND_CARD_WIDTH+HAND_CARD_GAP)))
        group_width = cols*HAND_CARD_WIDTH+(cols-1)*HAND_CARD_GAP
        self.canvas.coords(self.window, width/2, 0)
        self.canvas.itemconfigure(self.window, width=group_width)
        for i, (frame, _) in enumerate(self.card_widgets.values()):
            frame.grid(row=i//cols, column=i%cols, sticky="n",
                       padx=(0, HAND_CARD_GAP if i%cols < cols-1 else 0), pady=2)
        for i in range(12):
            self.options.columnconfigure(i, weight=0)
        if len(self.card_widgets) > cols:
            self.scrollbar.pack(side="right", fill="y", before=self.canvas)
        else:
            self.scrollbar.pack_forget()

    def reveal(self, decision):
        self.visible_actor = decision["actor"]
        self.show_decision(decision)

    def choose_card(self, instance):
        if self.session.pending or instance not in self.card_widgets:
            return
        previous = self.selected_option
        preferred = (previous["kind"], previous.get("free_source")) if previous else None
        self.selected_card = instance
        for identity, (frame, _) in self.card_widgets.items():
            frame.configure(highlightbackground="#278346" if identity == instance else "#cccccc",
                            highlightthickness=2)
        for child in self.actions.winfo_children():
            child.destroy()
        self.option_buttons = []
        offered = sorted(self.card_widgets[instance][1], key=lambda o: (
            list(ACTION).index(o["kind"]), bool(o.get("free_source"))))
        for option in offered:
            label = ACTION[option["kind"]]
            if option.get("free_source"):
                label += " · "+FREE[option["free_source"]]
            button = tk.Button(self.actions, text=label, font=("Verdana", 9), relief="raised",
                               command=lambda o=option: self.select(self.decision["actor"], o["option_id"]))
            button.pack(side="left", padx=(0, 5))
            if not option["selectable"]:
                button.configure(state="disabled")
            self.option_buttons.append((button, option))
        if self.decision["kind"] != "discard":
            for kind in ("build_structure", "build_wonder", "discard"):
                if not any(o["kind"] == kind for o in offered):
                    tk.Button(self.actions, text=ACTION[kind], font=("Verdana", 9),
                              state="disabled").pack(side="left", padx=(0, 5))
        legal = [o for o in offered if o["selectable"]]
        selected = next((o for o in legal if (o["kind"], o.get("free_source")) == preferred), next(iter(legal), None))
        self.selected_option = None
        self.details_button.state(["!disabled"])
        if selected:
            self.select(self.decision["actor"], selected["option_id"])
        else:
            self.confirm_button.state(["disabled"])
            self.payment.configure(text="You already have this building and cannot build it again.")

    def select(self, actor, option_id):
        if self.session.pending or not self.decision or actor != self.decision["actor"]:
            return
        option = next((o for _, o in self.option_buttons if o["option_id"] == option_id and o["selectable"]), None)
        if option is None:
            return
        self.selected_option = option
        for button, candidate in self.option_buttons:
            active = candidate["option_id"] == option_id
            button.configure(bg="#d1ead7" if active else "#eeeeee", relief="sunken" if active else "raised")
        self.confirm_button.state(["!disabled"])
        payment = option["payment"]
        total = payment["bank"] + sum(payment["to_seats"].values())
        parts = [f"Bank: {payment['bank']}"] if payment["bank"] else []
        for side, seat in self.session.state["players"][str(actor)]["neighbors"].items():
            amount = payment["to_seats"][str(seat)]
            if amount:
                parts.append(f"{'← left' if side == 'left' else 'right →'} (Player {seat+1}): {amount}")
        source = " · "+FREE[option["free_source"]] if option.get("free_source") else ""
        coins = "Income: 3 coins" if option["kind"] == "discard" else (
            f"Cost: {total} coins" + (" · "+" · ".join(parts) if parts else ""))
        self.payment.configure(text=f"{option['card']['name']} · {ACTION[option['kind']]}{source}\n{coins}")

    def confirm(self):
        if not self.selected_option or not self.decision or self.session.pending:
            return
        try:
            self.session.submit_choice(self.decision["actor"], self.selected_option["option_id"])
            self.clear_options()
            self.prompt.configure(text="Action submitted …")
        except (ValueError, RuntimeError) as error:
            self.fail(error)

    def selected_detail(self):
        if self.selected_card in self.card_widgets:
            self.show_detail(self.card_widgets[self.selected_card][1][0]["card"])

    def show_detail(self, card):
        if self.detail_window is not None and self.detail_window.winfo_exists():
            self.detail_window.destroy()
        win = self.detail_window = tk.Toplevel(self.root)
        win.title(card["name"])
        win.configure(bg="white")
        win.transient(self.root)
        tk.Label(win, text=f"{card['name']} · Age {card['age']}", bg="white", font=("Verdana", 12, "bold")).pack(padx=14, pady=10)
        picture = self.assets.photo("detail", card["type_id"], card["age"], self.root)
        if picture:
            tk.Label(win, image=picture, bg="white").pack()
        text = self.assets.card_text(card["type_id"], card["age"])
        chains = self.assets.chain_text(card["type_id"], card["age"])
        tk.Label(win, text=text+("\n"+chains if chains else ""), wraplength=350,
                 bg="white", justify="left", font=("Verdana", 10)).pack(padx=14, pady=12)
        ttk.Button(win, text="Close", command=win.destroy).pack(pady=(0, 12))
        win.bind("<Escape>", lambda _: win.destroy())

    def render(self, state):
        if state["engine"]["card_data"]["source_sha256"] != self.assets.source_hash:
            raise ValueError("Card data does not match the engine")
        game = state["game"]
        finished = game["phase"] in {"finished", "aborted"}
        self.heading.configure(text="Game over" if game["phase"] == "finished" else
                               "Game aborted" if game["phase"] == "aborted" else
                               f"Age {game['age']} · Turn {game['round_in_age']}")
        self.direction.configure(text="Final scores" if finished else "Pass cards: "+(
            "right →" if game["age"] == 2 else "← left"))
        decision = self.session.human_decision() if not self.session.pending else None
        if decision:
            self.focus_seat = decision["actor"]
        elif self.focus_seat is None:
            self.focus_seat = self.session.bots if self.session.bots < 3 else 1
        neighbors = state["players"][str(self.focus_seat)]["neighbors"]
        order = (neighbors["left"], self.focus_seat, neighbors["right"])
        for column, seat in enumerate(order):
            self.players[seat].frame.grid_configure(column=column)
        for seat, player in enumerate(self.players):
            role = ("You" if self.session.bots < 3 else "Spectating") if seat == self.focus_seat else (
                "Left neighbor" if seat == neighbors["left"] else "Right neighbor")
            player.render(state["players"][str(seat)], bool(decision and decision["actor"] == seat),
                          role=role, human=seat >= self.session.bots)
        self.asset_note.configure(text="Artwork is missing or unverified: current card values are shown as text." if self.assets.missing else "")
        if finished:
            self.clear_options()
            self.canvas.pack_forget()
            self.controls.pack_forget()
            self.payment.pack_forget()
            self.prompt.configure(text="Final scores: " + " · ".join(
                f"Player {s+1}: {state['players'][str(s)]['points']['total']} points" for s in range(3))
                if game["phase"] == "finished" else "The game was aborted.")
        elif decision:
            key = (decision["decision_id"], decision["offered_revision"])
            if key != self.decision_key:
                self.decision_key = key
                self.show_decision(decision)

    def tick(self):
        if self.closed:
            return
        try:
            state = self.session.poll()
            if not self.session.pending and self.session.drive_bots():
                self.clear_options()
                self.prompt.configure(text="Other players are choosing …")
            if state is not None:
                self.render(state)
        except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
            self.fail(error)
        if not self.closed and self.error is None:
            self.timer = self.root.after(15, self.tick)

    def fail(self, error):
        self.error = str(error)
        self.clear_options()
        self.prompt.configure(text=f"Game interrupted: {error}")

    def close(self):
        if self.closed:
            return
        self.closed = True
        self.root.after_cancel(self.timer)
        self.session.close()
        self.root.destroy()


def run(session, assets_dir=None):
    root = tk.Tk()
    DecisionApp(root, session, assets_dir)
    root.mainloop()
