# Nächster Agent: ausschließlich Paket 1 vorbereiten

Arbeite im Workspace `/home/marten/dev/7Wonders`. Die eigenständige Engine
ist das Git-Submodule `engine/`; das übergeordnete Repository enthält das
RL-Projekt. Prüfe beide Git-Stände und erhalte vorhandene Änderungen.

Lies zuerst:

1. `engine/docs/MIGRATION_PLAN.md`, besonders Paket 1 und die Quellenliste.
2. `engine/docs/GUI_AUDIT.md` und `engine/engine-contract.json`.
3. `docs/reference/edition-comparison/README.md`, `comparison.json` und
   `source_discrepancies.json`.
4. `references/bga-sevenwonders/README.md` sowie die dort beschriebenen
   lokalen Definitionen `card_types.json`, `wonders.json`, `manifest.json`.

Große JSON-Dateien programmatisch auswerten und nur die relevanten Zielfelder
in den Arbeitskontext laden; nicht die vollständige Gegenüberstellung ausgeben.

**Ziel dieses Auftrags:** Die bereits abgeglichenen BGA-Zieldefinitionen
für eine eigenständige Engine-Migration versioniert bereitstellen und ein
vollständiges, geprüftes Mapping festhalten. BGA ist die vom Nutzer bestimmte
Zielwahrheit. Die neue Edition ist noch nicht implementiert. Paket 0 hat nur
die alte GUI wieder eingebunden.

Lieferumfang:

- Ein nachvollziehbares Zieldatenpaket im Engine-Repository mit Quelle,
  Eingabe-Prüfsummen und 80 Karten-/Zeitalterzeilen sowie 14 Wunderseiten.
- Eindeutiges Mapping zwischen bestehenden Engine-Karten/Wundern,
  BGA-Definitionen, Zeitaltern und Sprite-Indizes. Neue Karten und
  Schreibvarianten ausdrücklich behandeln; bestehende IDs erhalten.
- Prüfung von Abdeckung, Eindeutigkeit, Kettenzielen, Deckmengen und den
  vier bereits korrigierten Kosten. Fachliche Zielwerte aus dem vorhandenen
  Vergleich übernehmen und mit den lokalen BGA-Definitionen abgleichen.
- Eine kurze begründete Entscheidung, wie künftig dieselbe Datenquelle
  C++-Definitionen und Python-Export speist, ohne fragile Laufzeitpfade.
- Planstatus und Ergebnisse aktualisieren, passende Prüfungen ausführen,
  einen abgegrenzten Commit erstellen und einen kurzen Abschlussbericht
  mit dem nächsten konkret vorbereiteten Arbeitspaket schreiben.

Dieser Auftrag ändert **keine laufenden Spielregeln, GUI-Darstellung,
RL-Features oder Trainingsstände**. Exportfehler gehören zu Paket 2.
Keine neue Gesamt-Recherche beginnen, keine Bilder/BGA-Clientquellen
veröffentlichen und kein weiteres Paket automatisch starten.

Wenn eine Referenz in einem anderen Workspace fehlt, zuerst die vorhandenen
versionierten Vergleichsdaten und das Quellenmanifest verwenden; genau die
fehlende Eingabe benennen. Nicht aus alten CSV-Werten eine vermeintlich
vollständige Zieldefinition erfinden.
