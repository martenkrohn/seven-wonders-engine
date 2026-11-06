"""Graphical cities: wonder, stage markers, symbols and built effect strips."""
import tkinter as tk
from tkinter import ttk

from .bga_assets import RESOURCE

COLORS = {"brown": "#90643f", "grey": "#757b80", "blue": "#2877aa",
          "yellow": "#ce9b19", "red": "#ae4540", "green": "#418354", "purple": "#805797"}
CITY_COLUMNS = (("Production", ("brown", "grey")), ("Blue", ("blue",)),
                ("Commerce", ("yellow",)), ("Military", ("red",)),
                ("Science", ("green",)), ("Guilds", ("purple",)))
SCORES = (("civilian", "Blue", "#2877aa", "III"),
          ("commercial", "Commerce and coins", "#ce9b19", "$"),
          ("military", "Military", "#ae4540", "⚔"),
          ("scientific", "Science", "#418354", "⚙"),
          ("guild", "Guilds", "#805797", "G"),
          ("wonder", "Wonder", "#977343", "▲"),
          ("total", "Total", "#333333", "Σ"))

# Centers on the 450 × 223 boards. Equal spacing misses two-stage Babylon.
STAGE_CENTERS = {0: (83, 223, 365), 1: (83, 223, 365), 2: (83, 223, 365),
                 3: (83, 223, 365), 4: (83, 223, 365), 5: (83, 223, 365),
                 6: (83, 223, 365), 7: (49, 164, 285, 404), 8: (244, 385),
                 9: (83, 223, 365), 10: (83, 223), 11: (83, 223, 365),
                 12: (83, 223, 365), 13: (83, 223, 365)}


class Tableau:
    def __init__(self, parent, seat, assets, detail, status):
        self.seat, self.assets, self.detail, self.status = seat, assets, detail, status
        self.state, self.active = None, False
        self.role, self.human = "", False
        self.frame = tk.Frame(parent, bg="white", highlightbackground="#c9c9c9", highlightthickness=1)
        self.frame.grid(row=0, column=seat, sticky="nsew", padx=4)
        self.summary = tk.Canvas(self.frame, bg="white", height=280, highlightthickness=0)
        self.summary.pack(fill="x")
        body = tk.Frame(self.frame, bg="white")
        body.pack(fill="both", expand=True)
        self.city = tk.Canvas(body, bg="white", height=190, highlightthickness=0)
        scroll = ttk.Scrollbar(body, orient="vertical", command=self.city.yview)
        self.city.configure(yscrollcommand=scroll.set)
        scroll.pack(side="right", fill="y")
        self.city.pack(side="left", fill="both", expand=True)
        self.summary.bind("<Configure>", lambda _: self.draw_summary())
        self.city.bind("<Configure>", lambda _: self.draw_city())
        self.city.bind("<MouseWheel>", lambda e: self.city.yview_scroll(-int(e.delta/120), "units"))
        self.city.bind("<Button-4>", lambda _: self.city.yview_scroll(-1, "units"))
        self.city.bind("<Button-5>", lambda _: self.city.yview_scroll(1, "units"))

    def hint(self, canvas, tag, text):
        canvas.tag_bind(tag, "<Enter>", lambda _: self.status.set(text))
        canvas.tag_bind(tag, "<Leave>", lambda _: self.status.set("Click a building to view its full card and construction chains."))

    def render(self, player, active=False, role="", human=False):
        changed = self.state != player
        if changed or (self.active, self.role, self.human) != (active, role, human):
            self.state, self.active = player, active
            self.role, self.human = role, human
            self.frame.configure(highlightbackground="#32864d" if active else "#c9c9c9")
            self.draw_summary()
        if changed:
            self.draw_city()

    def draw_summary(self):
        if self.state is None:
            return
        p, c = self.state, self.summary
        c.delete("all")
        width = max(300, c.winfo_width())
        c.create_text(width/2, 14, text=f"Player {self.seat+1} · {self.role}", font=("Verdana", 10, "bold"), tags="seat-title")
        c.create_text(width/2, 32, text=self.assets.wonder_name(p["wonder_id"])+" · "+("Human" if self.human else "Bot"),
                      font=("Verdana", 9), tags="wonder-title")
        bw = min(250 if c.winfo_toplevel().winfo_height() < 880 else 300, width-30)
        bh = round(bw*223/450)
        x, y = (width-bw)/2, 46
        picture = self.assets.photo("wonder", p["wonder_id"], None, c.winfo_toplevel(), (bw, bh))
        if picture:
            c.create_image(x, y, image=picture, anchor="nw")
        else:
            c.create_rectangle(x, y, x+bw, y+bh, fill="#eee6d6", outline="#b7aa8b")
            c.create_text(width/2, y+bh/2, text=self.assets.wonder_name(p["wonder_id"]), font=("Verdana", 14))
        centers = STAGE_CENTERS[p["wonder_id"]]
        spec = self.assets.wonders[p["wonder_id"]]
        for i, center in enumerate(centers):
            cx = x+bw*center/450
            stage_tag = f"stage-{i}"
            built = i < p["wonder_stage"]
            c.create_rectangle(cx-15, y+bh-1, cx+15, y+bh+13,
                               fill="#70624b" if built else "#f7f5f0", outline="#a79b86", tags=stage_tag)
            c.create_text(cx, y+bh+6, text="✓" if built else str(i+1),
                          fill="white" if built else "#817967", font=("Verdana", 8, "bold"), tags=stage_tag)
            stage = spec["stages"][i]
            cost = ", ".join(f"{n} {RESOURCE[r]}" for r, n in stage["cost"].items())
            self.hint(c, stage_tag, f"Stage {i+1}: {cost} · {'built' if built else 'not built'}")
        row = y+bh+32
        resources = ["coins", "shields"]
        for i, resource in enumerate(resources):
            cx = width/2+(i-.5)*85
            tag = "resource-"+resource
            icon = self.assets.icon(resource, c.winfo_toplevel())
            if icon:
                c.create_image(cx, row, image=icon, tags=tag)
            elif resource == "shields":
                c.create_oval(cx-10, row-10, cx+10, row+10,
                              fill="#b92738", outline="#73342f", tags=tag)
            else:
                c.create_text(cx, row, text=RESOURCE.get(resource, resource)[:2], font=("Verdana", 8), tags=tag)
            c.create_text(cx+20, row, text=p["resources"][resource], anchor="w", font=("Verdana", 11), tags=tag)
            hint = "Shields" if resource == "shields" else RESOURCE[resource]
            self.hint(c, tag, hint)
        row += 33
        for i, (key, name, color, symbol) in enumerate(SCORES):
            cx = 10+(width-20)/7*(i+.5)
            tag = "score-"+key
            c.create_oval(cx-10, row-10, cx+10, row+10, fill=color, outline="", tags=tag)
            c.create_text(cx, row, text=symbol, fill="white", font=("Verdana", 9, "bold"), tags=tag)
            c.create_text(cx, row+19, text=p["points"][key], font=("Verdana", 10, "bold" if key == "total" else "normal"), tags=tag)
            self.hint(c, tag, name+": "+str(p["points"][key])+" points")
        c.configure(height=row+36)

    def draw_city(self):
        if self.state is None:
            return
        c = self.city
        c.delete("all")
        width = max(280, c.winfo_width())
        cols = len(CITY_COLUMNS)
        cell = width/cols
        card_width = min(160, int(cell)-3)
        card_height = round(card_width*84/255)
        row_height = card_height+3
        cards = self.state["built_cards"]
        positions = []
        bottom = 60
        for column, (_, colors) in enumerate(CITY_COLUMNS):
            x = column*cell+1
            y = 2
            if column == 0:
                # Starting production belongs to the wonder, never to built_cards.
                resource = self.assets.wonders[self.state["wonder_id"]]["starting_resource"]
                height = max(24, card_height)
                tag = "wonder-production"
                c.create_rectangle(x, y, x+card_width, y+height, fill="#f1ecdf",
                                   outline="#b29a62", tags=tag)
                icon = self.assets.icon(resource, c.winfo_toplevel())
                if icon:
                    c.create_image(x+card_width/2, y+height/2, image=icon, tags=tag)
                else:
                    c.create_text(x+card_width/2, y+height/2, text="1 "+RESOURCE[resource],
                                  font=("Verdana", 7), tags=tag)
                # A small pyramid matches the wonder symbol used in the score row.
                c.create_polygon(x+card_width-12, y+10, x+card_width-7, y+3,
                                 x+card_width-2, y+10, fill="#977343", outline="",
                                 tags=(tag, "wonder-production-marker"))
                self.hint(c, tag, f"Wonder production: 1 {RESOURCE[resource]} · "
                          + self.assets.wonder_name(self.state["wonder_id"]) + " (not a building).")
                y += height+6
            for color in colors:
                group = [card for card in cards if self.assets.cards[(card["type_id"], card["age"])]["color"] == color]
                group.sort(key=lambda card: (str(self.assets.cards[(card["type_id"], card["age"])]["effects"].get("science", "")),
                                             card["type_id"], card["age"]))
                if color == "grey" and group and y > 2:
                    y += 6
                for card in group:
                    positions.append((card, x, y))
                    y += row_height
            bottom = max(bottom, y+4)
        for card, x, y in positions:
            tag = "card-"+card["instance_id"]
            spec = self.assets.cards[(card["type_id"], card["age"])]
            c.create_rectangle(x, y, x+card_width, y+card_height, outline=COLORS[spec["color"]], tags=tag)
            photo = self.assets.photo("built", card["type_id"], card["age"], c.winfo_toplevel(), (card_width, card_height))
            if photo:
                c.create_image(x+card_width/2, y, image=photo, anchor="n", tags=tag)
            else:
                text = self.assets.card_text(card["type_id"], card["age"]).split("\n", 1)[1].replace("\n", " · ")
                if c.tk.call("font", "measure", ("Verdana", 7), text) > card_width-6:
                    while text and c.tk.call("font", "measure", ("Verdana", 7), text+"…") > card_width-6:
                        text = text[:-1]
                    text += "…"
                c.create_text(x+card_width/2, y+card_height/2, text=text, font=("Verdana", 7), tags=tag)
            c.tag_bind(tag, "<Button-1>", lambda _, card=card: self.detail(card))
            self.hint(c, tag, card["name"]+" · "+self.assets.card_text(card["type_id"], card["age"]).replace("\n", " · "))
        if not cards:
            c.create_text(width/2, 91, text="No buildings yet", fill="#888888", font=("Verdana", 9))
            bottom = 110
        c.configure(scrollregion=(0, 0, width, bottom))
