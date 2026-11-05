# Dateiprotokoll 3 – implementierter Entwicklungsstand

Stand: 5. November 2026, Paket 5-I. Vertragskennung **`decision-v1`**.
`engine-contract.json`: Transportversion **3**, Release `v0.2.0-dev`,
Regeln `bga-second-edition-development`, `second_edition_implemented=false`.
Die Transportversion bezeichnet keine fertige BGA-Edition. Kartentabellen
entsprechen den geprüften Zielen; Ludus-/Decorators-Effekte sind noch offen,
Wunderfähigkeiten verwenden weiterhin die alten Definitionen.
`engine.card_data` nennt Formatversion, SHA-256 der Zielquelle,
`tables_complete=true`, `effects_complete=false` und `deferred_effect_ids=[76,78]`.
Die zusätzliche Capability heißt `card_tables_v1`; sie bestätigt keine
vollständigen Kartenregeln. Einzelheiten: [CARD_DATA.md](CARD_DATA.md).

Der vollständige Fachvertrag steht in [DECISION_PROTOCOL_DRAFT.md](DECISION_PROTOCOL_DRAFT.md),
die Ausführung und Grenzen in [SIMULTANEOUS_RESOLUTION.md](SIMULTANEOUS_RESOLUTION.md).
Die dortigen alten `decision-v1-draft`-Beispiele bleiben als Spezifikationsnachweis
markiert; das Binary meldet **`decision-v1`** und ergänzt die unten beschriebenen
Produktions-/Transportnachweise. [Protokoll 2](PROTOCOL_V2.md) ist archiviert.
Es gibt keinen V2-Modus; `ready.txt` und `player_1.json` bis `player_3.json`
werden nicht gelesen. Die alte GUI/RL-Anbindung benötigt ihre späteren Pakete.

## Start und nutzbarer Client

Eine Partie benötigt einen isolierten Arbeitsordner mit `io/` und `logs/`.
Der eigenständige Standardbibliotheksclient erzeugt ihn selbst:

```sh
python3 scripts/smoke_protocol.py --engine build/7Wonders --seed 8
```

Er spielt alle angebotenen regulären, siebten und Ablageentscheidungen, prüft
Kartenerhaltung, Quittierungen und Schlussresultat. Direkter Engine-Aufruf:

```sh
/path/to/7Wonders --seed 42 --poll-us 1000 --decision-timeout-ms 30000 --terminal-grace-ms 100
```

`--decision-timeout-ms 0` bedeutet unbegrenzte Entscheidungszeit (Standard).
`--terminal-grace-ms` hält den Prozess nach Abschluss für Wiederzustellungen
offen (Standard 100 ms). Beide Werte reichen von 0 bis 3.600.000 ms.
`--poll-us` reicht von 0 bis 1.000.000, `--quiet` unterdrückt stdout.
Normaler Abschluss liefert Exitcode 0, kontrollierter Spielabbruch 2;
Datei-/Startfehler melden sich auf stderr mit Exitcode 1.

## Austausch und Quittierung

1. Auf atomar veröffentlichtes `io/game_status.json` warten, Version und
   Vertragskennung prüfen. `game_id` ist je Prozess neu und unabhängig vom Seed.
2. Genau die eigenen offenen Entscheidungen aus `decisions` bedienen. `waiting_for`
   listet deren Sitze. Keine Eingabe für einen nicht beteiligten Sitz erfinden.
3. Das Bundle atomar nach `io/commands.json` schreiben, danach atomar nach
   `io/ready.json` signalisieren. Beide Dateien müssen Version, Partie und
   Bundlekennung teilen. Es darf nur eine Zustellung gleichzeitig unterwegs sein.
4. Auf `transport.batch_id` und die Entfernung von `ready.json` warten. Die
   Engine veröffentlicht die Quittierung, bevor sie das Signal freigibt.
   Die Engine bestätigt jeden Einzelrequest; ein zugestelltes Bundle ist kein
   Beweis, dass alle darin enthaltenen Spieleingaben gültig waren.
5. Danach aktuelle Entscheidungen und `revision` lesen. Zusatzwahlen können
   denselben `game.turn` wie die reguläre Aktion haben. Ohne siebte Karte werden
   die Slots 6/13/20 übersprungen. Nicht auf `turn+1` warten.

Beispiel für ein Bundle; Kennungen und Option immer aus dem echten Angebot lesen:

```json
{
  "protocol_version": 3,
  "game_id": "PARTIE",
  "batch_id": "bundle-1",
  "requests": [{
    "protocol_version": 3,
    "game_id": "PARTIE",
    "request_id": "eingabe-1",
    "actor": 0,
    "decision_id": "ENTSCHEIDUNG",
    "offered_revision": 1,
    "action": {
      "kind": "build_structure",
      "option_id": "OPTION",
      "card_instance_id": "17"
    }
  }]
}
```

Passendes `ready.json`:

```json
{"protocol_version":3,"game_id":"PARTIE","batch_id":"bundle-1"}
```

Eine Karteninstanz ist ein opaker String, kein Typ oder Vektorindex. Optionen
binden Handlung, Karte, Freibaumodus und festen Zahlungsplan. Zahlungen oder
weitere Parameter in einer Eingabe werden abgelehnt. `production_allocation`
belegt je Option Quelle, Ressourcenindex und Menge; Ressourcenindizes entsprechen
`include/resources.h`. Die kanonische Wahl minimiert Gesamtausgabe, dann Zahlung
an links, dann rechts, dann die lexikografische Quellenbelegung. Links ist
`(s+2)%3`, rechts `(s+1)%3`; alte interne Ost-/West-Flags folgen diesen konkret
ausgewiesenen Empfängersitzen.

`receipts` ist das dauerhafte Request-Ledger: zunächst `accepted`, nach dem
regulären Commit `applied`, bei vorherigem Abbruch `cancelled`. Nach Annahme
ist die Entscheidung gesperrt. Die Annahme eines anderen Sitzes ändert die
angebotene `offered_revision` der übrigen Sitze nicht. Eine identische
Wiederholung liefert den aktuellsten gespeicherten Receipt zurück, ohne neue
Revision oder Wirkung. Bei anderer Nutzlast unter derselben Requestkennung
entsteht `request_id_conflict`; der ursprüngliche Ledger-Eintrag bleibt erhalten.

`transport.responses` enthält die Antworten auf die letzte Bundlezustellung.
Darin können bei späteren Statusständen noch Antworten vom damaligen Stand
stehen; für den inzwischen fortgeschrittenen Erfolg gilt das aktuelle Ledger.
`request_errors` enthält Hüllen-/Partie-/Identitätsfehler außerhalb des Ledgers,
insbesondere Konfliktantworten, die den Originalrequest nicht überschreiben.
Ein Bundlekonflikt steht als `transport.code=batch_id_conflict` fest, bevor
neue Requests angenommen werden. Transportbestätigungen allein können sich
bei unveränderter Spielrevision ändern. Für eine reine Request-Wiederholung
in einer neuen Zustellung eine neue `batch_id` mit denselben Requests verwenden;
ein unverändertes altes Bundle kann ebenfalls erneut zugestellt werden.

Unvollständige JSON-Dateien, abweichende Bundlekennungen und nicht passende
Ready-Hüllen werden nicht verarbeitet. Es gibt keine Ausweichaktion; die
bestehenden Engine-Fristen laufen weiter. Diese lokalen Dateien sind keine
Remote-Authentisierungsgrenze und keine Wiederaufnahme nach Prozessabsturz.

## Ablagewahl, Ende und Abbruch

`discard_choice` enthält genau eine aktive Entscheidung. `can_skip=false`:
Bei legaler Option muss gebaut werden. Eigene Gebäudeduplikate bleiben sichtbar,
aber unselektierbar. Ohne legale Option erledigt die Engine den Grant automatisch
mit `skipped_no_options`. Noch nicht aufgelöste reguläre Wahlen bleiben verdeckt;
die Ablage ist nur im Angebot des berechtigten Sitzes vollständig sichtbar.
Der vertrauenswürdige Koordinator erhält alle Hände; spätere Policy-/GUI-/RL-
Adapter müssen die vertraglich beschriebenen Sitzansichten daraus bilden.

Deadlines beginnen erst mit der ersten Veröffentlichung des jeweiligen
Angebots und werden durch falsche oder wiederholte Nachrichten nicht verlängert.
Ein expliziter Abbruch ist ein normal zugestellter Kontrollrequest:

```json
{"protocol_version":3,"game_id":"PARTIE","request_id":"abbruch-1","control":"abort_game","reason":"window_closed"}
```

`aborted` bewahrt den letzten konsistenten Spielzustand, schließt ausstehende
Entscheidungen/Grants und enthält `result=null`, `scores_final=false` sowie
`abort_reason`. Es gibt kein Siegerresultat und keine `results.txt`.
Ein Timeout erzeugt `decision_timeout`, ein interner Transaktionsfehler
`resolution_error`. Der Adapter darf daraus keinen normalen Terminalreward machen.

Nur `finished` veröffentlicht `age=4`, `turn=21`, `scores_final=true` und
`result.totals`. Die Summen stimmen mit den Punktekategorien der Spieler überein.
`results.txt` wird anschließend genau einmal atomar geschrieben. Militär,
Gildenkopie und Endwertung erfolgen erst nach allen Pflichtentscheidungen.
`copied_guilds` sind abgeleitete Effektreferenzen auf eine fremde physische Karte;
sie werden nicht als zweite physische Karte in `built_cards` ausgegeben.
Der bisherige Gildenkopie-Algorithmus und seine bekannten Grenzen bleiben erhalten.
