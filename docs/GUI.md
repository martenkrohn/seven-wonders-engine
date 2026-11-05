# BGA-GUI mit angebotenen Entscheidungen

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
physische Karteninstanz, Entscheidung und Revision. Normale Aktionen, siebte
Karte und Pflicht-Ablagewahl verwenden dieselbe Auswahlansicht. Freibaumodi
und Bauketten haben eigene beschriftete Schaltflächen; Zahlungen sind sichtbar.
Bei mehreren Menschen wird die nächste Hand erst nach expliziter Übergabe
angezeigt. Öffentliche Städte, Ressourcen, alle Punktekategorien und die
abschließende Wertung bleiben sichtbar. Die Ereignisschleife wartet nicht
blockierend auf den Spielprozess; Schließen räumt den Prozess und seine
isolierten Austauschdateien auf.

Grafiken bleiben extern. Im gemeinsamen Workspace wird die vorhandene BGA-
Referenz automatisch gefunden; andernfalls beispielsweise:

```sh
python3 Game.py 2 --assets-dir /pfad/bga-sevenwonders
```

Benötigt werden `img/cards_v2.jpg` und `img/boards_v2.jpg` (tatsächlich WebP).
Die Checksummen des eingefrorenen Manifests verhindern, dass alte Bilder als
neue Kartenwerte ausgegeben werden. Die Zuschnitte verwenden die expliziten
Sprite-Rechtecke einschließlich Wunder-ID 0 und grauer Altersvarianten.
Kartennamen und Regelwerte werden immer zusätzlich dargestellt. Fehlende oder
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

Paket 7: zwölf aktive Prüfungen bestanden, fünf historische V2-Prozesstests
abgegrenzt. Vier echte Klickpartien mit Seeds 8/19/3/9 decken Bilder/Fallback,
Halikarnassos und alle neuen Olympia-Freibaumodi ab. Weitere Sessiontests
spielen mit 0/2/3 Bots und prüfen Isolation, falsche Auswahl, Abbruch und
Prozessende. Der Hotseat-Test prüft die verdeckte nächste Hand. Alle 80 Karten-
und 14 Wunderzuschnitte sind gegen die externen eingefrorenen Bilder geprüft.
