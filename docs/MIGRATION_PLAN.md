# Arbeitsplan: alte GUI bis zur spielbaren BGA-Edition

Stand: 5. November 2026. Dieser Plan ist die maßgebliche Reihenfolge für die
weitere Migration. **Ein Auftrag bearbeitet jeweils nur ein Arbeitspaket.**
Zum Abschluss werden Ergebnis, Tests, Commit und verbleibende Grenzen
eingetragen. Anschließend endet der Auftrag; das nächste Paket startet mit
frischem Kontext. Der [fertige nächste Agentenauftrag](NEXT_AGENT_TASK.md)
begrenzt die nächste Sitzung auf Paket 1.

## Ziel und feste Entscheidungen

- Ziel ist das Grundspiel der neuen Edition nach den bereits gesammelten
  **BGA-Definitionen**. Erweiterungen, Promos und weitere Spielerzahlen sind
  eigene spätere Vorhaben. Die Engine unterstützt zunächst drei Spieler.
- Engine, Spielregeln, GUI und Engine-Tests gehören in dieses Repository.
  RL, Feature-Engineering, Python-Datenexporte und Training bleiben separat.
- Die alte Implementierung bleibt über Tags reproduzierbar. Ein paralleler
  umschaltbarer Betrieb beider Editionen ist derzeit nicht erforderlich.
- Das aktuelle Paket 0 stellt ausschließlich die alte GUI wieder her.
  Neue Kosten, Fähigkeiten und Grafik werden getrennt umgesetzt.
- Jeder Commit muss klar benennen, welchen Regelstand er abbildet. Eine
  begonnene Migration erhält einen Entwicklungs-Regelnamen und
  `second_edition_implemented=false`; erst die Abnahme erlaubt eine fertige
  BGA-Version. Protokollversion und Regeledition sind verschiedene Verträge.

## Arbeitsverzeichnisse und Quellen

Im gemeinsamen Workspace liegt die Engine unter
`/home/marten/dev/7Wonders/engine`, das RL-Projekt eine Ebene darüber.
`engine/` ist ein Git-Submodule mit eigener Historie. Vor jeder Sitzung beide
Git-Arbeitsstände prüfen. Die RL-Arbeit liegt auf
`refactor/separate-engine-20261105`; sie wurde bisher lokal committed.

| Quelle relativ zum Engine-Verzeichnis | Verwendung |
| --- | --- |
| `docs/GUI.md`, `docs/GUI_AUDIT.md` | Gegenwärtiger GUI-Stand und konkrete offene Punkte |
| `docs/KNOWN_ISSUES.md`, `docs/PROTOCOL.md` | Bestehende Regelgrenzen und Transportvertrag |
| `../docs/reference/edition-comparison/comparison.json` | Bereits abgeglichene 80 Karten-/Zeitalterzeilen und 14 Wunderseiten; Felder `new`, `bga_id`, `new_sprite` |
| `../docs/reference/edition-comparison/COMPARISON.md` | Lesbare vollständige Gegenüberstellung |
| `../references/bga-sevenwonders/README.md` | Inventar und Erklärung der lokalen BGA-Referenz |
| `../references/bga-sevenwonders/card_types.json`, `wonders.json`, `manifest.json` | Eingefrorene BGA-Definitionen und Bild-Prüfsummen |
| `../docs/reference/EDITION_AUDIT.md` | Vertiefte technische Fehlerfälle; ältere offene Datenfragen durch Vergleich vom 5. November überholt |
| `../python/scripts/audit_edition_data.py`, `extract_*_db_from_cpp.py` | Bestehende Exportwege und bekannte Abweichungen |

Der bisherige Abgleich ist abgeschlossen: **17 Kartenzeilen und 11 Wunderseiten**
ändern sich gegenüber der alten gedruckten Ausgabe; vier weitere Kartenzeilen
zeigen geerbte Engine-Abweichungen. Diese Zahlen sind verschiedene Vergleiche.
Die neuen Ziele umfassen 77 Kartentypen, verteilt auf 80 Zeitalterzeilen.
Die lokale Sammlung ist keine Kopie des BGA-Server-Regelcodes. Falls ein
Spezialfall aus den Definitionen nicht eindeutig folgt, wird genau dieser
Fall als offene Regelfrage mit Quelle dokumentiert.

Rohreferenzen und Bilder sind lokal vorhanden, aber absichtlich nicht im
Engine-Repository versioniert. Ein separater Engine-Klon enthält sie deshalb
nicht. Die geprüften Zieldaten müssen in Paket 1 eigenständig nutzbar gemacht
werden. Bilddateien bleiben über einen externen lokalen Ordner verwendbar.

## Reihenfolge und Abschlusskriterien

| Paket | Status | Begrenzter Auftrag | Fertig, wenn … |
| --- | --- | --- | --- |
| 0 – Legacy-GUI | Erledigt, technische Abnahme siehe GUI-Audit | Alte Oberfläche wieder anbinden und kompatibel machen | Alte Partien mit Endwertung, Session-Isolation und Tests funktionieren; Regeln unverändert |
| **1 – Zielvertrag und Zuordnung** | **Als Nächstes** | Vorhandene BGA-Ziele versioniert festhalten und IDs/Schema zuordnen | 80 Kartenzeilen und 14 Seiten vollständig; Prüfsummen/Quelle dokumentiert; alle Zielkarten und Ketten eindeutig zugeordnet; kein Laufzeitumbau |
| 2 – Exporter reproduzierbar machen | Offen | Im RL-Projekt die alten Exportfehler beheben | Wunder-ID 0, Produktionsmengen und Mehrfach-Zeitalter korrekt; deterministischer Export mit geprüftem JSON-Diff und Regressionen |
| 3 – Wissenschaftswertung | Offen | Gierige Jokervergabe durch optimale Endwertung ersetzen | Dokumentierte Gegenbeispiele und Kombinationen mit mehreren Jokern/Quellen bestehen; keine Kartendaten-/GUI-Änderung |
| 4a – Entscheidungsvertrag | Offen | Ablagewahl und simultane Auflösung als Protokollzustände spezifizieren | Eingaben, sichtbare Optionen, Sitzreihenfolge, Timing, Abbruch und Terminalzustand sind für GUI und RL eindeutig beschrieben; Implementierung folgt separat |
| 4b – Sichere Auflösung | Offen | Vertrag aus 4a in der Engine implementieren | Ablagekarten genau einmal entnommen, keine Handentnahme mit Ablageindex, Berechtigung korrekt verbraucht, relevante Gleichzeitigkeit getestet |
| 5 – Neue Karten | Offen | Kosten, Punkte, Ketten, Deckmengen und neue Karteneffekte migrieren | Alle 80 Zeitalterzeilen gegen Ziel geprüft; Ludus und Decorators Guild funktionieren; Drei-Spieler-Decks stimmen |
| 6 – Neue Wunder | Offen | Die 14 Seiten familienweise umstellen | Jede Stufe: Kosten, Ertrag, Verfügbarkeit und richtige Anzahl geprüft; Sonderfähigkeiten mit gezielten Fällen abgenommen |
| 7 – GUI der neuen Edition | Offen | Neue Entscheidungsansichten, Bilder und Layout einbinden | Alle neuen Fähigkeiten menschlich bedienbar; korrekte Bilder/Namen/Stufen; vollständige Partien und fehlende Bilder getestet |
| 8 – RL-Anbindung | Offen | Neue Engine bewusst im RL-Projekt übernehmen | Exportdaten, Beobachtungen, Aktionen, Merkmale und Checkpoint-Verträge passen; Protokoll-/Kurztrainingsprüfungen bestehen |
| 9 – Gemeinsame Abnahme | Offen | Befunde schließen und neue Version veröffentlichen | Referenzvergleich, Regelfälle, GUI-Partien, automatisierte Spiele und CI grün; Release, Regelnamen und RL-Pin stimmen |

Paket 6 wird selbst in kleine Aufträge geteilt: zuerst einfache Kosten-/Punkte-
Wunder, danach Babylon, Olympia und Halikarnassos einzeln. Ein einzelner Agent
bekommt jeweils nur eine dieser Teilaufgaben. Die sicheren Entscheidungen aus
4b sind Voraussetzung für den vollständigen Halikarnassos-Pfad.
Auch 4b wird getrennt (Ablageentnahme, danach Runden-/Zahlungsauflösung),
Paket 5 in Datentabellen und einzelne neue Effekte, Paket 7 in
Entscheidungsansichten und anschließende Bild-/Layoutintegration.

## Was die einzelnen Pakete berücksichtigen müssen

**1 – Quellen und Schema:** Den vorhandenen Abgleich wiederverwenden, nicht
die gesamte Recherche wiederholen. Engine-IDs nicht als BGA-IDs oder
Sprite-Indizes interpretieren. Alte Engine-Karten-IDs möglichst erhalten,
neue Karten anhängen. Graue Varianten nach Karte **und Zeitalter** trennen.
`Pawnshop`/`Well`, `Apothocary`/`Apothecary`, `Craftsmen Guild`/`Craftsmens Guild`
explizit zuordnen. Wunder: BGA 1–14, Engine 0–13. Die vier bereits korrigierten
BGA-Kosten (Town Hall, Arena, Circus, Craftsmens Guild) nicht auf frühere
CSV-Werte zurücksetzen. Festhalten, wie dieselbe Definition künftig C++ und
Python speist; isolierte Runtime-Verzeichnisse dürfen keine Quelldatenpfade
aus dem aktuellen Arbeitsverzeichnis voraussetzen.

**2 – Export:** Erst Diagnose/Regression, dann Exporter reparieren. Die aktuell
gepflegten Datenbanken sind nicht vollständig mit den alten Skripten
reproduzierbar. Einen ungeprüften Export nicht als neue Wahrheit übernehmen.
Quellen von Kosten, Effekten und Stufenplänen eindeutig machen.

**3/4 – Fachliche Grundlagen:** Wissenschaftsjoker optimal über sämtliche
Symbole verteilen. Halikarnassos braucht eine tatsächlich sichtbare Auswahl
nach den regulären Aktionen. Kein global überschriebenes `extra`, keine
doppelte Entnahme, kein Zugriff auf eine fremde Handposition. Zahlungen,
Produktionsmöglichkeiten und reguläre Aktionen auf den passenden
Rundenanfangszustand beziehen; fremde Einnahmen/neue Karten derselben Runde
dürfen nicht unbeabsichtigt früh nutzbar werden. Ärenende und Schlusswertung
müssen zusätzliche Entscheidungen abwarten.
Der Status soll Regelnamen und verfügbare Sonderaktionen ausdrücklich tragen;
Transportversion 2 allein belegt keine kompatible Edition eines anderen Binarys.

**5/6 – Daten und Fähigkeiten:** Neue Karten nicht auf reine Kostenänderungen
reduzieren: Ludus, Decorators Guild und geänderte blaue Punkte berücksichtigen.
Olympia erhält andere Freibauregeln; Babylon B zwei Stufen und frühere siebte
Karte. Die Engine soll verfügbare Sonderentscheidungen explizit mitteilen.
Clients sollen sie nicht mehr aus hart codierten Wunder-IDs erraten.

**7 – Bildintegration:** BGA-Karten sind Hochformat, die alte GUI nutzt
Querformat. Neue Tableaus haben andere Höhe und Stufenanordnung. Nach
Sprite-Index und geprüftem Mapping schneiden; BGA-Karten brauchen ein
Namensoverlay. Tatsächlichen MIME-Typ verwenden (teils WebP). Ersatzbilder
behalten; alte Bilder mit abweichenden Werten nicht als aktuelle Edition
ausgeben. Die GUI kann bis dahin über den Legacy-Tag als Vergleich dienen.

**8/9 – Übernahme:** Neue Regeln bedeuten neue Daten-/Featuresemantik. Alte
Checkpoints oder Lernkurven sind nicht ohne Prüfung übertragbar. Alte Tags
bleiben erhalten. Den Engine-Commit zuerst veröffentlichen, danach
Submodule-Pin und `engine.lock.json` im RL-Projekt aktualisieren.

## Vorgehen je Auftrag

1. Nur das freigegebene Paket lesen, vorhandene Befunde und saubere Git-Stände
   prüfen; bei lokalen Änderungen deren Herkunft klären und sie erhalten.
2. Den kleinsten fachlich vollständigen Schritt implementieren. Bei Regeln
   einen aussagekräftigen Fehlerfall vor der Korrektur nachweisen.
3. Betroffene Tests ausführen. Engine-Build, Herkunftsaudit und CTests bei
   C++-Änderungen; echte Tk-Prüfung bei GUI-Verhalten; RL-Vertragstests nur
   bei betroffener Integration. Weitere Tests durch Befunde begründen.
4. Engine-Änderungsprotokoll, Patch/Inventar bei geändertem C++-Stand und
   gegebenenfalls Regelnamen/Protokollvertrag aktualisieren.
5. Planstatus, konkreten Commit, Tests und offene Punkte eintragen und
   den Auftrag abschließen. Keine anschließende Phase automatisch beginnen.

Als technischer Startpunkt ist die GUI-Abnahme in
[gui-validation.json](gui-validation.json) festgehalten. Die bisherigen
Regelgrenzen sind dadurch nicht als gelöst markiert.
