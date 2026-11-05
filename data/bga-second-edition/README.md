# BGA-Zielvertrag, Format 1

Stand: 5. November 2026. Paket 1 des [Migrationsplans](../../docs/MIGRATION_PLAN.md).
`targets.json` ist die geprüfte, versionierte Zieldefinition des Grundspiels:
80 Karten-/Zeitalterzeilen, 77 Kartentypen und 14 Wunderseiten. Sie wird von
der laufenden Legacy-Engine noch nicht geladen. Ihr Entwicklungs-Regelname
lautet `bga-second-edition-development`, `second_edition_implemented=false`.
Der Vertrag des tatsächlich laufenden Binarys steht in
[`engine-contract.json`](../../engine-contract.json). Seit Paket 3 trägt er
denselben Entwicklungsnamen: Die Wissenschaftswertung ist korrigiert, Karten
und Wunder verwenden weiterhin Legacy-Daten; die vollständige zweite Edition
ist noch nicht implementiert.
`format_version=1` bezeichnet dieses Datenschema, nicht Transportversion 2.

## Herkunft und Prüfung

Die `new`-Felder des vorhandenen, geprüften Vergleichs wurden übernommen und
gegen die lokalen statischen BGA-Definitionen abgeglichen. Keine neue Recherche
und keine erneute Interpretation der früheren CSV-Kosten. `provenance.json`
enthält Quelldatum, Quell-URLs, Ausgangscommits, SHA-256 der tatsächlichen
Eingaben sowie die vier bereits geprüften Kostenkorrekturen mit vorherigem Wert.
Die `comparison_input_sha256` dokumentieren zusätzlich die historischen
Eingaben des Vergleichsgenerators; sie sind keine Laufzeitabhängigkeiten.

`reference/card_types.json`, `reference/wonders.json` und
`reference/manifest.json` sind bytegleiche Kopien der drei gehashten lokalen
BGA-Eingaben. Sie bleiben ein unveränderter Auditbeleg; künftige Generatoren
lesen die normalisierte `targets.json`. Die Referenz enthält statische Daten,
keinen BGA-Client- oder Serverregelcode und keine Bilder.

Aus jedem Arbeitsverzeichnis, auch in einem eigenständigen Engine-Klon:

```sh
python3 /pfad/zur/engine/scripts/audit_bga_targets.py --check
```

Zusätzlich im ursprünglichen gemeinsamen Workspace:

```sh
python3 scripts/audit_bga_targets.py --check --workspace ..
python3 -m unittest discover -s tests -p 'test_bga*.py' -v
```

Der optionale Workspace-Abgleich prüft die Eingabeprüfsummen und vergleicht
sämtliche normalisierten Kartenwerte, Wunderstufen und Zuordnungen erneut mit
`comparison.json`. Der Standardaudit benötigt weder das RL-Projekt noch Bilder,
Netzwerk, Build oder zusätzliche Python-Pakete. Er prüft Werte und Zuordnungen
gegen die eingefrorenen BGA-Daten, Bestands-IDs gegen die C++-Enums sowie
vollständige Decks und reziproke Ketten. CI führt ihn und die Regressionen aus.

## Schema und Identität

| Feld | Vertrag |
| --- | --- |
| `cards[].key` | Eindeutige Kartenzeile, etwa `glassworks_age2`; stabiler Verweis für Ketten |
| `engine_id`, `engine_symbol` | Bestehende oder reservierte künftige C++-Identität; keine BGA-/Sprite-ID |
| `legacy_engine_id`, `legacy_name` | Tatsächliche alte Karte, bei neuen Karten `null` |
| `comparison_engine_id` | Historische Vergleichsposition; allein keine Identitätszusage |
| `bga_id`, `bga_name` | ID und unveränderter Name der eingefrorenen BGA-Definition |
| `name`, `age`, `color` | Normalisierter Anzeigename, Zeitalter 1–3, Farbe |
| `cost` | Ressourcenname → positive Menge; fehlend bedeutet 0; `coins` sind Bankkosten |
| `effects` | Geprüftes Effektvokabular des Vergleichs, siehe unten |
| `copies` | Spielerzahlschwellen je zusätzlicher Kopie, etwa `[3, 4, 7]` |
| `counts_by_players` | Gesamte Kopien je Spielerzahl mit Schlüsseln `"3"` bis `"7"` |
| `chain_from`, `chain_to` | Sortierte Listen eindeutiger Kartenzeilenschlüssel; Vorgänger sind Alternativen |
| `sprite_index`, `sprite` | BGA-Bildslot und Rechteck in der externen Bilddatei; keine Engine-ID |
| `wonders[].key`, `engine_symbol` | Bisheriger Enum-Name, etwa `gizah_a` |
| `wonders[].engine_id`, `bga_id`, `side` | Engine 0–13, BGA 1–14, explizite Seite `a`/`b` |
| `wonders[].starting_resource` | Eine Startressource, unabhängig von den Baustufen |
| `wonders[].stages` | Geordnete Stufen, jede mit eigenem `cost`, `vp` und `effects` |
| `legacy_card_ids`, `retired_cards` | Vollständiger Bestand einschließlich `none=0` und reservierter entfallener Karten |

Ressourcen heißen `wood`, `ore`, `clay`, `stone`, `loom` (Stoff), `glass`,
`papyrus`, `coins`. `production` nennt tatsächliche Mengen, `choice` wählbare
Alternativen; `tradable` beschreibt die Handelbarkeit. Wissenschaft heißt
`tablet`, `gear`, `compass` oder `joker`. Direkte Effekte heißen `vp`, `coins`,
`shields`, `discount`. Zähleffekte enthalten `vp_per` und/oder `coins_per`,
`count` und `target` (`self`, `neighbors`, `self_and_neighbors`).
`includes_self` zählt auch die gerade gebaute Karte; `condition=wonder_complete`
ist die Endwertungsbedingung der Decorators Guild. Die Wunder-Effektbezeichner
sind Zielsemantik; sie sind nicht die numerischen alten `EFFECT`-Enums und
belegen noch keine funktionierende Engine-Implementierung.

Alle bestehenden Kartentypen behalten ihre IDs. Die Ausnahme ist keine
Umnummerierung: Strategists Guild entfällt im Ziel, ihre **73 bleibt reserviert**.
Der alte Vergleich verwendet diese Position für Decorators Guild; deshalb
bleibt dort `comparison_engine_id=73`, während die neue Karte **78** erhält.
Ludus erhält **76**, Castrum **77**. Diese drei IDs sind hier reserviert;
`include/card.h` wird erst mit der späteren Kartenmigration erweitert.
Loom (15), Glassworks (16) und Press (17) teilen ihre jeweilige Engine-ID
über zwei Zeitalter, bleiben aber sechs eindeutige Zielzeilen mit verschiedenen
BGA-IDs und teilweise anderen Deckmengen.

Die expliziten Namenszuordnungen sind Pawnshop → Well, Apothocary → Apothecary,
Craftsmen Guild / alter Engine-Name Craftmens Guild → Craftsmens Guild und
Bazaar → Bazar. Die [vollständige Zuordnung](MAPPING.md) zeigt alle 94 Zeilen.

## Decks und Bilder

Die zehn Gilden bilden einen Auswahlpool: In Zeitalter III werden `Spieler + 2`
verschiedene Gilden gewählt. Ihre `[3]`-Schwelle bedeutet eine Karte im Pool,
nicht zehn automatisch beigemischte Gilden. Reguläre Karten plus ausgewählte
Gilden ergeben pro Zeitalter 21/28/35/42/49 Karten für 3/4/5/6/7 Spieler.
Nur **drei Spieler** sind als Engine-Spielerzahl vorgesehen; die übrigen Mengen
bleiben Referenzwerte und erweitern keine Laufzeitzusage.

Kartenrechtecke: `x=(sprite_index % 10)*255`, `y=(sprite_index // 10)*393`,
Größe 255 × 393. Wunderrechtecke: `x=0`, `y=(bga_id-1)*223`, Größe 450 × 223.
Die drei grauen Typen teilen ihr Bild über die Zeitalter; Sprite-Indizes sind
deshalb absichtlich nicht je Zeile eindeutig. Das eingefrorene Manifest nennt
Hashes, URLs, Dimensionen und tatsächliche MIME-Typen. Insbesondere enthalten
`img/cards_v2.jpg` und `img/boards_v2.jpg` WebP. Grafikdateien bleiben extern;
BGA-Karten brauchen später ein Namensoverlay.

## Entscheidung für spätere C++-/Python-Erzeugung

Nach der Migration dient **diese normalisierte `targets.json`** als gemeinsame
fachliche Quelle. Ein deterministischer Generator im Engine-Repository soll
daraus beim Build C++-Tabellen/Header und ein sprachneutrales Exportartefakt
mit Schema-, Regelnamen- und Quelldatenhash erzeugen. Die Tabellen werden in
das Binary kompiliert. Ein isolierter Spielprozess benötigt keine Quelldatei
und keinen Pfad relativ zum Arbeitsverzeichnis. Ressourcen/Effekte werden
dabei ausdrücklich auf C++-Enums und implementierte Handler abgebildet;
Sonderentscheidungen lassen sich nicht allein durch Datentabellen ersetzen.

Der Python-Export konsumiert dasselbe geprüfte Artefakt oder beim Paketbau
eine Kopie derselben Quelle, anschließend als Paketressource. Die generierten
Dateien sind Ableitungen und erhalten einen reproduzierbaren Diff-/Hashcheck;
es entstehen keine unabhängig von Hand gepflegten zweiten Zielwerte und keine
Rückextraktion der neuen Regeln aus C++ per Regex. Diese Generatorintegration
gehört zur späteren Karten-/Wundermigration und RL-Übernahme. Paket 2 repariert
zunächst ausdrücklich die alten Exporter gegen den alten Regelstand.

## Abnahmegrenzen

Die Definitionen ergeben für diesen Datenauftrag keine zusätzliche offene
Regelfrage (`open_rule_questions=[]`). Sie beweisen kein BGA-Serververhalten.
Paket 3 hat inzwischen die Wissenschaftsjoker korrigiert. Simultane Auflösung,
Ablageentscheidungen und Ausführung neuer Fähigkeiten bleiben bei den folgenden
Paketen. Kartenkosten/-punkte, Deckmengen und Wunderstufen der laufenden Engine
wurden durch die Datenvorbereitung und Wissenschaftskorrektur nicht geändert.
GUI-, RL-Feature- und Trainingsmigration sind ebenfalls offen.
