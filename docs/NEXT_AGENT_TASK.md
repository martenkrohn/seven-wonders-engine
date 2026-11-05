# Nächster Agent: ausschließlich Paket 4b-II – Runden-/Zahlungsauflösung

Arbeite im Workspace `/home/marten/dev/7Wonders`. Die Engine ist das
Git-Submodule `engine/`; das RL-Projekt liegt auf
`refactor/separate-engine-20261105`. Prüfe beide Git-Stände, erhalte vorhandene
Änderungen und lies den aktuellen Engine-HEAD. Der gespeicherte RL-Pin und
`engine.lock.json` bleiben bei `f39c1d7` (`v0.1.1-legacy`). Nicht zurücksetzen
oder aktualisieren, nichts pushen oder veröffentlichen.

Paket 4b-I ist abgenommen: `85d82561a554fab3f8ca5fec05b2dfe098a9d055`.
Physische Karteninstanzen, atomarer interner Ablagebau, getrennte Handentnahme
und einmalige Quellstufen sind vorhanden. Aktiver Vertrag weiterhin
`v0.2.0-dev`, `ruleset=bga-second-edition-development`,
`second_edition_implemented=false`, Transportversion **2**. Protokoll 3 ist
noch ein Entwurf; GUI und RL verwenden den alten Vertrag.

Lies zuerst, relativ zum Engine-Verzeichnis:

1. `docs/MIGRATION_PLAN.md`, insbesondere Abnahme 4b-I und Paketgrenzen;
   `engine-contract.json`, `docs/PROTOCOL.md`, `docs/KNOWN_ISSUES.md`.
2. `docs/DECISION_PROTOCOL_DRAFT.md`, `docs/decision-protocol-examples.json`
   und die historischen/aktuellen Nachweise
   `docs/decision-protocol-validation.json`, `docs/discard-safety-validation.json`.
   Die gekürzten Nachrichtenbeispiele sind keine vollständigen spielbaren Fixtures.
3. `docs/DISCARD_SAFETY.md`, `tests/test_discard_safety.cpp`, `Card`-Identitäten,
   `Player::ApplyBuilding/BuildStructure/BuildDiscardFree/DiscardFree` und
   `Game::Loop/ResolveDiscardExtras/NextTurn/WriteGameStatus`.
4. Bestehende Ressourcen-/Endzustands-/Wissenschaftsregressionen,
   `scripts/smoke_protocol.py`, `docs/SIMULATOR_CHANGES.md`, Herkunftsaudit.
5. GUI-/RL-Transport und `ObservationHistory` bei Bedarf nur lesen, um
   inkompatible Annahmen zu erkennen; keine Integration dieser Pakete vorziehen.

**Bereits verbindlich entschieden, nicht erneut zur Bestätigung vorlegen:**
Der Nutzer hat am 5. November 2026 RQ-1 und RQ-2 mit **nein/nein** beantwortet.
Reguläre Vineyard-/Bazar-Auszahlungen zählen den gemeinsamen regulären
Bauzustand und werden durch späteren Ablagebau nicht rückwirkend erhöht.
Bei mindestens einer legalen Ablageoption ist eine Auswahl Pflicht:
`can_skip=false`, kein freiwilliges `skip`. Ohne legale Option automatische
Erledigung ohne Münzen. Dies sind ausdrückliche Projektvorgaben, kein
zusätzlich behaupteter Nachweis aus BGA-Servercode. Keine Gesamt-Recherche.

**Ziel:** Den vereinbarten Engine-Entscheidungs- und Auflösungsvertrag mit
regulärer Gleichzeitigkeit, nachgelagerter sichtbarer Ablagewahl und sicheren
Abschlussbarrieren implementieren. Die interne Entnahme aus 4b-I weiterverwenden.

Lieferumfang:

- Zuerst rote, gezielte Fälle der weiterhin sequenziellen Fehler nachweisen:
  Einnahmen/neue Produktion derselben Runde finanzieren keine reguläre Aktion,
  reguläre Sofortzähler berücksichtigen die gemeinsam errichteten Nachbarkarten,
  zusätzliche Entscheidungen verhindern vorzeitigen Ären-/Spielabschluss.
- Einen unveränderlichen gemeinsamen Rundenanfang für Hände, Münzen,
  Produktion, Rabatte, Freibaurechte, Legalität und Zahlungspläne verwenden.
  Zahlungsquellen pro Käufer unabhängig behandeln; neue Einnahmen/Produktion
  derselben Runde nicht vorziehen. Auszählung regulärer Effekte vom
  Zahlbarkeitszustand trennen; Lighthouse zählt sich selbst. Bestehende
  Kartendefinitionen, Kosten und Wunder-Aktivierungsstufen nicht migrieren.
- Reguläre Eingaben vollständig sammeln, danach gemeinsam atomar anwenden.
  Fehler zuvor ablehnen, kein stiller Ersatzabwurf. Interne Fehler kontrolliert
  behandeln, keine teilweise aufgelöste Runde als gültigen Status veröffentlichen.
- Anschließend Grants öffnen und Optionen aus der tatsächlichen Ablage
  veröffentlichen, einschließlich aktueller Verkäufe und fälliger Restkarten,
  ohne unter Wundern verbaute Karten. Jede Wahl an Partie, Sitz, Entscheidung,
  Quellenrecht und stabile physische Karteninstanz binden. Pflichtwahl,
  Leerfall, Altersduplikate und einmalige Entnahme wie vereinbart behandeln.
  Mehrfach-Grant-Reihenfolge nur als gekennzeichnete synthetische Engine-Regel
  prüfen; keine Erweiterungsunterstützung behaupten.
- Vor der Handweitergabe/Ärenauflösung alle Folgeentscheidungen abschließen.
  Neue Babylon-Züge mit frischem Anfangszustand anbieten, nur berechtigte
  Sitze abfragen. Neu aktiviertes Recht im sechsten Zug berücksichtigen.
  Kämpfe und Schlusswertung genau einmal nach allen notwendigen Entscheidungen;
  kein `finished`, Ergebnisdatei oder normales Terminalresultat davor.
- Revisions-/Entscheidungs-/Requestkennungen, angebotene Revisionen, atomaren
  Dateiaustausch, Quittierungen, Wiederholung/Identitätskonflikte, veraltete und
  falsche Eingaben, optionalen Engine-Timeout und expliziten Abbruch aus dem
  Entwurf implementieren. Ein interner oder Transportabbruch ist keine
  Niederlagenwertung. Fähigkeiten/Regelnamen/Transportversion explizit trennen.
- Den implementierten Transport wahrheitsgemäß dokumentieren und den Vertrag
  entsprechend versionieren. Alte V2-Clients dürfen kein Protokoll 3 als V2
  interpretieren; keine unbegründete Kompatibilität behaupten. Ein kleiner
  eigenständiger Headless-Client/Test darf die neuen Entscheidungen ausüben.
  Bestehende GUI-/RL-Clients werden erst in ihren eigenen Paketen migriert;
  ihre Grenzen und den nutzbaren Entwicklungs-Testweg ausdrücklich festhalten.
- Aussagekräftige C++-/Transportregressionen aus D01–D36 ableiten: unabhängig
  eingefrorene Kosten/Produktion, mehrere Schritte desselben Zugs, unterschiedliche
  Sitze, doppelte Zustellung, Fehler, Timeout/Abbruch, letzte Ablagewahl in Ära III,
  zusätzliche Babylon-Karte und konsistente Endresultate. Die Beispiele nicht
  nur mit sich selbst vergleichen. Vorhandene Tests erhalten/gezielt anpassen.
- Release-Build, alle betroffenen CTests/Transporttests, eigenständigen BGA-
  Zielaudit und Herkunftsaudit ausführen. C++-Patch/Inventar, Änderungsprotokoll,
  Implementierungsvertrag und Planabnahme mit konkretem Commit aktualisieren.
  Der optionale historische `--workspace`-Audit benötigt seine eingefrorenen
  Quelldateien; historische Prüfsummen nicht durch heutige Werte ersetzen.

**Grenzen:** Ausschließlich dieses Auflösungspaket. Keine neuen Karten-/Wunder-
werte oder Datenmigration, keine Wissenschafts-/Gildenkopierkorrektur,
GUI-Bild-/Layoutarbeit, RL-/Feature-/Export-/Trainingsänderung. Insbesondere
alte Exporter nicht still als kompatibel mit `ApplyBuilding()` ausgeben.
Keine fertige BGA-Edition behaupten, kein Release/Push, kein Parent-Pin-/Lock-
Update, keine Checkpointmigration. Ergebnis, Tests, konkreten Commit und
verbleibende Grenzen dokumentieren und lokal committen. Danach ausschließlich
Paket 5 (zuerst Kartendatentabellen) als eigenen Folgeauftrag vorbereiten und
stoppen; keine weitere Phase automatisch beginnen.
