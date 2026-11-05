# Arbeitsplan: alte GUI bis zur spielbaren BGA-Edition

Stand: 5. November 2026. Dieser Plan ist die maßgebliche Reihenfolge für die
weitere Migration. **Aktuelle Nutzerfreigabe: alle verbleibenden Pakete bis einschließlich 9, nacheinander.**
Zum Abschluss werden Ergebnis, Tests, Commit und verbleibende Grenzen
eingetragen. Die Pakete werden gemäß der erweiterten Nutzerfreigabe vom 5. November 2026
nacheinander umgesetzt und einzeln geprüft/committed. Der [fertige nächste Agentenauftrag](NEXT_AGENT_TASK.md)
begrenzt die nächste Sitzung auf Paket 5-IIa (ausschließlich Ludus).

## Ziel und feste Entscheidungen

- Ziel ist das Grundspiel der neuen Edition nach den bereits gesammelten
  **BGA-Definitionen**. Erweiterungen, Promos und weitere Spielerzahlen sind
  eigene spätere Vorhaben. Die Engine unterstützt zunächst drei Spieler.
- Engine, Spielregeln, GUI und Engine-Tests gehören in dieses Repository.
  RL, Feature-Engineering, Python-Datenexporte und Training bleiben separat.
- Die alte Implementierung bleibt über Tags reproduzierbar. Ein paralleler
  umschaltbarer Betrieb beider Editionen ist derzeit nicht erforderlich.
- Paket 0 hat die alte GUI wiederhergestellt, Paket 1 die BGA-Ziele und IDs
  eingefroren und Paket 2 die Legacy-Exporte repariert. Paket 3 hat den
  Laufzeitumbau mit optimaler Wissenschaftswertung im Entwicklungs-Regelstand
  begonnen. Paket 4a spezifiziert zusätzliche Entscheidungen und simultane
  Auflösung. Paket 4b-I sichert die interne Ablageentnahme; Paket 4b-II
  implementiert gemeinsame Auflösung, Pflichtentscheidungen und Transport 3.
  Paket 5-I übernimmt die erzeugten Kartentabellen. Ludus-/Decorators-Effekte,
  Wunder und Grafik werden in den jeweiligen Folgeaufträgen umgesetzt.
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
| `docs/SCIENCE_SCORING.md`, `docs/science-validation.json` | Korrigierte Wissenschaftswertung, Testnachweis und getrennte Kopier-/Python-Vorschaugrenzen aus Paket 3 |
| `docs/DECISION_PROTOCOL_DRAFT.md`, `docs/decision-protocol-examples.json`, `docs/decision-protocol-validation.json` | Entscheidungsvertrag aus Paket 4a; Quellen, Nachrichten und Abnahmefälle. RQ-1/RQ-2 sind inzwischen durch Nutzervorgabe entschieden; vollständiger Automat noch nicht implementiert. |
| `docs/DISCARD_SAFETY.md`, `docs/discard-safety-validation.json` | Interne Ablagesicherheit, drei rote Fehlerfälle, Transaktions-/Instanznachweis und verbleibende V2-Grenzen aus 4b-I |
| `docs/SIMULTANEOUS_RESOLUTION.md`, `docs/resolution-validation.json` | Implementierter Protokoll-3-Auflösungskern, Tests, aktuelle Client-/Fähigkeitsgrenzen aus 4b-II |
| `data/bga-second-edition/README.md`, `targets.json`, `provenance.json` | Eigenständiger geprüfter Zielvertrag, Schema, Zuordnungen und Eingabe-Prüfsummen aus Paket 1 |
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

Die geprüften Zieldaten sowie die drei statischen BGA-JSON-Eingaben sind seit
Paket 1 eigenständig im Engine-Repository versioniert. Der vollständige alte
Vergleich bleibt im RL-Projekt. Bilder und BGA-Clientquellen bleiben extern;
der Zielaudit benötigt sie nicht. Bilddateien bleiben über einen externen
lokalen Ordner verwendbar.

## Reihenfolge und Abschlusskriterien

| Paket | Status | Begrenzter Auftrag | Fertig, wenn … |
| --- | --- | --- | --- |
| 0 – Legacy-GUI | Erledigt, technische Abnahme siehe GUI-Audit | Alte Oberfläche wieder anbinden und kompatibel machen | Alte Partien mit Endwertung, Session-Isolation und Tests funktionieren; Regeln unverändert |
| 1 – Zielvertrag und Zuordnung | Erledigt, Abnahme unten | Vorhandene BGA-Ziele versioniert festhalten und IDs/Schema zuordnen | 80 Kartenzeilen und 14 Seiten vollständig; Prüfsummen/Quelle dokumentiert; alle Zielkarten und Ketten eindeutig zugeordnet; kein Laufzeitumbau |
| 2 – Exporter reproduzierbar machen | Erledigt, Abnahme unten | Im RL-Projekt die alten Exportfehler beheben | Wunder-ID 0, Produktionsmengen und Mehrfach-Zeitalter korrekt; deterministischer Export mit geprüftem JSON-Diff und Regressionen |
| 3 – Wissenschaftswertung | Erledigt, Abnahme unten | Gierige Jokervergabe durch optimale Endwertung ersetzen | Dokumentierte Gegenbeispiele und Kombinationen mit mehreren Jokern/Quellen bestehen; keine Kartendaten-/GUI-Änderung |
| 4a – Entscheidungsvertrag | Erledigt als Spezifikation, Abnahme unten | Ablagewahl und simultane Auflösung als Protokollzustände spezifizieren | Eingaben, sichtbare Optionen, Sitzreihenfolge, Timing, Abbruch und Terminalzustand beschrieben; die beiden ursprünglichen Regelfragen inzwischen durch Nutzer entschieden |
| 4b-I – Sichere Ablageentnahme | Erledigt, Abnahme unten | Internen Ablagebau mit stabiler Instanz, atomarer Entnahme und korrektem Rechtverbrauch absichern | Keine Handentnahme über Ablageindex; zentrale Ablage genau einmal geändert; Fehler, Wiederholung und leere Auswahl geprüft; kein neuer interaktiver Transport |
| 4b-II – Runden-/Zahlungsauflösung | Erledigt, Abnahme unten | Gemeinsamen Anfangszustand, Zahlungen, Folgeentscheidungen und Nachrichtenvertrag implementieren | Gleichzeitigkeit, Restkarten, siebte Karte, Abschlussbarrieren und Nachrichtenledger geprüft; RQ-1/RQ-2 gemäß ausdrücklicher Nutzervorgabe umgesetzt |
| 5-I – Kartendatentabellen | Erledigt, Abnahme unten | Geprüfte Kartendaten deterministisch in Engine-Tabellen übernehmen | Alle 80 Zeitalterzeilen, Kosten/Punkte/Ketten, Mengen und feste Effekte gegen Ziele geprüft; IDs erhalten; neue Sondereffekte getrennt und ausdrücklich offen |
| **5-II – Neue Kartensondereffekte** | Erledigt: Ludus und Decorators, Abnahme unten | Ludus und Decorators Guild einzeln umsetzen und abnehmen | Neue Zähl-/Wertungseffekte mit gezielten Gegenbeispielen geprüft; keine bloße Datenmigration als vollständige neue Edition ausgegeben |
| 6 – Neue Wunder | Erledigt, familienweise Abnahme unten | Die 14 Seiten familienweise umstellen | Jede Stufe: Kosten, Ertrag, Verfügbarkeit und richtige Anzahl geprüft; Sonderfähigkeiten mit gezielten Fällen abgenommen |
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

## Abnahme Paket 1 – 5. November 2026

Ergebnis: [eigenständiger Zielvertrag](../data/bga-second-edition/README.md)
mit 80 Kartenzeilen, 77 Kartentypen, 14 Wunderseiten, 25 Kettenverbindungen,
vollständigem [Mapping](../data/bga-second-edition/MAPPING.md), gehashten
BGA-Eingaben und den vier übernommenen Kostenkorrekturen. Strategists-ID 73
bleibt reserviert; neue Karten erhalten 76–78. Der Vertrag legt die spätere
gemeinsame Erzeugung von C++-Tabellen und Python-Export fest.

Implementierungscommit: `b916ea45d109149a96f4aa6837bdb1f442a3ef1e`
(`Freeze BGA migration targets while retaining legacy runtime rules`).
Der anschließende Dokumentationscommit hält diese Abnahme und Paket 2 fest.

Prüfungen: Zielaudit einschließlich Original-Workspace-Abgleich; elf neue
Datensatz-/Fehlermutationsregressionen und acht bestehende GUI-/Sessiontests
(19 Python-Tests); Release-Build und alle fünf CTests; Herkunftsaudit für
49 C++-/Build-/Testdateien unverändert gültig. Ein Archiv des tatsächlichen
Implementierungscommits besteht Zielaudit und elf neue Tests ohne RL-Projekt,
Bilder oder Quellpfade relativ zum Arbeitsverzeichnis. Maschinenlesbarer
Nachweis mit Artefaktprüfsummen: [bga-target-validation.json](bga-target-validation.json).

Grenzen: keine Laufzeit-, C++-, GUI-, Exporter- oder Trainingsänderung.
Statische BGA-Daten belegen kein vollständiges Serververhalten.
`engine-contract.json` bleibt Legacy; das inaktive Ziel trägt einen eigenen
Entwicklungsnamen und `second_edition_implemented=false`. Submodule-Pin und
`engine.lock.json` im RL-Projekt werden hier nicht aktualisiert. Der damalige
Folgeauftrag war Paket 2 zur Reparatur ausschließlich der alten Exportwege.

## Abnahme Paket 2 – 5. November 2026

RL-Implementierungscommit: `a283d5897d01b8edd32021562271698f1d41a3ee`
(`Fix legacy C++ data exports without changing runtime rules`), Branch
`refactor/separate-engine-20261105`. Der Engine-Commit zu dieser Abnahme
ändert ausschließlich diesen Plan und den nächsten Agentenauftrag.

Ergebnis: Gemeinsamer Enum-Parser erhält Wunder-IDs 0–13. Alle fünf
Spielerzahlspalten 3–7 werden für jede der 78 Legacy-Kartenzeilen exportiert;
Loom, Glassworks und Press behalten ihre getrennten Altersmengen. Konflikte
bei Kosten/Ketten zwischen Altersvarianten und doppelte Zeilen werden
abgelehnt. Die vier Doppelproduzenten erhalten Menge 2 im bereits unterstützten
Ressourcenobjektformat; zwei veraltete Forum-/Caravansery-Flags entfallen.
Der geprüfte Diff betrifft nur diese Felder der alten Kartendatenbank.
Wunder- und Mechanikdateien bleiben bytegleich. `--output-dir`, `--engine-root`
und schreibfreies `--check` erlauben Prüfung vor Übernahme; CI prüft die Exporte.

Prüfungen: 18 gezielt nachgewiesene Fehlerfälle vor der Reparatur;
27 neue Exportregressionen und insgesamt 66 bestandene betroffene Tests,
einschließlich Datenbank-, Produktions-/Handels-, Beobachtungs- und
Checkpoint-Verträgen. Beide Exportprüfungen und der Editionsaudit sind grün.
Zwei Exporte aus einem isolierten Archiv des tatsächlich gepinnten
`f39c1d7ead271df437269b7f555a2c7a0d4b9ad1` sind bytegleich und stimmen mit
allen drei Paketdateien überein. Der Herkunftsaudit für 49 C++-/Build-/Testdateien
bleibt gültig. [Anleitung](../../docs/guides/LEGACY_DATA_EXPORT.md) und
[maschinenlesbarer Nachweis](../../docs/reference/legacy-export-validation.json)
liegen im RL-Projekt.

Grenzen: Engine-Regeln, C++, GUI, Featurecode und Trainingsstände bleiben
unverändert. Die Datenkorrektur ändert den bestehenden `public_schema_hash()`;
keine Kompatibilitätsprüfung wurde umgangen. Die bekannten Legacy-Decklücken
ab vier Spielern bleiben als C++-Quellstand abgebildet; die Engine unterstützt
weiter nur drei Spieler. Keine BGA-Aktivierung, kein Push/Release, keine
Änderung von RL-Submodule-Pin oder `engine.lock.json`. Der damalige Folgeauftrag
war Paket 3 zur optimalen Wissenschaftswertung.

## Abnahme Paket 3 – 5. November 2026

Implementierungscommit: `11de177935bb001c43044e90a1215a6dc8d1c49e`
(`Optimize science jokers and mark the BGA development ruleset`).
Der anschließende Dokumentationscommit hält die Abnahme und Paket 4a fest.

Ergebnis: `CalculateScientificScore()` bewertet alle vollständigen
Jokerverteilungen optimal. Workshop, Laboratory, Apothecary und Scientists
Guild ergeben jetzt 13 statt 10 Punkte. Bisherige Scientists-Guild-/Babylon-
Quellen und Stufengrenzen bleiben gleich; tatsächlich kopierte Gilden werden
als eigene Einträge genau einmal gezählt. Wiederholte Wertung und Veröffentlichung
bleiben stabil. Der aktive Vertrag lautet `v0.2.0-dev` mit
`ruleset=bga-second-edition-development`, `second_edition_implemented=false`
und unveränderter Transportversion 2.

Prüfungen: roter C++-Gegenbeispieltest vor der Korrektur; 375 Bestände gegen
einen unabhängigen rekursiven Prüfer, acht Babylon-Stufengrenzen, Kopierquellen
und kontrollierte Schlussrundenveröffentlichung mit konsistenten Kategorien,
Gesamtpunkten und `results.txt`. Release-Build, alle sechs CTests und 19
Python-Tests sind grün. Der BGA-Zielaudit bleibt grün; C++-Patch und Inventar
sind aktualisiert, Herkunftsaudit für 50 C++-/Build-/Testdateien grün.
Die vorhandene Python-Zustandsinterpretation und passive Diagnose stimmen
mit den tatsächlich veröffentlichten Schluss-Wissenschaftspunkten überein.
Details: [SCIENCE_SCORING.md](SCIENCE_SCORING.md),
[science-validation.json](science-validation.json).

Grenzen: Die getrennte alte `CopyGuild()`-Kandidatenwahl schätzt den Wert
der Scientists Guild weiterhin näherungsweise (nachgewiesen: ohne feste
Symbole wird eine mögliche Kopie mit einem Punkt abgelehnt). Die Python-
Marginalvorschau ohne vorhandene Joker liefert bei einem dokumentierten
Fall 8 statt 5 optimale Zusatzpunkte; das bleibt für Paket 8 abgegrenzt.
Keine Karten-/Wunderdaten, Fähigkeiten, GUI, RL-Features oder Trainingsstände
verändert. Keine Freischaltung vollständiger BGA-Regeln, kein Push/Release,
kein RL-Submodule-/Lock-Update. Der damalige Folgeauftrag war Paket 4a zur
Spezifikation zusätzlicher Entscheidungen und simultaner Auflösung.

## Abnahme Paket 4a – 5. November 2026

Spezifikationscommit: `0aaea3651915b071b95f2ae0f87d243b1845c126`
(`Specify BGA development decision protocol without changing runtime`).
Der anschließende Dokumentationscommit hält diese Abnahme und ausschließlich
Paket 4b-I als nächsten Auftrag fest.

Ergebnis: [Entscheidungs- und Auflösungsvertrag](DECISION_PROTOCOL_DRAFT.md)
`decision-v1-draft` mit vorgeschlagener, **nicht implementierter** Transportversion 3.
Beschrieben sind Anfangszustand und gemeinsame Zahlungen, der getrennte
Zählzustand gleichzeitiger Soforteffekte, reguläre Sammlung und Auflösung,
nachgelagerte Ablagewahl, stabile physische Karten-/Entscheidungskennungen,
Quellberechtigungen und deren einmaliger Verbrauch. Die Warteschlange öffnet
erst nach regulärem Commit und fälligen Restkarten; neue Babylon-Züge und
Ären-/Schlusswertung warten auf ihre Abschlussbarrieren. Quittierungen,
Wiederholung, ungültige Eingaben, Timeout und Abbruch sowie Sichtbarkeits- und
GUI-/RL-Anforderungen sind festgehalten.

Prüfungen: [34 Nachrichtenbeispiele und 36 Akzeptanzfälle](decision-protocol-examples.json)
auf JSON-Syntax, eindeutige Kennungen, Querverweise, Statusbedingungen,
Karten-/Alterszuordnung gegen die eingefrorenen Ziele, Grant-/Ereignisverweise,
Eingabe-/Optionsbindung, Wiederholung, Bundlekennung und Endresultatkonsistenz
geprüft. 23 lokale Dokumentationsverweise gültig. Zwei Fälle sind ausdrücklich
von offenen Regelfragen abhängig. Die Fälle sind **künftige Abnahmeanforderungen**,
keine ausgeführten Protokoll-3-Laufzeittests. Herkunftsaudit unverändert grün
für 50 C++-/Build-/Testdateien; BGA-Zielaudit einschließlich Workspace-Abgleich
grün; `git diff --check` grün. Keine neuen Build-/CTest-/GUI-/RL-Läufe, da
ausschließlich Dokumentation geändert wurde. Quellen, Umfang und Prüfsummen:
[decision-protocol-validation.json](decision-protocol-validation.json).

Grenzen: **RQ-1** betrifft reguläre Vineyard-/Bazar-Zähler bei späterem
Nachbar-Ablagebau; die getrennte Auszählung ist eine gekennzeichnete
Arbeitsauslegung. Vor Freigabe der betroffenen Münzauflösung in 4b-II bestätigen.
**RQ-2** betrifft freiwilligen Verzicht auf eine vorhandene legale Ablageoption;
vor interaktiver Freischaltung bestätigen. Statische BGA-Daten sind kein
Serverregelcode. Beide Fragen blockieren die interne sichere Entnahme in
4b-I nicht. Technische Mehrfach-Grant-Fälle behaupten keine Erweiterungsregeln.

Der aktive Vertrag bleibt `v0.2.0-dev`,
`ruleset=bga-second-edition-development`, `second_edition_implemented=false`,
Transportversion **2**. Kein C++-/GUI-/RL-, Karten-/Wunder-, Wissenschafts-,
Export-/Feature-/Trainingseingriff. Keine Veröffentlichung und kein Release;
RL-Pin und `engine.lock.json` bleiben bei `f39c1d7`. Der nächste Auftrag ist
ausschließlich **4b-I – sichere Ablageentnahme**; er ist vorbereitet und hier
nicht begonnen. Runden-/Zahlungsauflösung bleibt ein eigener Folgeauftrag.

Nachtrag aus 4b-I: Die oben historisch offenen RQ-1/RQ-2 hat der Nutzer am
5. November 2026 ausdrücklich mit **nein/nein** entschieden. Der aktuelle
Entwurf und die Akzeptanzfälle sind entsprechend aktualisiert; die ursprünglichen
Abnahmeprüfsummen bleiben als historischer Nachweis erhalten. Es ist keine
erneute Regelbestätigung erforderlich.

## Abnahme Paket 4b-I – 5. November 2026

Implementierungscommit: `85d82561a554fab3f8ca5fec05b2dfe098a9d055`
(`Secure development discard construction with atomic physical-card removal`).
Der anschließende Dokumentationscommit hält diese Abnahme und ausschließlich
Paket 4b-II als nächsten Auftrag fest.

Ergebnis: Physische Kartenkopien besitzen innerhalb einer Partie stabile
Kennungen. Die zentrale Ablage wird per Referenz und Instanz angesprochen;
Gebäudeanwendung und Entnahme aus Hand/Ablage sind getrennt. Ablagebau bereitet
Spieler und Restablage vor und übernimmt Karte, Stadteintrag, Effekte und
Rechtverbrauch gemeinsam über nicht werfende Operationen. Ungültige Eingaben
ändern das offene Recht nicht; null legale Optionen schließen es automatisch
ohne Ersatz. Bereits verbrauchte/abgelaufene Quellstufen erteilen kein neues
Recht. Bestehende Halikarnassos-Aktivierungsstufen bleiben unverändert.
V2-Namensvorwahlen gehören jetzt dem jeweiligen Sitz; mehrdeutige Namen bauen
nichts. Details: [DISCARD_SAFETY.md](DISCARD_SAFETY.md).

Regelentscheidungen des Nutzers: **RQ-1 nein**, spätere Ablagekonstruktionen
ändern reguläre Münzauszahlungen nicht rückwirkend. **RQ-2 nein**, bei legalen
Optionen kein freiwilliger Verzicht. Diese verbindlichen Projektentscheidungen
sind im Entwurf und D14/D26 hinterlegt; kein zusätzlicher BGA-Servernachweis
wird behauptet. Die sichtbare Durchsetzung der Wahlpflicht folgt erst in 4b-II.

Prüfungen: drei rote C++-Bestands-/Rechtregressionen vor der Korrektur.
`discard_safety` prüft unter anderem zwei physische Kopien, Altersduplikate,
Handisolation, fehlendes/falsches/verbrauchtes Recht, Wiederholung, echte
Halikarnassos-Quellstufen, Leerfall, Sitzbindung im vollständigen V2-Dateipfad
und die Erhaltung aller 63 Deckkennungen beim Austeilen/Weiterreichen.
**63 erzwungene Allokationsfehler** vor der Übernahme hinterlassen jeweils
unveränderte Originalzustände und einen erfolgreichen gültigen Wiederholungsversuch.
Release-Build, alle **sieben CTests**, **19 Python-/Sessiontests**, eigenständiger
BGA-Zielaudit sowie Herkunftsaudit für **51** Dateien grün. 34 Nachrichten-
beispiele und 36 Akzeptanzfälle erneut auf Konsistenz geprüft; keine offenen
RQ-1/RQ-2-Fälle mehr. Artefakte und Umfang:
[discard-safety-validation.json](discard-safety-validation.json).

Der optionale historische Workspace-Zielaudit meldet nach dem erlaubten Umbau
die geänderte `engine/include/card.h`-Prüfsumme. Derselbe Audit besteht in einem
temporären Workspace mit den echten eingefrorenen `f39c1d7`-Quelldateien und
den unveränderten Originalreferenzen. Historische Herkunftswerte wurden nicht
umgeschrieben. Karten-Deklarationen und Gebäudeeffekt-Switch sind bytegleich;
Wunder-/Wissenschafts-/Gildenkopierlogik und Zieldaten unverändert.

Grenzen: V2 besitzt weiterhin nur eine alte Namensvorwahl und kann die sichtbare
Pflichtauswahl noch nicht erzwingen. Unerfüllte Vorwahlen bauen nichts; ihr
Recht verfällt am Zugfenster. Noch kein vollständiger Grant-/Request-Automat,
keine simultane reguläre Auflösung, Restkarten-/Babylon-/Ärenbarriere oder
Protokoll-3-/GUI-/RL-Integration. Keine Karten-/Wunderwerte, Exporte, Features
oder Trainingsdateien geändert. Die spätere Übernahme der Entwicklungs-Engine
benötigt einen bewussten Exportweg für den ausgelagerten Gebäudeeffektcode.
Aktiver Vertrag weiter `v0.2.0-dev`, `second_edition_implemented=false`,
Transportversion **2**. Kein Push/Release und kein RL-Pin-/Lock-Update.
Der damalige Folgeauftrag war Paket 4b-II zur vollständigen Runden-/
Zahlungsauflösung; die oben beschriebenen V2-Grenzen sind der historische
Abnahmestand von 4b-I.

## Abnahme Paket 4b-II – 5. November 2026

Implementierungscommit: `6ad395332da5535f3cdc7f7699fea6fd12c07e9f`
(`Resolve frozen turns and mandatory discard decisions in development protocol 3`).
Der anschließende Dokumentationscommit hält diese Abnahme und ausschließlich
Paket 5-I als nächsten Auftrag fest.

Ergebnis: `DecisionEngine` besitzt den tief kopierten Spieler-/Wunder-/Karten-
zustand und plant alle regulären Aktionen gegen denselben Rundenanfang.
Minimal festgelegte Zahlungen und unabhängige Käuferbelegungen verhindern
vorzeitige Nutzung neuer Münzen/Produktion/Rabatte. Erst nach allen Annahmen
werden reguläre Bauten gemeinsam übernommen und ihre Sofortmünzen gezählt;
Lighthouse zählt sich selbst. Tatsächliche Quellstufen erzeugen Grants, die
sichtbare Pflichtwahl benutzt aktuelle physische Ablageinstanzen. Restkarten,
Handweitergabe, neuer Babylon-Zug, Konflikte und Endwertung warten auf ihre
Barrieren. Beide ausdrücklichen Nutzerentscheidungen RQ-1/RQ-2 sind umgesetzt.

Aktiver Transport jetzt **3**, Vertrag **`decision-v1`**, weiterhin
`v0.2.0-dev`, `bga-second-edition-development`,
`second_edition_implemented=false`. Eingabe-/Entscheidungs-/Bundlekennungen,
Quittierungen, Wiederholung und Konflikte, angebotene Revisionen, optionale
Fristen sowie kontrollierter Abbruch sind implementiert. Keine stille
Ersatzaktion. Nur vollständiger Normalabschluss schreibt das Ergebnis einmal.
Der eigenständige Standardbibliotheksclient ist der aktuelle Testweg:

```sh
python3 scripts/smoke_protocol.py --engine build/7Wonders --seed 8
```

Prüfungen: vier rote Ökonomie-/Abschlussfälle gegen `da93178`; sauberer
Release-Build; **neun CTests grün**. Python-Sammlung: **26 Tests**, davon
**21 bestanden und fünf historische V2-GUI-Prozesstests ausdrücklich
ausgelassen**. Die reale Ablehnung von Protokoll 3 durch die unveränderte
GUI ist separat grün. Die sechs neuen Prozessprüfungen umfassen vollständige
Partien, Teilannahmen, identische und widersprüchliche Zustellung, alte Partie
bei gleichem Seed, veraltete Angebote, ungültige Kostenüberschreibung,
fehlerhafte Dateien, ignorierte V2-Dateien, Timeout und Abbruch ohne Resultat.
C++ prüft unter anderem geteilte Minimalzahlungen, simultane Zähler,
Freibaumodi, Pflichtwahl, letzte reguläre/siebte Karte, synthetische Queue,
Leerfälle, neu aktiviertes Babylon und vollständiges Rückrollen nach einem
bewusst erst nach der Stufenmutation ausgelösten Fehler. Ein expliziter Abbruch
während der Schluss-Ablagewahl erhält den vorherigen regulären Commit.

Fünf protokollierte reale Partien mit Seeds 0, 7, 8, 19, 42 prüfen über alle
Zustände Kartenerhaltung, legale tatsächlich angewandte Optionen und konsistente
Ergebnisdateien. Seed 8 enthält eine reale Ablageentscheidung; Seeds 0/8
enthalten Babylon-Züge. Wiederholter Seed bleibt reproduzierbar, die
Partiekennung ist unabhängig davon neu. Terminalwiederholung verändert
weder Wertung/Ereignisse noch den Änderungszeitpunkt der Ergebnisdatei.
Herkunftsaudit für **55** Dateien und eigenständiger BGA-Zielaudit grün.
Details: [SIMULTANEOUS_RESOLUTION.md](SIMULTANEOUS_RESOLUTION.md),
[PROTOCOL.md](PROTOCOL.md), [resolution-validation.json](resolution-validation.json).

Grenzen: Alle Karten-Deklarationen und Wunderstufendefinitionen bleiben
unverändert, ebenso Wissenschaftsformel und alte Gildenkopie-Kandidatenwahl.
Babylon/Olympia verwenden noch ihre bisherigen Fähigkeiten und Stufen;
`legacy_olympia_once` wird ausdrücklich ausgewiesen, keine künftigen ersten/
sechsten Freibaurechte. Noch keine fertige BGA-Edition. GUI und RL sind nicht
auf Version 3 migriert; GUI verwirft das neue Binary, der alte GUI-Tag bleibt
reproduzierbar. Keine GUI-/RL-Laufzeit-, Export-/Feature-/Trainingsänderung,
kein Push/Release und kein Parent-Submodule-/Lock-Update. Paket 5-I ist
vorbereitet, nicht begonnen; dieses Paket endet hier.


## Abnahme Paket 5-I – Kartendatentabellen

Implementierung: **`8b35b280635c248f8c4cdb985d20ad7d86a8c1a1`**.
Weiter `v0.2.0-dev`, `bga-second-edition-development`, Protokoll 3,
`decision-v1`, `second_edition_implemented=false`.

Die eingefrorenen Ziele erzeugen reproduzierbar C++-Tabellen und den neutralen
Export `data/generated/cards-v1.json`, Format 1 mit Quellenhash. Das Binary
enthält die Tabellen; es benötigt keine Daten-JSON oder Pfade zum RL-Workspace.
Alle 80 Zeitalterzeilen/77 Typen, 25 Ketten, Kosten, blaue Punkte, Mengen,
feste Produktion/Münzen/Schilde, Rabatte und Wissenschaftssymbole sind
übernommen. Planer und Gebäudeanwendung lesen dieselben Produktionswerte.
Graue Altersvarianten bleiben getrennt; mehrdeutige Namenssuche verweigert eine
stille Auswahl. Bestehende IDs und physische Kennungen bleiben erhalten;
73 bleibt reserviert, 76/77/78 sind angehängt. Drei-Spieler-Decks haben pro
Ära 21 Karten, darunter in III fünf verschiedene Gilden aus zehn.

Fünf rote Vorher-Fälle zeigen Altar-Punkte, Aqueduct-Kosten, Pantheon-Kette,
Town-Hall-Mengen und den alten Strategists-Pool. Die Regression prüft alle
Zeilen im realen Entscheidungs-/Baupfad, sämtliche Ketten, exakte und
unzureichende Kosten, feste und einmalige Wahlproduktion. Der unabhängige
Vergleich sämtlicher kompilierter Felder mit den Zielen und 40 reale
Deckziehungen sind grün. Generator-Schreibfreiheit, absichtlich veraltete
Artefakte, doppelte bytegleiche Erzeugung und ein isolierter Generatorcheckout
mit unverwandtem Arbeitsverzeichnis sind geprüft.

Release-Build und **zwölf CTests** grün. Python: **31 Tests, davon 26 bestanden
und fünf historische V2-GUI-Prozesstests ausdrücklich ausgelassen**. Fünf
vollständige reale Protokoll-3-Partien (0/7/8/19/42), Wiederholung von Seed 7
und Terminalwiederholung von Seed 8 prüfen Kartenerhaltung, angewandte legale
Optionen und Ergebnisdateien. Seed 8 enthält weiterhin eine Ablagewahl.
Ziel- und Herkunftsaudit für 58 Dateien sind grün. Der optionale historische
Workspace-Audit ist mit Originalbytes sämtlicher damaliger `engine/*`-Eingaben
aus `f39c1d7` ebenfalls grün; eingefrorene Prüfsummen bleiben unverändert.
Zusätzlich besteht ein vollständiger frischer Release-Build im isolierten
Engine-Klon ohne RL-Dateien/Bilder mit sämtlichen zwölf CTests. Das Binary
meldet den sauberen Implementierungscommit; sein Dateipfad wird aus einem
unabhängigen temporären Arbeitsordner benutzt.
Weitere Abschlussnachweise: [card-data-validation.json](card-data-validation.json),
[CARD_DATA.md](CARD_DATA.md).

Grenzen: Ludus und Decorators behalten ihre vollständigen Zielmetadaten,
ihre neuen Zähl-/Wertungseffekte liefern in 5-I jedoch ausdrücklich null.
Decorators ist bereits im neuen Gildenpool, weshalb seine sieben bedingten
Punkte noch fehlen können; Ludus hat im normalen Drei-Spieler-Deck keine
Kopie. Castrum wendet seine festen drei Schilde an. Status und Vertrag nennen
`tables_complete=true`, `effects_complete=false` und fehlende IDs `[76,78]`.
Die Wissenschaftsoptimierung und alte Gildenkopie-Kandidatenwahl bleiben
unverändert; feste Symbole kommen nun aus der Tabelle. Wunder, GUI/RL,
Legacy-Exporter, Features und Training sind unverändert. Kein Push/Release,
kein Parent-Pin-/Lock-Update; der Parent bleibt bei `a283d58`, sein Pin bei
`f39c1d7`. Der vorbereitete Folgeauftrag bearbeitet ausschließlich Ludus
(5-IIa); Decorators und jede weitere Phase bleiben getrennt. Hier endet 5-I.


## Laufende Gesamtabnahme bis Paket 9

Erweiterte Nutzerfreigabe: alle verbleibenden Pakete nacheinander. Die alten
Stop-Hinweise in historischen Einzelaufträgen sind dadurch aufgehoben.

5-IIa Ludus: umgesetzt; roter Endwertungsfall nachgewiesen, danach alle 13
CTests grün. Eigene/fremde Militärkarten, spätere Karten, Replay und freier
Ablagebau geprüft. Einzelne Commits und weitere Nachweise folgen unten.

5-IIa: Commit `ea0dcf6`. 5-IIb Decorators: eigener roter Fall für spätere
Wunder-Vollendung, danach 13 CTests grün. Die Wertung verwendet die tatsächliche
Stufenzahl (einschließlich vierstufiger Wunder); sieben Punkte nur bei
eigener Vollendung. Alle Karteneffekte jetzt implementiert.

5-IIb: Commit `1032eca`. Paket 6a: Gizah, Rhodos, Ephesos und Alexandria
beider Seiten generiert; rote Gizah-Holzkosten vor der Änderung nachgewiesen.
Alle Stufen dieser acht Seiten gegen die eingefrorenen Ziele und im realen
Entscheidungspfad geprüft. 15 CTests grün. Babylon/Olympia/Halikarnassos
bleiben für die folgenden getrennten Schritte.

6a: Commit `10cd70b`. 6b Babylon: beide Seiten verwenden die Ziele; B hat
exakt zwei Stufen und erhält die siebte Karte bereits nach Stufe eins, auch
beim Bau im sechsten Zug derselben Ära. Joker ab der zweiten Stufe. Roter
Siebtkarte-Fall vor der Korrektur; 15 CTests einschließlich 375 Science-
Beständen und neuem Stufengrenztest grün.

6b: Commit `15be460`. 6c Olympia: neue explizite Freibaumodi für erste Farbe
(Tag) und erste/sechste Karte einer Ära (Nacht). Frühere Farben zählen mit;
Ketten und Freibaumodi bleiben getrennt. Keine alte Einmal-pro-Ära-Regel,
kein neuer Rohstoffrabatt und keine Gildenkopie. Roter neuer Freibaumodus,
danach alle 15 CTests grün; 36 Nacht-Stufen-/Ären-/Rundenkombinationen geprüft.

6c: Commit `d910add`. 6d Halikarnassos: alle neuen Kosten/Stufen übernommen;
Ablagerechte werden aus dem tatsächlichen Stufeneffekt statt Wunder-ID-
Sonderwissen erteilt. Roter Nacht-Stufe-eins-Kostenfall, danach alle 15 CTests
grün. Schlussrunden-Pflichtwahl, Replay, leere Optionen und bisherige atomare
Entnahmefälle bleiben grün. Alle 14 Seiten und ihre Stufen sind jetzt exakt
gegen Targets geprüft. Paket 6 vollständig, Paket 7 folgt.

6d: Commit `63934ef`. Paket 7a: neuer gemeinsamer Standardbibliotheksclient
und entscheidungsbasierte GUI; keine Aktionsableitung aus Wunder-IDs.
Vier neue Sessiontests prüfen sechs vollständige Partien mit 0/2/3 Bots,
Pflicht-Ablagewahlen, ungültige Auswahl vor Dateischreiben, Isolation, Prozess-
ende und Abbruch ohne Resultat. Alte V2-GUI bleibt explizit historisch.
Grafik und echte Tk-Bedienung folgen als nächster Schritt 7b.
