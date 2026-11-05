"""Tk interface driven exclusively by the current offered options."""
import tkinter as tk
from tkinter import ttk
from .decision_session import DecisionSession
from .bga_assets import BgaAssets, RESOURCE

ACTION = {"build_structure": "Bauen", "build_wonder": "Wunderstufe",
          "discard": "Verkaufen (+3)", "build_discard_free": "Ablage: kostenlos bauen"}
FREE = {"chain": "Baukette", "olympia_first_color": "Erste Farbe",
        "olympia_first_age_card": "Erste Ärenkarte", "olympia_last_age_card": "Letzte Ärenkarte",
        "discard_grant": "Ablagewahl"}


class DecisionApp:
    def __init__(self, root, session, assets_dir=None):
        self.root, self.session, self.assets_dir = root, session, assets_dir
        self.assets = BgaAssets(assets_dir)
        self.closed, self.error, self.visible_actor = False, None, None
        self.decision_key, self.option_buttons, self.reveal_button = None, [], None
        root.title("7 Wonders – BGA-Edition")
        root.geometry("1280x900")
        root.minsize(1000, 760)
        self.heading = ttk.Label(root, font=("TkDefaultFont", 15, "bold"))
        self.heading.pack(pady=8)
        table = ttk.Frame(root)
        table.pack(fill="x", padx=12)
        self.players = []
        for seat in range(3):
            frame = ttk.LabelFrame(table, text=f"Spieler {seat+1}", padding=8)
            frame.grid(row=0, column=seat, sticky="nsew", padx=4)
            table.columnconfigure(seat, weight=1)
            label = ttk.Label(frame, justify="left", wraplength=380)
            picture = ttk.Label(frame)
            picture.pack()
            label.pack(anchor="w")
            city = ttk.Treeview(frame, columns=("age",), height=3)
            city.heading("#0", text="Öffentliche Gebäude")
            city.heading("age", text="Ära")
            city.column("#0", width=250)
            city.column("age", width=35, stretch=False)
            city.pack(fill="x", pady=4)
            self.players.append((frame, label, picture, city))
        self.asset_note = ttk.Label(root)
        self.asset_note.pack()
        self.prompt = ttk.Label(root, font=("TkDefaultFont", 12))
        self.prompt.pack(pady=8)
        self.canvas = tk.Canvas(root, highlightthickness=0)
        scrollbar = ttk.Scrollbar(root, orient="vertical", command=self.canvas.yview)
        self.canvas.configure(yscrollcommand=scrollbar.set)
        scrollbar.pack(side="right", fill="y")
        self.canvas.pack(fill="both", expand=True, padx=12)
        self.options = ttk.Frame(self.canvas)
        self.window = self.canvas.create_window((0, 0), window=self.options, anchor="nw")
        self.options.bind("<Configure>", lambda _: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas.bind("<Configure>", lambda e: self.canvas.itemconfigure(self.window, width=e.width))
        root.protocol("WM_DELETE_WINDOW", self.close)
        self.timer = root.after(1, self.tick)

    def clear_options(self):
        for widget in self.options.winfo_children():
            widget.destroy()
        self.option_buttons, self.reveal_button = [], None

    def show_decision(self, decision):
        self.clear_options()
        actor = decision["actor"]
        kind = decision["kind"]
        self.prompt.configure(text=f"Spieler {actor+1}: " + (
            "Pflichtwahl aus der Ablage" if kind == "discard" else
            "Siebte Karte" if kind == "seventh" else "Karte und Aktion wählen"))
        if self.visible_actor != actor:
            self.reveal_button = ttk.Button(self.options, text=f"Hand für Spieler {actor+1} anzeigen",
                command=lambda: self.reveal(decision))
            self.reveal_button.pack(pady=20)
            return
        cards = {}
        for option in decision["options"]:
            cards.setdefault(option["card"]["instance_id"], []).append(option)
        for index, options in enumerate(cards.values()):
            card = options[0]["card"]
            frame = ttk.LabelFrame(self.options, text=f"{card['name']} · Ära {card['age']}", padding=6)
            frame.grid(row=index//5, column=index%5, sticky="nsew", padx=4, pady=6)
            photo = self.assets.photo("card", card["type_id"], card["age"], self.root)
            if photo is not None:
                ttk.Label(frame, image=photo).pack()
            # Names and values are always visible, including sprites without names.
            ttk.Label(frame, text=self.assets.card_text(card["type_id"], card["age"]),
                      wraplength=210, justify="left").pack(pady=4)
            for option in options:
                label = ACTION[option["kind"]]
                if option.get("free_source") and option["free_source"] != "discard_grant":
                    label += " · " + FREE.get(option["free_source"], "kostenlos")
                payment = option["payment"]
                total = payment["bank"] + sum(payment["to_seats"].values())
                if total:
                    label += f" · {total} Münzen"
                    details = ([f"Bank {payment['bank']}"] if payment["bank"] else [])
                    details += [f"Spieler {int(s)+1}: {n}" for s, n in payment["to_seats"].items() if n]
                    ttk.Label(frame, text=" · ".join(details), wraplength=210).pack()
                button = ttk.Button(frame, text=label, command=lambda o=option: self.select(actor, o["option_id"]))
                button.pack(fill="x", pady=2)
                if not option["selectable"]:
                    button.state(["disabled"])
                self.option_buttons.append((button, option))
        for col in range(5):
            self.options.columnconfigure(col, weight=1)

    def reveal(self, decision):
        self.visible_actor = decision["actor"]
        self.show_decision(decision)

    def select(self, actor, option):
        try:
            self.session.submit_choice(actor, option)
            self.clear_options()
            self.prompt.configure(text="Aktion abgegeben …")
        except (ValueError, RuntimeError) as error:
            self.fail(error)

    def render(self, state):
        if state["engine"]["card_data"]["source_sha256"] != self.assets.source_hash:
            raise ValueError("Die Kartendaten passen nicht zur Engine")
        game = state["game"]
        self.heading.configure(text="Spielende" if game["phase"] == "finished" else
                               "Partie abgebrochen" if game["phase"] == "aborted" else
                               f"Ära {game['age']} · Zug {game['round_in_age']}")
        for seat, (_, label, picture, city) in enumerate(self.players):
            p = state["players"][str(seat)]
            photo = self.assets.photo("wonder", p["wonder_id"], None, self.root)
            picture.configure(image=photo or "")
            city.delete(*city.get_children())
            for card in p["built_cards"]:
                city.insert("", "end", text=card["name"], values=(card["age"],))
            label.configure(text=f"{p['wonder_name']} · Stufen {p['wonder_stage']}/{p['wonder_stage_count']}\n"
                f"Münzen: {p['resources']['coins']} · Schilde: {p['resources']['shields']}\n"
                + " · ".join(f"{RESOURCE[r]} {p['resources'][r]}" for r in list(RESOURCE)[:7]) + "\n"
                f"Punkte: {p['points']['total']} · Gebäude: {len(p['built_cards'])}\n"
                + " · ".join(f"{name}: {p['points'][key]}" for key, name in (
                    ("civilian","Blau"),("commercial","Gelb/Münzen"),("guild","Gilden"),
                    ("scientific","Wissenschaft"),("military","Militär"),("wonder","Wunder"))))
        self.asset_note.configure(text="Grafiken fehlen: Alle Kartenwerte bleiben als Text sichtbar." if self.assets.missing else "")
        if game["phase"] in {"finished", "aborted"}:
            self.clear_options()
            self.prompt.configure(text="Endwertung: " + " · ".join(
                f"Spieler {s+1}: {state['players'][str(s)]['points']['total']}" for s in range(3))
                if game["phase"] == "finished" else "Die Partie wurde beendet.")
        elif not self.session.pending:
            decision = self.session.human_decision()
            if decision:
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
                self.prompt.configure(text="Mitspieler wählen …")
            if state is not None:
                self.render(state)
        except (ValueError, RuntimeError, OSError, KeyError, TypeError) as error:
            self.fail(error)
        if not self.closed and self.error is None:
            self.timer = self.root.after(10, self.tick)

    def fail(self, error):
        self.error = str(error)
        self.clear_options()
        self.prompt.configure(text=f"Partie unterbrochen: {error}")

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
