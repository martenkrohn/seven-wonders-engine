# Nächster Agent: ausschließlich Paket 5-I – Kartendatentabellen

Arbeite im Workspace `/home/marten/dev/7Wonders`. Engine ist das Git-Submodule
`engine/`; das RL-Projekt liegt auf `refactor/separate-engine-20261105`.
Prüfe beide Git-Stände und erhalte vorhandene Änderungen. Der gespeicherte
RL-Pin und `engine.lock.json` bleiben bei `f39c1d7` (`v0.1.1-legacy`).
Kein Push/Release und keine Aktualisierung des Parent-Pins oder Locks.

Paket 4b-II ist abgenommen: `6ad395332da5535f3cdc7f7699fea6fd12c07e9f`.
Aktiver Transport **3**, Vertrag `decision-v1`, Release `v0.2.0-dev`,
Regeln `bga-second-edition-development`, `second_edition_implemented=false`.
`DecisionEngine` löst reguläre Aktionen gemeinsam auf und bietet nachgelagerte
Pflichtentscheidungen. GUI und RL sind noch nicht migriert; nutzbarer Testweg
ist `python3 scripts/smoke_protocol.py --engine build/7Wonders --seed 8`.
Karten-/Wunderdefinitionen sind bislang noch Legacy.

Lies zuerst, relativ zum Engine-Verzeichnis:

1. `docs/MIGRATION_PLAN.md`, besonders die Abnahme 4b-II und die Trennung
   zwischen 5-I (Tabellen) und 5-II (einzelne neue Sondereffekte).
2. `data/bga-second-edition/README.md`, `MAPPING.md`, `targets.json`,
   `provenance.json` und den eigenständigen Zielaudit. Das Paket enthält
   80 Zeitalterzeilen, 77 Kartentypen, 25 Ketten und die geprüften ID-Zuordnungen.
   Referenz-JSON/Prüfsummen bleiben eingefroren; keine Gesamt-Recherche.
3. `docs/PROTOCOL.md`, `docs/SIMULTANEOUS_RESOLUTION.md`,
   `docs/resolution-validation.json` und `engine-contract.json`.
4. Karten-Deklarationen/Deckaufbau in `main.cpp`, `Card`,
   `Player::ApplyBuilding/ImmediateCoins/CalculateCivilianScore` sowie
   `DecisionCore::Sources/Quote/OfferRound`. Der neue Planer und die
   Gebäudeanwendung müssen dieselben Produktions-/Effektwerte verwenden.
5. Herkunftsaudit/Änderungsprotokoll und bestehende Daten-, Produktions-,
   Wissenschafts-, Auflösungs- und Transportregressionen.

**Ziel:** Die geprüften Kartendaten als deterministisch erzeugte Engine-
Tabellen übernehmen. Keine händisch divergierenden C++-/Python-Wahrheiten.
Die frühere Pipelineentscheidung aus Paket 1 verwenden: geprüfter Zielvertrag
als Quelle, reproduzierbare C++-Tabellen und ein klar versioniertes Exportartefakt
für spätere Consumer. RL-Paketdaten und alte Exportskripte hier nicht verändern.

Lieferumfang:

- Zunächst gezielte rote Abweichungen der alten Tabellen gegen die Ziele
  nachweisen: geänderte Kosten/Punkte/Ketten, graue Altersvarianten und Deckmengen.
  Die bereits bestätigten BGA-Kosten Town Hall, Arena, Circus und Craftsmens
  Guild nicht auf alte CSV-Werte zurücksetzen.
- Eine reproduzierbare Erzeugung samt schreibfreier Prüfung entwickeln.
  Gleichartige Datenfelder nicht separat in C++-Switches und Exportdateien
  pflegen. Herkunft und Formatversion des erzeugten Artefakts dokumentieren;
  ein isolierter Engine-Klon benötigt keine RL-Quelldateien oder Bildordner.
  Generierte Runtime-Daten dürfen nicht vom aktuellen Arbeitsverzeichnis auf
  einen zufälligen Quellpfad zugreifen.
- Alle 80 Karten-/Zeitalterzeilen mit Namen, Typ, Alter, Kosten, Punkten,
  Ketten, Drei-Spieler-Mengen und festen Produktions-/Schild-/Münzeffekten
  gegen die Ziele abbilden. Graue Varianten getrennt behandeln; keine
  unbemerkte Namenssuche über ein falsches Zeitalter wieder einführen.
- IDs erhalten: Engine-IDs sind keine BGA-/Sprite-IDs. Strategists 73 bleibt
  reserviert und aus dem neuen Grundspiel ausgeschlossen. Ludus 76, Castrum 77,
  Decorators Guild 78 gemäß Mapping verwenden; andere IDs nicht verschieben.
  Physische Instanzkennungen und Entscheidungsauswahl aus 4b-II erhalten.
- Die Produktions-/Zahlungsplanung und tatsächliche Gebäudeanwendung an die
  erzeugten festen Effekte anbinden, damit Angebot und Ausführung nicht
  unterschiedliche Mengen kennen. Wissenschaftsoptimierung und bestehende
  Auflösungs-/Ablage-/Endbarrieren erhalten. Keine Wunderdefinition umstellen.
- Ludus und Decorators Guild nicht als reine Kostenänderung ausgeben. Ihre
  neuen besonderen Zähl-/Wertungseffekte sind eigene Folgeaufträge in 5-II.
  Zielmetadaten erhalten, fehlende Laufzeitwirkung ausdrücklich dokumentieren
  und gezielt abgrenzen; keinen ungeprüften vollständigen Kartenregelstand
  oder eine fertige Edition behaupten.
- Zielvergleich und Generatordeterminismus einschließlich isoliertem Checkout
  und unverwandtem Arbeitsverzeichnis prüfen. Drei-Spieler-Decks pro Ära
  vollständig gegen Ziel, Kosten-/Ketten-/Punkte-/Produktionsfälle über echte
  Angebote und Anwendung prüfen. Alle betroffenen CTests und Protokoll-3-
  Partien, Datenprüfungen und Herkunftsaudit ausführen. Die historischen
  V2-GUI-Prozesstests bleiben für den alten Tag abgegrenzt; keine GUI-Migration.
- C++-Inventar/Patch, Änderungsprotokoll, aktuellen Entwicklungsvertrag und
  Planabnahme mit konkretem Commit aktualisieren und lokal committen.
  Der optionale historische Workspace-Abgleich benötigt seine eingefrorenen
  Quelldateien; historische Prüfsummen nicht durch neue Werte ersetzen.

**Feststehende Regeln:** RQ-1/RQ-2 sind durch den Nutzer entschieden:
keine rückwirkenden regulären Münzen nach späterem Ablagebau, kein freiwilliger
Verzicht bei legaler Ablageoption. Nicht erneut zur Bestätigung vorlegen.

**Grenzen:** Ausschließlich Kartendatentabellen und deren feste Felder/
Integration. Keine neuen Ludus-/Decorators-Sonderalgorithmen, Wunderfähigkeiten,
GUI-/Bild-/Layout-, RL-/Feature-/Exportskript-/Trainingsänderungen. Keine
Wissenschafts-/Gildenkopie-Kandidatenkorrektur, Checkpointmigration oder neue
Spielerzahlen. Kein Release/Push/Parent-Pin-/Lock-Update. Entwicklungsname und
`second_edition_implemented=false` beibehalten. Nach Abschluss ausschließlich
Paket 5-II vorbereiten, zuerst einen einzelnen neuen Kartensondereffekt, und
stoppen; keine weitere Phase automatisch beginnen.
