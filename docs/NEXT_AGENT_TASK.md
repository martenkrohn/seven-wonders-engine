# Nächster Agent: ausschließlich Paket 4a – Entscheidungsvertrag spezifizieren

Arbeite im Workspace `/home/marten/dev/7Wonders`. Die Engine ist das
Git-Submodule `engine/`; das übergeordnete RL-Projekt liegt auf
`refactor/separate-engine-20261105`. Prüfe beide Git-Stände und erhalte
vorhandene Änderungen. Der lokale Engine-HEAD enthält die bereits abgenommene
Wissenschaftskorrektur. Der gespeicherte RL-Pin und `engine.lock.json` bleiben
bewusst bei `f39c1d7` (`v0.1.1-legacy`); nicht zurücksetzen oder aktualisieren.

Lies zuerst, relativ zum Engine-Verzeichnis:

1. `docs/MIGRATION_PLAN.md`, besonders Paket 4a/4b und die Abnahme von Paket 3,
   sowie `engine-contract.json`, `docs/PROTOCOL.md`, `docs/KNOWN_ISSUES.md`.
2. `docs/GUI_AUDIT.md`, `gui/session.py`, die Rundenschleife/Statusveröffentlichung
   in `main.cpp` und die Ablage-/Freibauberechtigungen in `src/player.cpp`.
3. Nur die einschlägigen Halikarnassos-/Babylon-/Olympia-Zieldefinitionen und
   Effekte in `data/bga-second-edition/targets.json` und `reference/wonders.json`.
   Statische BGA-Definitionen enthalten nicht den vollständigen Serverregelcode.
4. Die bestehenden Endzustands-/Ressourcenregressionen unter `tests/`;
   im RL-Projekt `python/src/sevenwonders_rl/envs/sevenwonders_env.py`,
   `python/src/sevenwonders_rl/features/observation_history.py` und die aufgerufenen Transport-/Collector-
   Schnittstellen nur zur Klärung des notwendigen Vertrags.
5. Bei konkreten Regelfragen die einschlägigen Stellen aus
   `../docs/reference/EDITION_AUDIT.md` und den vorhandenen Quellen.
   Keine neue Gesamt-Recherche und keine anderen Pakete implementieren.

**Ziel:** Einen eindeutig prüfbaren Entwurf für zusätzliche Entscheidungen
und simultane Auflösung dokumentieren, den spätere Engine-, GUI- und RL-
Implementierungen gemeinsam verwenden können. Dieser Auftrag ändert keine
Laufzeitlogik. Der aktuell implementierte Transport bleibt Version 2; neue
Verträge müssen ausdrücklich als Entwurf gekennzeichnet sein.

Lieferumfang:

- Zustandsmodell mit regulärer Aktionssammlung, Auflösung, ausstehenden
  Ablageentscheidungen, Ärenwechsel und Schlusswertung. Für jeden Zustand:
  aktiver Sitz, sichtbare Informationen, Eingaben und erlaubte Folgezustände.
- Sichtbare Ablageauswahl erst nach Abschluss aller regulären Aktionen.
  Kartenauswahl über eindeutige, stabile Identität statt fremdem Handindex;
  jede Karte höchstens einmal entnehmen. Fehlende Optionen, doppelte eigene
  Karten, übersprungene Entscheidungen und Verbrauch der Berechtigung
  ausdrücklich definieren.
- Reihenfolge mehrerer gleichzeitiger Sonderentscheidungen, Sichtbarkeit
  bereits entnommener Karten und Timing weiterer Effekte vollständig festlegen.
  Entscheidungen dürfen nicht von einem global überschriebenen `extra`
  oder einer verdeckten Vorauswahl abhängen.
- Einen Rundenanfangszustand für Legalität, Produktion, Zahlbarkeit und
  reguläre Aktionen festlegen. Zwischen Zahlungen, verfügbaren Mitteln und
  Auszählung gleichzeitiger Effekte klar unterscheiden; fremde Einnahmen
  oder neue Produktionskarten derselben Runde nicht unbeabsichtigt vorziehen.
- Konkrete Nachrichtenbeispiele für Status, Optionen, Entscheidung, Quittierung,
  veraltete/ungültige Eingabe, wiederholte Nachricht, Timeout und Abbruch.
  Runden-/Entscheidungsidentität und genau einmalige Auflösung beschreiben.
  Kein `finished` oder Schlussresultat veröffentlichen, solange Zusatzentscheidungen
  ausstehen; auch siebte Karte und letzte Runde ausdrücklich behandeln.
- Regelnamen, Transportversion und verfügbare Sonderaktionen als getrennte
  Vertragsfelder spezifizieren. GUI/RL sollen Fähigkeiten aus Engine-Angaben
  lesen können, statt Wunder-IDs und Stufen selbst zu erraten.
- Akzeptanzfälle mit erwarteten Übergängen als Grundlage für Paket 4b festhalten.
  Falls eine benötigte Regel aus den vorhandenen Definitionen nicht eindeutig
  folgt, die konkrete Frage mit Quelle kennzeichnen und Implementierungsfolgen
  benennen; kein ungeprüftes BGA-Serververhalten behaupten.
- Die bestehende Protokolldokumentation auf den neuen Entwurf verweisen lassen,
  ohne Entwurf und implementierten Stand zu vermischen. Beispiele und Verweise
  prüfen, Planstatus/Ergebnis/Commit/Grenzen aktualisieren und lokal committen.
  Danach ausschließlich den ersten Teil von Paket 4b (sichere Ablageentnahme)
  als nächsten Auftrag vorbereiten und stoppen; Runden-/Zahlungsauflösung
  bleibt ein weiterer eigener Auftrag.

**Grenzen:** Keine C++-/GUI-/RL-Implementierung, keine Karten-/Wunderänderung,
keine neue Wissenschaftskorrektur, keine Export-/Feature-/Trainingsänderung.
Die getrennten Kopier-/Vorschaugrenzen aus Paket 3 nicht in diesen Auftrag
hineinziehen. Kein Push/Release, kein Submodule-/Lock-Update und keine
Checkpointmigration. Paket 4b oder ein anderes Paket nicht automatisch beginnen.
