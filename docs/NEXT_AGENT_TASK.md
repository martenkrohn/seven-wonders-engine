# Nächster Agent: ausschließlich Paket 5-IIa – Ludus

Arbeite im Workspace `/home/marten/dev/7Wonders`. Engine ist das Git-Submodule
`engine/`; das RL-Projekt liegt auf `refactor/separate-engine-20261105`.
Prüfe beide Git-Stände und erhalte vorhandene Änderungen. Der gespeicherte
RL-Pin und `engine.lock.json` bleiben bei `f39c1d7` (`v0.1.1-legacy`).
Kein Push/Release und keine Aktualisierung des Parent-Pins oder Locks.

Paket 5-I ist implementiert: `8b35b280635c248f8c4cdb985d20ad7d86a8c1a1`.
Die nachfolgende Abnahmedokumentation enthält den endgültigen Prüfstand.
Aktiver Transport **3**, Vertrag `decision-v1`, Release `v0.2.0-dev`,
Regeln `bga-second-edition-development`, `second_edition_implemented=false`.
80 Karten-/Zeitalterzeilen sind generiert und integriert. Der Status nennt
`tables_complete=true`, `effects_complete=false`, fehlende Effekte `[76,78]`.
Ludus und Decorators sind bewusst noch gesperrt; Wunder verwenden Legacy.
GUI/RL sind nicht migriert. Nutzbarer Testweg:

```sh
python3 scripts/smoke_protocol.py --engine build/7Wonders --seed 8
```

Lies zuerst:

1. `docs/MIGRATION_PLAN.md`, Abnahme 5-I und Grenzen 5-II.
2. `docs/CARD_DATA.md`, `docs/card-data-validation.json`, `engine-contract.json`.
3. Ludus-Zeile in `data/bga-second-edition/targets.json`, Mapping/README sowie
   `scripts/generate_card_data.py`, `include/card_data.h`, `Card::GetEffects`.
4. `Player::ImmediateCoins/CountCardEffect/CalculateCommercialScore`,
   `DecisionCore::ResolveRound` und Folge-Ablagebau; `docs/PROTOCOL.md` und
   `docs/SIMULTANEOUS_RESOLUTION.md` für den feststehenden Auszahlungszeitpunkt.
5. `tests/test_card_tables.cpp`, `test_bga_card_data.py`, die bestehenden
   Auflösungs-/Ablage-/Wissenschaftsregressionen und Herkunftsaudit.

**Ziel:** Ausschließlich den neuen Ludus-Effekt (Engine-ID 76) implementieren
und abnehmen. Ziel: einmalig drei Münzen je eigener roter Karte bei Errichtung,
am Spielende ein Punkt je eigener roter Karte. Fremde rote Karten zählen
nicht. Die regulären Münzen verwenden den gemeinsamen Bauzustand; spätere
Ablagebauten zahlen keine regulären Münzen rückwirkend aus. Später gebaute
rote Karten zählen aber bei der Endwertung. Keine erneute Regelfrage nötig.

Lieferumfang:

- Zuerst rote gezielte Laufzeitfälle gegen 5-I nachweisen. Bereits vorhandene
  Metadaten allein belegen keine implementierte Wirkung; `runtime_complete`
  sperrt aktuell Ludus vollständig. Tests des 5-I-Offenstatus gezielt ablösen,
  die Decorators-Abgrenzung erhalten.
- Null/eine/mehrere eigene rote Karten, fremde rote Karten, spätere rote Karte,
  Wiederholungswertung und genau einmalige Auszahlung prüfen. Regulären
  Angebots-/Auflösungspfad und freien Ablagebau kontrolliert abdecken.
  Physische Instanzkennungen, Zahlungen und die Quellentnahme erhalten.
- Faktoren und Zählziel weiter aus derselben erzeugten Definition beziehen;
  keine zweite unabhängige Kost-/Effekttabelle oder neue Regex-Extraktion.
  `DEFERRED`/generierte Artefakte, Status, Exportgrenze und Vertrag konsistent
  aktualisieren: Ludus vollständig, **Decorators 78 weiterhin offen**.
- Ludus hat bei drei Spielern **null normale Kopien**. Aussagekräftige
  kontrollierte Engine-Fixtures verwenden; nicht die Deckmengen erhöhen, um
  den Effekt in regulären Drei-Spieler-Smoke-Partien auftauchen zu lassen.
- `--check`, Zielaudit, neue/alte CTests, echte Protokoll-3-Partien und
  Herkunftsaudit ausführen. C++-Inventar/Patch und Änderungsprotokoll erneuern.
  Isolierte Runtime und vollständige Ergebnisdateien erhalten.
- Ergebnis, Tests, Commit und Grenzen im Plan festhalten, lokal committen.
  Danach ausschließlich den separaten Decorators-Auftrag (5-IIb) vorbereiten
  und stoppen; dessen Algorithmus hier nicht implementieren.

**Feststehend:** RQ-1/RQ-2 sind durch den Nutzer entschieden: keine
rückwirkenden regulären Münzen nach Ablagebau; keine freiwillige Ablehnung
bei legaler Ablageoption. Nicht erneut zur Bestätigung vorlegen.

**Grenzen:** Nur Ludus. Keine Decorators-Bedingung/-Wertung, keine Wunder,
Gildenkopie-Kandidaten- oder Wissenschaftsoptimierungskorrektur, GUI-/Bild-/
Layout-, RL-/Feature-/Legacy-Exporter-/Trainingsänderung. Keine neue
Spielerzahl. Frozen Targets/Referenz/Provenance nicht ändern. Der optionale
historische Workspace-Audit benötigt die ursprünglichen gehashten
`engine/*`-Dateien aus `f39c1d7`, nicht neue Prüfsummen. Entwicklungsname,
Protokoll 3 und `second_edition_implemented=false` beibehalten. Kein Release,
Push oder Parent-Pin-/Lock-Update. Keine weitere Phase automatisch beginnen.
