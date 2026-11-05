# Erzeugte Kartentabellen – Paket 5-I

`data/bga-second-edition/targets.json` ist die einzige gepflegte Quelle für
die Kartenfelder. Der eingefrorene Zielaudit prüft sie unabhängig gegen die
aufbewahrten BGA-Definitionen und die stabilen Engine-Enums. Historische
Konstruktoren stehen weiterhin im alten Tag; ihre Eingabeprüfsummen in
`provenance.json` werden nicht auf neue Quelldateien umgebogen.

```sh
python3 scripts/audit_bga_targets.py --check
python3 scripts/generate_card_data.py
python3 scripts/generate_card_data.py --check
```

Der Standardbibliotheksgenerator erzeugt bytegleich aus derselben Quelle:

- `include/card_data.generated.h`: C++17-Tabelle aller 80 Karten-/Zeitalterzeilen.
- `data/generated/cards-v1.json`: sprachneutraler Export, `format_version=1`,
  Entwicklungs-Regelname, Quellpfad/-format und SHA-256 der gesamten Zielquelle,
  vollständige Kartenmetadaten einschließlich Ketten/Bildzuordnung sowie
  ausdrücklich abgegrenzter Laufzeitumfang.

`--check` schreibt keine Dateien, auch bei Abweichungen. Alle Pfade sind an
den Skriptort gebunden. Der Engine-Build verwendet eingecheckte Tabellen und
benötigt keinen Generator, keine Python-Installation, keine RL-Dateien und
keine Bilder. Das Binary benötigt zur Laufzeit keine Daten-JSON. Ein Consumer
muss Format, Quellenhash und Laufzeitumfang prüfen; der neue Export ersetzt
hier noch keine RL-Datenbank oder deren Legacy-Exporter.

`Card::FromDefinition(id, age)` ist die strikte Deckfabrik. Kosten, Namen,
Farbe, Mengen, feste Effekte und alternative Ketten stammen aus der Tabelle.
Graue Altersvarianten bleiben getrennt. Die ältere Fixture-Namenssuche
verweigert mehrdeutige Zeitalter; ein explizites Alter löst sie auf. Nur der
alte synthetische Testkonstruktor erlaubt frei vorgegebene Kosten/Ketten und
bei einem eindeutig einzeiligen Typ einen Effekt-Lookup trotz abweichendem
Fixture-Alter. Reale Karten durchlaufen ausschließlich die strikte Fabrik.

Der Deckaufbau verwendet für drei Spieler 21 Karten pro Ära. Ära III wählt
fünf verschiedene Gilden aus zehn; Decorators 78 ersetzt im Pool Strategists
73, ohne die reservierte Identität zu übernehmen. Ludus 76 und Castrum 77
haben bei drei Spielern keine normale Kopie; ihre Definitionen und festen
Felder sind dennoch vorhanden. Die Referenzmengen für vier bis sieben Spieler
belegen keine Unterstützung dieser Spielerzahlen. Physische Kennungen werden
weiterhin pro Kopie vergeben und bei Ortswechseln erhalten.

Produktionsplaner und Gebäudeanwendung lesen dieselben Produktionsmengen,
Wahlalternativen und Handelbarkeiten. Bankkosten, Schilde, Rabatte, feste
Münzen, blaue Punkte und Wissenschaftssymbole stammen ebenfalls aus dieser
Tabelle. Vorhandene Zähleffekte verwenden ihre bisherigen Zählregeln mit
Tabellenfaktoren/Zielen; simultane Erträge und RQ-1/RQ-2 bleiben erhalten.
Die Wissenschaftsoptimierung und die alte Gildenkopie-Kandidatenwahl werden
nicht geändert. Wunderdefinitionen bleiben Legacy.

**Offen:** Ludus-Zählung und Decorators-Bedingung werden getrennt in 5-II
implementiert. Ihre Zielmetadaten bleiben vollständig erhalten;
`runtime_complete=false` sperrt ihre neuen Münz-/Punktebeiträge. Insbesondere
liefert Decorators derzeit auch beim vollendeten Wunder null statt sieben
Punkte. Ludus liefert derzeit null statt drei Münzen/einem Punkt je roter
Karte. `engine.card_data` nennt die beiden fehlenden Effekte ausdrücklich;
`second_edition_implemented=false`, Release `v0.2.0-dev` und Protokoll 3 bleiben.

Die Regression `card_tables` prüft gezielte Vorher-Fehler, alle Zeilen im
echten Angebots-/Baupfad, sämtliche Ketten, exakte und unzureichende Kosten,
Produktionsmengen, einmalige Auswahlquellen und die Grenze zu 5-II.
`compiled_card_data` vergleicht alle kompilierten Felder mit `targets.json`,
prüft 40 reale Deckziehungen und den schreibfreien, deterministischen
Generator in einem isolierten Verzeichnis. Die bestehenden Regeln- und
Protokolltests bleiben erforderlich. Ergebnisse und Commit stehen in
[card-data-validation.json](card-data-validation.json).
