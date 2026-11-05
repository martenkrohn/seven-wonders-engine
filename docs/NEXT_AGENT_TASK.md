# Nächster Agent: ausschließlich Paket 3 – optimale Wissenschaftswertung

Arbeite im Workspace `/home/marten/dev/7Wonders`. Die eigenständige Engine
ist das Git-Submodule `engine/`; das übergeordnete Repository enthält das
RL-Projekt auf `refactor/separate-engine-20261105`. Prüfe beide Git-Stände
und erhalte vorhandene Änderungen. Der lokale Engine-HEAD enthält die
Paket-1-Ziele und die Paket-2-Abnahme; der RL-Pin und `engine.lock.json`
bleiben bewusst auf `f39c1d7` (`v0.1.1-legacy`). Nicht zurücksetzen oder
ungeprüft als neuen Pin übernehmen.

Lies zuerst:

1. `docs/MIGRATION_PLAN.md` im Engine-Repository, besonders Paket 3 und die
   bisherigen Abnahmen, sowie `engine-contract.json` und `docs/KNOWN_ISSUES.md`.
2. `src/player.cpp`, insbesondere `CalculateScientificScore()`,
   `CalculateScores()` und die vorhandenen Joker-/Gildenkopierquellen;
   dazu die relevanten Schnittstellen in `include/player.h` und `src/wonder.cpp`.
3. Die vorhandenen Engine-C++-Tests, `CMakeLists.txt`,
   `docs/SIMULATOR_CHANGES.md` und `docs/DEVELOPMENT.md`.
4. Im RL-Projekt nur die einschlägige Evidenz in
   `docs/reference/EDITION_AUDIT.md` (P0 Wissenschaftsjoker),
   `docs/guides/LEGACY_DATA_EXPORT.md` und die vorhandene Wissenschaftsdiagnose.
   Keine neue Gesamt-Recherche und kein BGA-Datenumbau.

**Ziel:** Die Wissenschaftsendwertung maximiert
`gear² + compass² + tablet² + 7 * min(gear, compass, tablet)` über sämtliche
Verteilungen aller verfügbaren Joker. Jeder Joker zählt genau einmal.
Feste Symbole und Quellen werden nicht durch eine gierige Zwischenentscheidung
verändert. Die Korrektur gehört in die Engine.

Lieferumfang:

- Zuerst einen aussagekräftigen C++-Fehlerfall nachweisen: Workshop,
  Laboratory, Apothecary und Scientists Guild ergeben zwei Zahnräder,
  einen Zirkel und einen Joker; optimal sind 13 Punkte, die alte Engine
  erreicht nur 10. Reproduzierbarer roter Test vor der Korrektur.
- Kleinste vollständige optimale Wertung implementieren. Joker aus
  Scientists Guild und Babylon sowie relevante Kombinationen/Kopierquellen
  berücksichtigen; keine davon doppelt zählen. Fälle ohne Joker, mit
  fehlenden Symbolen, mehreren Jokern und gleichwertigen Optima prüfen.
  Wiederholte Status-/Endwertungsberechnung darf keinen Zustand verändern.
- Schlusswertung, veröffentlichte Wissenschaftspunkte und Gesamtpunkte
  für identische Symbolbestände konsistent prüfen. Vorhandene Python-
  Diagnose/Punktevorschau nur auf Widersprüche prüfen; keinen RL-Feature-
  oder Trainingsumbau in dieses Paket hineinziehen. Falls ein weiterer
  fachlicher Konflikt auftaucht, ihn konkret abgrenzen und dokumentieren.
- Dem begonnenen Laufzeitumbau einen Entwicklungs-Regelnamen geben und
  `second_edition_implemented=false` beibehalten. Release-/Regelvertrag
  darf die korrigierte Entwicklungsversion nicht als unveränderten alten
  Legacy-Release oder fertige BGA-Edition ausgeben. Transportversion und
  Regelnamen sind getrennte Verträge.
- Release-Build, alle betroffenen/neuen CTests und Herkunftsaudit ausführen;
  `docs/SIMULATOR_CHANGES.md`, `docs/simulator/upstream.patch` und
  `docs/simulator/manifest.json` gemeinsam aktualisieren. Ergebnis, Tests,
  konkreten Commit und Grenzen im Plan eintragen und den Engine-Stand
  lokal committen. Paket 4a als nächsten Auftrag vorbereiten, dann stoppen.

**Grenzen:** Keine Kartenkosten/-punkte/-mengen, Wunderstufen/-fähigkeiten,
BGA-Zieldaten, Ablage-/Simultanentscheidungen, GUI oder RL-Datenbanken ändern.
Die in Paket 2 reparierten Exporter gehören zum gepinnten Legacy-Stand;
sie sind kein Auftrag, die neue Entwicklungsengine schon ins RL-Projekt
zu übernehmen. Kein Push/Release, kein Submodule-/Lock-Update, keine
Checkpointmigration oder Trainingsläufe. Kein weiteres Paket automatisch beginnen.
