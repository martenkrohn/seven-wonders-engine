# Alte grafische Oberfläche verwenden

Die ursprüngliche Tk-Oberfläche ist als optionales Frontend wieder eingebunden.
Sie verwendet weiterhin den bisherigen Regelstand und benötigt kein RL-Paket.
`gui/legacy_view.py` stammt aus dem ursprünglichen `Game.py`; Start, Dateiaustausch
und Prozessverwaltung liegen jetzt getrennt in `Game.py` und `gui/session.py`.

## Starten

Im Engine-Repository:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
python3 Game.py 2 --assets-dir /home/marten/dev/7Wonders/images --seed 42
```

Python 3.10+, Tkinter und ein erreichbares Grafikdisplay sind erforderlich.
Unter Ubuntu/WSL stellt das Systempaket `python3-tk` Tkinter bereit. Mit
`python3 Game.py --help` lassen sich die Optionen auch ohne Display ansehen.

Der Bildpfad bezeichnet den vorhandenen Ordner mit `cards/`, `wonders/` und
`resources/`. Die Bilder bleiben lokal. Ohne `--assets-dir` wird `engine/images/`
verwendet; fehlende Bilder werden durch beschriftete Platzhalter ersetzt.
Bei einem Build aus dem RL-Projekt kann dessen Binary verwendet werden:

```sh
python3 engine/Game.py 2 --engine build/7Wonders --assets-dir images --seed 42
```

## Bedienung

- `0`: drei menschliche Plätze am selben Bildschirm.
- `1` oder `2`: entsprechend viele einfache Bots auf den ersten Plätzen.
- `3`: den einfachen Bots zuschauen.
- Pro menschlichem Platz eine Handkarte anklicken und die gewünschte Aktion
  wählen. Anschließend **Play selected actions** oder Enter drücken.
- Bots bauen die erste laut Engine baubare Karte und werfen sonst ab. Das sind
  einfache Begleiter, keine trainierten RL-Agenten.
- Übersprungene siebte Karten werden automatisch quittiert. Babylon B erhält
  seine reguläre Auswahl, wenn die alte zweite Stufe gebaut ist.
- Am Ende bleibt die vollständig ausgewertete Punktetafel sichtbar. Das
  Schließen des Fensters beendet auch einen noch laufenden Engine-Prozess.

Alle Hände sind sichtbar. Die Oberfläche dient lokalen Test-/Hotseat-Partien;
sie verbirgt keine Gegnerinformationen. Der Ablagebau von Halikarnassos bleibt
wegen der bestehenden Engine-/Entscheidungsprobleme unvollständig. Alte Bilder
können bei den bereits dokumentierten vier Kartenabweichungen andere Kosten
zeigen als die Engine. Maßgeblich für verfügbare Aktionen ist deren Status.
Nachbarzahlungen werden von der Engine bestimmt; eine eigene Auswahl
zwischen mehreren Zahlungsvarianten ist in dieser Oberfläche nicht vorhanden.

## Prüfen

Ohne Display und ohne zusätzliche Python-Pakete:

```sh
python3 -m unittest discover -s tests -p 'test_gui*.py' -v
ctest --test-dir build --output-on-failure
python3 scripts/audit_simulator.py --check
```

Mit Display, einschließlich tatsächlicher Tk-Klicks und Schlussanzeige:

```sh
python3 scripts/smoke_gui.py --assets-dir /home/marten/dev/7Wonders/images
python3 scripts/smoke_gui.py --assets-dir /home/marten/dev/7Wonders/images --bots 0 --mixed-actions
python3 scripts/smoke_gui.py --bots 3
```

Grenzen und Prüfergebnisse: [GUI-Audit](GUI_AUDIT.md). Weitere Arbeit wird nach
dem [Migrationsplan](MIGRATION_PLAN.md) jeweils in einem abgegrenzten Schritt
fortgesetzt; die Wiedereinbindung stellt keine Editionsmigration dar.
