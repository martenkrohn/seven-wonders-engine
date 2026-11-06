# BGA-GUI mit angebotenen Entscheidungen

Die gesamte aktive Oberfläche verwendet Englisch, einschließlich Kartendetails,
Wundernamen und Tag-/Nachtseiten, Sonderaktionen und Fehlermeldungen.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
python3 -m pip install -e '.[gui]'
python3 Game.py 2
```

Tk muss installiert sein (unter Ubuntu `python3-tk`). `0` Bots bedeutet drei
lokale Menschen, `2` einen Menschen als Spieler 3, `3` eine beobachtete Botpartie.
`--seed`, `--engine`, `--timeout` und `--assets-dir` sind optional.

Die Oberfläche verwendet die tatsächlichen angebotenen Optionen und bindet
physische Karteninstanz, Entscheidung und Revision. Drei grafische Spielerbereiche
zeigen Wunder, gebaute Stufen, Vermögen, Schilde und Punktesymbole sowie gebaute
Gebäude als kompakte Bildstreifen. Der aktive menschliche Spieler steht immer
in der Mitte und ist mit **You** gekennzeichnet; die anderen Bereiche heißen
**Left neighbor** und **Right neighbor** und folgen den Nachbarsitzen der
Engine. Bei mehreren Menschen dreht sich diese Perspektive mit der Übergabe.

Gebäude verwenden die vollständige obere Effektleiste der neuen Karten ohne
Illustration. Sie stehen in sechs festen Spalten: Produktion, Blau, Handel,
Militär, Wissenschaft und Gilden. Die Produktionsspalte enthält zuerst eine
braune und darunter eine getrennte graue Gruppe mit kleinem Abstand.
Ganz oben in dieser Spalte steht immer das Startgut des Wunders als eigener
Symbolstreifen mit einer kleinen Pyramidenmarkierung. Der englische Hinweis
„Wonder production“ nennt die Quelle; diese Produktion zählt nicht als Gebäude.
Spalten-/Gruppenüberschriften und Gebäudenamen entfallen: Mit vorhandenen
Grafiken sind ausschließlich die Effektbilder sichtbar. Innerhalb der Wissenschaft
wird nach Symbol sortiert. Alle Farbspalten bleiben auch bei kleinen Fenstern
nebeneinander; vollständige Kartendetails sind per Klick erreichbar.
Große Städte sind vertikal scrollbar.

Darunter stehen die vollständigen Hochkantkarten der aktiven Hand als
zentrierte Reihe mit festen Abständen von 10 Pixeln zwischen den Kartenrahmen.
Weniger Karten bleiben als kompakte Gruppe mittig; der Abstand wächst nicht.
Karte
anklicken, eine Aktion beziehungsweise einen Freibaumodus wählen und erst
dann **Confirm turn** oder die Eingabetaste drücken. Vor der Bestätigung
werden Kosten und Zahlungsempfänger angezeigt; die Wahl kann geändert werden.
Die Bestätigung bindet die tatsächlich angebotene Option. Normale Aktionen,
siebte Karte und Pflicht-Ablagewahl verwenden dieselbe Kartenansicht; große
Ablageangebote umbrechen und sind scrollbar. Unzulässige Duplikate bleiben
sichtbar und gesperrt. Nicht verfügbare normale Aktionen sind ausgegraut.

Ein Klick auf ein gebautes Gebäude, ein Rechts-/Doppelklick auf eine Handkarte
oder **View card** öffnet die vollständige Karte mit aktuellen Werten und
Bauketten. Mauszeiger über Münzen, Schilden, Punktesymbolen und Stufen zeigt
deren Bedeutung in der unteren Hinweiszeile. Die sieben Produktionszähler
werden nicht zusätzlich angezeigt; Produktion bleibt auf den Karten und
Wunderstufen sichtbar.

Bei mehreren Menschen wird die nächste Hand erst nach expliziter Übergabe
angezeigt; auch offene Kartendetails schließen bei Übergabe. Öffentliche Städte,
Vermögen, Schilde, alle Punktekategorien und die
abschließende Wertung bleiben sichtbar. Die Ereignisschleife wartet nicht
blockierend auf den Spielprozess; Schließen räumt den Prozess und seine
isolierten Austauschdateien auf.
Bei Spielende wird der Handbereich eingeklappt, sodass die öffentlichen Städte
mehr Platz für die abschließende Übersicht erhalten.

Standardgröße: 1320 × 940, Mindestgröße: 1120 × 800 Pixel. Die Wunderansicht
passt ihre Größe bei kleineren Fenstern an. Sieben vollständige Handkarten
passen auch bei Mindestgröße in eine Reihe. Die Stufenmarker berücksichtigen
alle 14 Wunderseiten mit zwei, drei oder vier Stufen.

Grafiken bleiben extern. Im gemeinsamen Workspace wird die vorhandene BGA-
Referenz automatisch gefunden; andernfalls beispielsweise:

```sh
python3 Game.py 2 --assets-dir /pfad/bga-sevenwonders
```

Benötigt werden `img/cards_v2.jpg` und `img/boards_v2.jpg` (tatsächlich WebP).
`img/icons_v2.png` liefert das Münzsymbol; ohne diese Datei wird das Vermögen
beschriftet dargestellt.
Die Checksummen des eingefrorenen Manifests verhindern, dass alte Bilder als
neue Kartenwerte ausgegeben werden. Die Zuschnitte verwenden die expliziten
Sprite-Rechtecke einschließlich Wunder-ID 0 und grauer Altersvarianten.
Kartennamen stehen an den Handkarten; gebaute Gebäude zeigen nur ihr Effektbild.
Vollständige Namen, Regelwerte und Bauketten stehen in der Detailansicht. Fehlende oder
unpassende Grafiken erhalten vollständige Textdarstellung. GUI-Daten und
Engine müssen denselben Zielquellenhash melden.

Die Standardbibliotheksbibliothek `sevenwonders_engine.DecisionClient` ist der
geteilte Prozessclient für GUI und RL; die GUI-Sitzung enthält keine eigenen
Regel-/Wunder-ID-Heuristiken. Frühere V2-Module bleiben ausschließlich für den
historischen Tag und dessen Tests erhalten. `Game.py` startet die neue GUI.

Prüfung:

```sh
xvfb-run -a python3 -m unittest discover -s tests -p 'test_gui*.py' -v
```

Die GUI-Prüfungen umfassen fünf echte Klickpartien mit Seeds 8/19/6/9/39 für
Bilder/Fallback, Halikarnassos, die menschliche Babylon-Siebtkarte und alle
Olympia-Freibaumodi. Jede Aktion wird durch Kartenklick, Aktionsauswahl und
gesonderte Bestätigung bedient. Weitere Sessiontests
spielen mit 0/2/3 Bots und prüfen Isolation, falsche Auswahl, Abbruch und
Prozessende. Der Hotseat-Test prüft die verdeckte nächste Hand und geschlossene
private Details. Layoutprüfungen decken Mindestgröße, Resize und eine große
Ablage mit gesperrtem Duplikat ab. Alle 80 Karten- und 14 Wunderzuschnitte
werden gegen die externen eingefrorenen Bilder geprüft; die Effektausschnitte
erhalten die Pixel beider Ränder einschließlich der rechten Baukettensymbole.

Abnahme des Umbaus: 16 aktive Prüfungen bestanden, fünf historische
V2-Prozessprüfungen bleiben abgegrenzt. Details:
[gui-original-layout-validation.json](gui-original-layout-validation.json).
