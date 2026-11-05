# Nächster Agent: ausschließlich Paket 2 – alte Exporter reproduzierbar machen

Arbeite im Workspace `/home/marten/dev/7Wonders`. Die eigenständige Engine
ist das Git-Submodule `engine/`; das übergeordnete Repository enthält das
RL-Projekt auf `refactor/separate-engine-20261105`. Prüfe beide Git-Stände
und erhalte vorhandene Änderungen. Ein lokal weitergezogener Engine-HEAD
kann aus Paket 1 stammen; nicht zurücksetzen oder ohne Prüfung als neuen
RL-Pin übernehmen. Der Pin und `engine.lock.json` bleiben vorerst beim
Legacy-Release, bis die Übernahme ausdrücklich ansteht.

Lies zuerst:

1. `engine/docs/MIGRATION_PLAN.md`, Paket 2 und die Abnahme von Paket 1.
2. `engine/engine-contract.json`, `engine/data/bga-second-edition/README.md`.
   Die BGA-Ziele sind vorbereitet, die laufende Engine bleibt Legacy.
3. `python/scripts/extract_card_db_from_cpp.py`,
   `python/scripts/extract_wonder_db_from_cpp.py`,
   `python/scripts/audit_edition_data.py` und passende bestehende Datenbanktests.
4. `python/src/sevenwonders_rl/data/card_db.json`, `wonder_db.json` sowie
   die alten Engine-Definitionen unter `engine/main.cpp`, `include/`, `src/`.
5. Nur soweit erforderlich `docs/reference/EDITION_AUDIT.md` und den bereits
   vorhandenen Editionsvergleich. Keine neue Gesamt-Recherche.

**Ziel:** Die aktuell gepflegten alten Python-Datenbanken nachvollziehbar
aus dem Legacy-C++-Stand reproduzieren. Zuerst Diagnose und Fehlerfälle,
danach Korrektur. Der vorhandene Export ist keine ungeprüfte neue Wahrheit.

Lieferumfang:

- Vorherige Exporte in temporäre Dateien schreiben und mit den gepflegten
  Datenbanken vergleichen. Jede Abweichung nach Feld und Ursache erfassen.
- Regressionen für Wunder-ID **0**, tatsächliche Produktionsmengen
  (insbesondere Doppelproduzenten) und dieselbe Karten-ID in mehreren
  Zeitaltern. Kein Verlust von Kosten, Effekten, Deckmengen oder Stufenplänen.
- Kleinste vollständige Exporterkorrektur im RL-Projekt. Bestehende
  Datenbankschemata und Consumer bewusst berücksichtigen; Altersvarianten
  dürfen nicht still überschrieben werden. Wenn ein notwendiger Schemawechsel
  den Paketumfang vergrößert, Diagnose und Grenze konkret dokumentieren.
- Deterministischer Export: zweimal erzeugen, JSON-Diffs prüfen, Änderungen
  an versionierten Datenbanken einzeln begründen. Quellen für Kosten,
  Effekte und Wunderstufen eindeutig dokumentieren. Geerbte Enginefehler
  nicht versehentlich als Editionskorrekturen ausgeben.
- Betroffene Datenbank-/Consumer-Tests ausführen, Exporter-Dokumentation und
  Planstatus mit Ergebnis, konkretem Commit und Grenzen aktualisieren.
  Einen passenden RL-Commit erstellen; Paket 3 nur als nächsten Auftrag
  vorbereiten und anschließend stoppen.

**Grenzen:** Keine BGA-Regeln aktivieren, keine Engine-Kartendaten oder
Wunderfähigkeiten migrieren, keine Wissenschafts-/Ablagekorrektur, keine GUI-,
Feature- oder Trainingsänderung. Die BGA-Zieldaten aus Paket 1 dürfen nicht
als Ersatz für den alten Export in die laufenden Python-Datenbanken gelangen.
Den späteren gemeinsamen Datengenerator nicht in dieses Paket hineinziehen.
Kein Push/Release und keine automatische Submodule-/Lock-Übernahme.
