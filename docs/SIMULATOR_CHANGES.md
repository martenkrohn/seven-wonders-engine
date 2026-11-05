# Änderungsprotokoll des C++-Simulators

Stand: 5. November 2026. Diese Datei dokumentiert den gesamten aktuellen Unterschied des Simulatorcodes zur festen Ursprungsfassung, einschließlich Änderungen, die bereits vor der heutigen Self-Play-Arbeit vorhanden waren. Frühere Änderungen werden nicht nachträglich mit einem erfundenen Implementierungsdatum versehen.

## Vergleichsbasis und exakter Nachweis

Ursprung: [dmag-ufsm/7Wonders](https://github.com/dmag-ufsm/7Wonders), Commit [`a0bc333a7a7f21f83156de8a2d522e7baa913f0a`](https://github.com/dmag-ufsm/7Wonders/tree/a0bc333a7a7f21f83156de8a2d522e7baa913f0a). Verglichen wird mit diesem Commit, nicht mit einem veränderlichen GitHub-Branch.

Der vollständige, maschinenlesbare Zeilendifferenz liegt in [simulator/upstream.patch](simulator/upstream.patch). [simulator/manifest.json](simulator/manifest.json) enthält für jede C++-Quelldatei und jeden Header unter `src/`, `include/`, `tests/` sowie `main.cpp` und `CMakeLists.txt` die SHA-256-Prüfsummen und Dateimodi der Ursprungs- und aktuellen Fassung. Auch neue, noch nicht mit Git erfasste C++-Tests sind enthalten. Unveränderte Dateien werden ausdrücklich aufgeführt. Python-Dateien sind eine getrennte Erweiterung und gehören nicht zu diesem Simulatorpatch.

Aus dem Engine-Repository regenerieren und prüfen:

```sh
python3 scripts/audit_simulator.py
python3 scripts/audit_simulator.py --check
```

Regeneration benötigt den bereits lokal vorhandenen Upstream-Commit. Die Prüfung benötigt keinen Netzwerkzugriff und keine alte Git-Historie; sie vergleicht den Arbeitsstand mit dem erzeugten Inventar und prüft die Integrität des Patches. Sie ersetzt keine fachliche Prüfung des Erläuterungstextes. Bei jeder weiteren Simulatoränderung müssen dieser Text, Patch und Inventar gemeinsam aktualisiert werden. Ein automatischer CI-Schritt weist veraltete Inventare zurück. Ein bereits vorhandener Patch allein dokumentiert noch nicht den aktuellen Arbeitsstand.

## ENG-001 – Auswahlproduktion nur einmal pro Kostenberechnung

Bereits vor der Dokumentation vom 19. Oktober 2026 vorhanden; genauer Implementierungszeitpunkt unbekannt. Dateien: `include/player.h`, `src/player.cpp`.

Vorher enthielt `Player` die Flags `raw_extra` und `manuf_extra`. Alexandria-Auswahlproduktion konnte innerhalb einer Ressourcenberechnung mehrfach herangezogen werden. Außerdem wurde zusätzliche Rohstoffproduktion über `raw_extra` neben der kartenspezifischen Auswahl gezählt.

Aktuell werden `used_wonder_choice_raw` und `used_wonder_choice_manuf` im Konstruktor mit `-1` initialisiert und in `ResetUsed()` zurückgesetzt. `ChooseExtraRaw()` beziehungsweise `ChooseExtraManuf()` tragen die erste Auswahl ein und verweigern eine weitere Verwendung bis zum Reset. `BuildStructure()` aktiviert für Forum und Caravansery keine parallelen Extra-Flags mehr; deren Auswahl wird über `AvailableCard()` und die bestehenden `used_*`-Felder behandelt. Die zusätzliche `if (raw_extra) cont++`-Zählung entfällt.

Zweck: Eine einzelne Auswahlquelle kann nicht beliebig viele verschiedene Anforderungen derselben Entscheidung bezahlen. Prüfungen: `tests/test_choice_simple.cpp`, `tests/test_choice_once_per_turn.cpp` (CTest `choice_simple`, `choice_once_per_turn`). Die genaue Abgrenzung zwischen Entscheidungen unterschiedlicher Spieler wird zusätzlich durch ENG-006 korrigiert.

## ENG-002 – Caravansery-Auswahl nicht vom Nachbarn beziehen

Bereits vor dem 19. Oktober vorhanden; genauer Zeitpunkt unbekannt. Datei: `src/player.cpp`, Funktion `IncrementOnDemand()`.

Die Caravansery-Abfragen in den Zweigen für Holz, Lehm und Stein verwenden zusätzlich `!is_neighbor`. Die Einschränkung für Erz sowie die vorhandenen Beschränkungen anderer Sonderproduktion sind im exakten Patch beziehungsweise in der Vergleichsbasis nachvollziehbar; sie werden hier nicht als neue Änderung ausgegeben. Kartenspezifische Produktion bleibt über die bestehenden Auswahlmechanismen verfügbar. Zweck ist die Trennung eigener Sonderproduktion von handelbarer Nachbarproduktion. Dies ist keine vollständige Neufassung des Handelsalgorithmus.

## ENG-003 – Vierte Stufe von Gizah B erreichbar machen

Bereits vor dem 19. Oktober vorhanden; genauer Zeitpunkt unbekannt. Datei: `src/wonder.cpp`, Funktionen `Gizah_b::AddStage()` und `Gizah_b::CanAddStage()`.

Beide behandeln die vierte Baustufe jetzt unter `case 3` statt `case 4`: Nach drei errichteten Stufen ist der aktuelle Stufenzähler drei. Kosten, Erträge und die übrigen Wunderdefinitionen wurden dadurch nicht an eine neue Edition angepasst. Der Patch weist genau die beiden Falländerungen aus. Die heutigen Self-Play-Arbeiten ändern diese Wunderdefinitionen nicht. Die bestehenden CTests sind kein vollständiger Test sämtlicher Wunderstufen.

## ENG-004 – Reproduzierbarer Zufall und kontrolliertes Warten

23. Oktober 2026, bereits vor dem aktuellen Self-Play-Umbau. Datei: `main.cpp`.

`Game` erhält einen lokalen `std::mt19937_64`, den effektiven `engine_seed` und `poll_us`. Der Konstruktor akzeptiert Seed und Warteintervall. `GiveCards()` und die Gildenauswahl benutzen `std::shuffle(..., rng)` statt `random_shuffle`; `GiveWonders()` benutzt eine Gleichverteilung von 0 bis 13 statt `rand() % 14`. Das zeitbasierte `srand()` in der Kartenerstellung entfällt.

`main()` akzeptiert `--seed`, `--poll-us` (0 bis 1.000.000 Mikrosekunden) und `--quiet`; Zahlen werden geprüft, Fehler führen zu einer Fehlermeldung auf stderr und Rückgabewert 1. Ohne vorgegebenen Seed wird ein Seed über `random_device` gebildet. `--quiet` deaktiviert stdout-Ausgaben. Die Warteschleife auf die Spielerdateien schläft standardmäßig 1 ms; mit 0 bleibt ein Vergleich mit aktivem Warten möglich.

Auswirkung: Wiederholungen desselben Seeds sind innerhalb derselben Toolchain reproduzierbar; identische alte Zufallsfolgen werden nicht erhalten. Prüfungen: `python/tests/test_engine_protocol.py`, insbesondere Wiederholung mit verschiedenen Austauschpfaden und Warteintervallen. Messungen zum früheren Runtime-Stand: `docs/archive/performance/RUNTIME_BENCHMARK_2026-10-23.json`.

## ENG-005 – Vollständiger Endzustand und atomarer Dateiaustausch

23. Oktober 2026, bereits vor dem aktuellen Self-Play-Umbau. Dateien: `main.cpp`, `src/filer.cpp`.

`WriteGameStatus()` veröffentlicht zusätzlich `protocol_version=2`, `game.phase`, `game.engine_seed` und `last_actions`. Letzteres dokumentiert je Sitz angeforderten/ausgeführten Befehl, Runde und Status `applied`, `skipped` oder `fallback_discard`. Wenn ein Befehl eine Rundennummer enthält, wird eine falsche Runde zurückgewiesen. Das Überspringen der siebten Karte bleibt erhalten und wird jetzt ausdrücklich quittiert.

Nach der letzten Aktion und den bereits in `NextTurn()` ausgeführten Schlusskämpfen werden zuerst die Gildenkopien aller Spieler ausgeführt. Danach wird ein abschließender JSON-Status mit Runde 21, Ära 4 und Phase `finished` veröffentlicht. Erst anschließend schreibt die Engine ihre Ergebnisdatei. Vorher endete die JSON-Beobachtung vor der abschließenden Auswertung; Gildenkopien erfolgten einzeln während des Schreibens der Ergebnisdatei.

`Filer::WriteMessage()` schreibt zunächst `path + ".tmp"`, prüft den Schreibstatus, schließt die Datei und benennt sie mit `std::rename` um. Fehler werfen eine Ausnahme. Dadurch sieht der Leser entweder den alten oder den vollständig veröffentlichten neuen Status. Die Dateinamen und die grundlegende JSON/Ready-Kommunikation bleiben erhalten; dieser Protokollzusatz verlangt den passenden Python-Wrapper.

Prüfungen: `tests/test_final_status.cpp`, Python-Protokolltests einschließlich Prozessende, Timeout, ungültigem JSON und abschließender Veröffentlichung. `friend struct GameTestAccess` in `Game` erlaubt den C++-Tests kontrollierte Zustände ohne zusätzliche öffentliche Spielschnittstelle.

## ENG-006 – Auswahlbelegung zwischen Spielerentscheidungen zurücksetzen

23. Oktober 2026, während der ersten rein neuronalen Trainingsversuche. Datei: `main.cpp`, Funktion `Game::Loop()`, unmittelbar vor dem Ausführen einer nicht übersprungenen Spielerentscheidung.

Beobachtung: Die neuen Läufe mit Trainings-Seeds 7 und 19 brachen ab, weil die Engine eine zuvor als legal veröffentlichte Aktion durch Abwerfen ersetzte. Ein reproduzierter Fall betraf Engine-Seed `17091312795664974005`, Runde 8: Spieler 0 baute eine Alexandria-B-Stufe und benutzte seine Clay-Pit-Auswahl; Spieler 2 konnte anschließend sein zuvor legales Forum nicht mehr ausführen.

Ursache: Die `used_*`-Markierungen der Auswahlproduktion blieben nach der ersten Entscheidung für die weiteren Spieler desselben globalen Zeitschritts belegt. Die eigene Nutzung einer Produktionsquelle durch ihren Besitzer blockierte dadurch einen anschließenden Kauf derselben Quelle durch einen anderen Spieler.

Korrektur: Vor jeder tatsächlich ausgeführten Spielerentscheidung wird für **alle** Spieler `ResetUsed()` aufgerufen. Innerhalb einer Entscheidung bleiben die Markierungen erhalten; eine Auswahlquelle wird nicht für jede einzelne Ressourcenanforderung neu freigegeben. Der bestehende Reset am Rundenende bleibt bestehen. Keine Änderung an Wunderkosten, Wunderfähigkeiten oder der gewünschten Edition.

Regression: `tests/test_joint_action_resources.cpp` stellt den Konflikt in einer kontrollierten letzten Runde her. Alexandria B benutzt eine Clay Pit zum Wunderbau; ein anderer Spieler benötigt dieselbe handelbare Auswahlquelle für sein Forum. Der Test prüft die anfängliche Legalität, den erfolgten Wunderbau und die Quittierung/Errichtung des Forums. Er ist vor der Korrektur nachweislich mit `fallback_discard` fehlgeschlagen. Der Python-Self-Play-Integrationstest verwendet außerdem Seed 7 und einen ausreichend langen ersten Rollout, der den ursprünglichen Fehler erreicht.

Grenze der Korrektur: Sie löst die nachgewiesene Verschleppung der Auswahlbelegung. Sie macht aus der weiterhin sequenziellen Engine keine vollständig neu implementierte simultane Auflösung und garantiert nicht die vollständige Korrektheit sämtlicher Handels-/Sonderregeln.

## ENG-007 – Build, Tests und Herkunftshinweise

`CMakeLists.txt` behält den bereits in der Vergleichsbasis vorhandenen Sprachstandard C++17 bei, ergänzt die Auswahlproduktionstests, die Endzustandsregression und die Regression für gemeinsame Ressourcenentscheidungen und registriert diese bei CTest. Die beiden älteren assert-basierten Tests behalten bei GCC/Clang ihre Assertions auch im Release-Build durch `-UNDEBUG`. Neue Tests mit `require()` werfen auch im Release-Build bei verletzten Bedingungen.

Die Tests für Endzustand und gemeinsame Ressourcenentscheidungen erhalten getrennte Arbeitsverzeichnisse und eine CTest-Zeitgrenze. Die zusätzlichen C++-Tests sind Downstream-Dateien; der Patch enthält sie vollständig. Herkunftskommentare am Anfang von `main.cpp`, `include/player.h`, `src/player.cpp`, `src/wonder.cpp` und `src/filer.cpp` verweisen auf Ursprung und Dokumentation und verändern kein Spielverhalten. Die gebündelten JSON-Header behalten ihre ursprünglichen Hinweise.

## ENG-008 – Doppelte Arbeit bei Status und unterdrückten Debugausgaben

25. Oktober 2026. Dateien: `main.cpp`, `include/player.h`, `src/player.cpp`.

`Game` speichert den Quiet-Modus jetzt explizit. Die Debugblöcke in `Game::Loop()` werden bei `--quiet` vollständig übersprungen. Zuvor unterdrückte lediglich der Fehlerstatus von `std::cout` die Ausgabe; die Ausdrücke zur erneuten Berechnung von `GetPlayableCards()` und `CanBuildWonder()` wurden trotzdem ausgeführt.

`Player::CalculateScores()` liefert die sechs bisherigen Punktekategorien und deren Summe in einem Durchlauf und aktualisiert weiterhin `victory_points`. `CalculateScore()` delegiert daran. `WriteGameStatus()` verwendet diesen gemeinsamen Datensatz, statt zunächst alle Kategorien und anschließend dieselben Kategorien für die Summe erneut zu berechnen. Regeln, Daten und Protokollschema bleiben gleich.

Prüfung: alle vier CTests, Python-Integrationstests und ein Vergleich mit dem eingefrorenen vorherigen Engine-Binary anhand vollständiger Status-/Spielverlaufs-Fingerprints. Der Vorher-Nachher-Benchmark ist in `python/scripts/benchmark_optimizations.py` wiederholbar; Details und Messgrenzen stehen im Runtime-Ergebnisbericht. Das ersetzt keine vollständige Prüfung sämtlicher Spielregeln.

## ENG-009 – Eigenständiges Engine-Repository

5. November 2026. Die C++-Quelldateien, Header und vier Regressionen wurden
unverändert aus dem RL-Sicherungsstand `pre-engine-split-2026-11-05`
übernommen. Spielregeln, Kartendaten, Wunder und Protokoll bleiben identisch.
CMake verwendet quellordnerbezogene Include-Pfade und legt Laufzeitordner im
Build-Verzeichnis an. Ein optionaler Python-Standardbibliotheksclient prüft
die vollständige Protokollpartie zusätzlich in CTest. Build, CI, Herkunft,
Lizenz und Release-Vertrag sind unabhängig vom RL-Paket nutzbar.

Prüfung: eigenständiger Release-Build, vier C++-Regressionen, Protokoll-Smoke-Test,
Dateiprüfsummen gegen den gesicherten RL-Stand sowie identische vollständige
Spielverläufe mit alter und ausgegliederter Engine unter derselben Toolchain.
Der RL-Consumer verwendet einen festen Submodule-Commit. Die BGA-Migration
beginnt erst in einer nachfolgenden Änderung.

## ENG-010 – Alte GUI optional wieder eingebunden

5. November 2026, `v0.1.1-legacy`. Die alte Tk-Darstellung aus Upstream-`Game.py`
liegt unter `gui/legacy_view.py`; `Game.py` ist der eigenständige Launcher.
`gui/session.py` ergänzt isolierte Laufzeitverzeichnisse, atomare Aktionen,
Rundenprüfung, nicht blockierendes Warten, einfache Begleiter und geordnetes
Prozessende. Der abschließende Status wird angezeigt statt Runde 20 vorzeitig
zu beenden. Fehlende externe Bilder haben beschriftete Platzhalter.

C++-Dateien, Kartendaten, Wunder, Protokollversion und `CMakeLists.txt` sind
unverändert. Der bestehende C++-Patch und das Inventar bleiben deshalb gültig.
Acht zusätzliche GUI-/Clientprüfungen laufen ohne Display; tatsächliche
Tk-Klickpartien prüfen Bedienung und Schlussanzeige mit und ohne lokale Bilder.
Details und Grenzen: [GUI_AUDIT.md](GUI_AUDIT.md). Die weitere Migration wird
über [MIGRATION_PLAN.md](MIGRATION_PLAN.md) einzeln abgearbeitet.

## ENG-011 – BGA-Zieldaten und Zuordnung vorbereitet

5. November 2026, Paket 1. `data/bga-second-edition/` enthält die geprüften
80 Kartenzeilen und 14 Wunderseiten, statische BGA-JSON-Eingaben mit ihren
Prüfsummen, Quellenentscheidungen und vollständige Zuordnungen. Neue IDs:
Ludus 76, Castrum 77, Decorators Guild 78; Strategists 73 bleibt reserviert.
Der Vergleich hatte Decorators an Position 73 gegenübergestellt; diese
historische Position wird ausdrücklich von der neuen Kartenidentität getrennt.

Ein eigenständiger Standardbibliotheksaudit und elf Regressionen prüfen
Abdeckung, Bestands-IDs, Altersvarianten, Kosten, Effekte, Wunderstufen,
Sprite-Zuordnungen, 25 Ketten und Deckmengen; CI führt beide aus. Der optionale
Workspace-Abgleich prüft zusätzlich alle Zielwerte gegen den vorhandenen
Vergleich. Einzelheiten: [Zielvertrag](../data/bga-second-edition/README.md).

Kein C++- oder Buildstand, keine GUI und keine RL-Daten wurden verändert.
Patch/Inventar bleiben gültig. Der ausführbare Regelstand bleibt
`legacy-dmag-with-downstream-fixes`, Releasevertrag `v0.1.1-legacy`, Protokoll 2.
Nur die inaktive Zieldatei trägt `bga-second-edition-development` mit
`second_edition_implemented=false`. Generatorintegration und Regelmigration
folgen erst in den dafür vorgesehenen Paketen.

## ENG-012 – Optimale Wissenschaftsjoker im Entwicklungs-Regelstand

5. November 2026, Paket 3, `v0.2.0-dev`,
`ruleset=bga-second-edition-development`, `second_edition_implemented=false`.
Datei: `src/player.cpp`, `Player::CalculateScientificScore()`.

Vorher konnte die gierige Vergabe Scientists Guild und Babylon unterbewerten:
Workshop, Laboratory, Apothecary plus Scientists Guild erhielten 10 statt
13 Punkte. Der neue CTest `science_scoring` weist diesen Fehler vor der
Korrektur ausdrücklich nach. Die Wertung zählt feste Symbole und bisherige
Jokerquellen unverändert und maximiert anschließend über alle vollständigen
Verteilungen der Joker auf drei Symbolarten. Sie verändert weder Karten noch
Ressourcen oder Wunderstufen. Auswertung O(J²), zusätzlicher Speicher O(1).

`tests/test_science_scoring.cpp` prüft 375 Symbol-/Jokerbestände gegen einen
unabhängigen rekursiven Prüfer, alte Babylon-Stufengrenzen, eigene und kopierte
Gilden, wiederholte Wertung und eine echte Schlussrundenveröffentlichung mit
übereinstimmenden Kategorien, Gesamtpunkten und `results.txt`. Der Test wird
über `CMakeLists.txt` in einem isolierten Laufzeitordner registriert. Alle
sechs CTests, die 19 Python-Tests und der BGA-Zielaudit sind grün.
Patch und Inventar werden für den geänderten C++-/Test-/Buildstand erneuert.

Grenzen: keine neuen Karten-/Wunderdaten oder Fähigkeiten, kein Protokoll-
oder GUI-Umbau. Die getrennte alte `CopyGuild()`-Kandidatenbewertung bleibt
näherungsweise; eine tatsächlich kopierte Gilde wird optimal gewertet.
Python-Diagnose und feste Symbolformel stimmen mit der Veröffentlichung
überein; die heuristische Marginalvorschau ohne Joker bleibt für Paket 8
abgegrenzt. Keine RL-Übernahme, Pin-/Lock-Änderung, Veröffentlichung oder
Trainingsänderung. Details: [SCIENCE_SCORING.md](SCIENCE_SCORING.md).

## ENG-013 – Sichere interne Ablageentnahme

5. November 2026, Paket 4b-I, unverändert `v0.2.0-dev`,
`ruleset=bga-second-edition-development`, `second_edition_implemented=false`,
Transportversion 2. Dateien: `include/card.h`, `src/card.cpp`,
`include/player.h`, `src/player.cpp`, `main.cpp`, `CMakeLists.txt` und
`tests/test_discard_safety.cpp`.

Drei getrennte rote C++-Fälle weisen die geerbten Fehler nach: erfolgreiche
Konstruktion ohne zentrale Ablageentnahme, Löschen einer fremden Handposition
über den Ablageindex und Rechtverbrauch bereits bei ungültiger Auswahl.
Physische Deckkopien erhalten jetzt eine nichtnullige, parteilokale
Instanzkennung. `Card::Equal()` bleibt der logische Typvergleich. Mischen,
Austeilen, Weiterreichen und Ortswechsel erhalten die physische Kennung.
Namensbasierte V2-Handaktionen werden an die tatsächliche Hand gebunden,
nicht an eine möglicherweise gleichnamige Karte eines anderen Zeitalters.
Eine fehlende Namenssuche liefert eine sicher initialisierte leere Karte.

`ApplyBuilding()` trennt die unveränderte Gebäudeanwendung von der
Quellentnahme. `BuildDiscardFree()` nimmt den zentralen Ablagevektor per
Referenz und bindet genau eine physische Instanz. Ein vorbereiteter Spieler-
und Ablagezustand übernimmt Gebäudeeffekt, Stadtzugang, einmalige Entnahme
und Rechtverbrauch gemeinsam über nicht werfende Swaps. Fehler davor lassen
den Originalzustand unverändert. Kein Ablageindex wird in der Hand verwendet.
Vorhandene Halikarnassos-Stufen erteilen ein Recht höchstens einmal je
Quellstufe; null legale Optionen schließen es automatisch ohne Ersatz.

V2 `extra` wird pro Sitz gespeichert und auf eine eindeutige Namensvorwahl
in der tatsächlichen Ablage begrenzt. Eine unerfüllte Vorwahl baut nichts;
das alte Zugfenster endet ohne Verschleppung des Rechts. Die vollständige
sichtbare Pflichtauswahl und die simultane Auflösung bleiben 4b-II. Die
Nutzervorgaben zu RQ-1/RQ-2 sind im Entwurf übernommen: kein nachträglicher
regulärer Münzertrag durch späteren Ablagebau, kein freiwilliger Verzicht
bei legaler Option. V2 erzwingt diese Wahlpflicht noch nicht.

Prüfungen: `discard_safety` mit physischer Kopienwahl, Altersduplikaten,
Handisolation, falscher/verbrauchter Berechtigung, ungültigem und anschließend
gültigem Versuch, realen Halikarnassos-Stufen, leerer Auswahl, Sitzbindung im
echten Schlussrunden-Dateipfad, mehrdeutigen Namen und 63 erzwungenen
Allokationsfehlern vor dem Commit. Alle sieben CTests und die 19 bestehenden
Python-/Sessiontests bestehen. Eigenständiger Zielaudit grün; historischer
Workspace-Abgleich benötigt seine eingefrorenen C++-Quelldateien, deren
Prüfsummen nicht überschrieben werden. Patch/Inventar sind aktualisiert.
Details und verbleibende Grenzen: [DISCARD_SAFETY.md](DISCARD_SAFETY.md).

## ENG-014 – Simultane Auflösung und Protokoll 3

5. November 2026, Paket 4b-II, weiter `v0.2.0-dev`,
`ruleset=bga-second-edition-development`, `second_edition_implemented=false`.
Der aktive Transport ist jetzt **3**, Vertrag `decision-v1`; Version 2 wird
nicht parallel interpretiert. Betroffen: neuer `DecisionEngine`-Kern,
Player-Produktionsprobe und verzögerte Sofortmünzen, kopierbare Wunderzustände,
Game-Schleife/Dateitransport, Buildkonfiguration und gezielte Regressionen.

Vier rote Fälle gegen `da93178` belegen neue Produktion und Einnahmen während
derselben sequenziellen Runde, verpasste simultane Vineyard-Zähler sowie
vorzeitigen Abschluss trotz legaler Halikarnassos-Pflichtwahl. Der neue Kern
plant alle regulären Optionen gegen einen unveränderlichen Anfangszustand.
Produktion wird mit unabhängigen Käuferbelegungen und minimalen festen
Zahlungsplänen ermittelt; Karten- und Startquellen bleiben nachvollziehbar.
Wunderkosten werden aus dem bestehenden `CanAddStage()`-Pfad erfasst, ohne
Kosten-/Stufentabellen umzuschreiben. Nach Quote wird dieselbe alte Stufe
mit bereits abgerechneten Zahlungen angewandt.

Spieler, Wunder, Kartenorte und Zufall werden tief in eine isolierte Runde
kopiert. Erst nach allen regulären Annahmen werden die Konstruktionen und
Zahlungen gemeinsam übernommen. Sofortmünzen werden nach dem gemeinsamen
Bauzustand gezählt; Lighthouse zählt sich selbst. Neue Münzen/Produktion
finanzieren keine andere Aktion dieses Zugs. Eine nachfolgende Ablagewahl
erhöht keine vorherige reguläre Münzauszahlung rückwirkend (RQ-1/USER).

Quellgrants, aktuelle Verkäufe und Restkarten werden vor der sichtbaren
Ablagewahl aufgelöst. Bei legalen Karten ist die Auswahl Pflicht, andernfalls
automatische Erledigung ohne Ersatz (RQ-2/USER). Der sichere Instanzbau aus
ENG-013 bleibt die Entnahmeoperation. Weitergabe, neuer Babylon-Zug, Militär
und Schlusswertung warten auf alle Folgeentscheidungen; unbeteiligte Sitze
erhalten keine siebten Fake-Aktionen. Die bisherigen Wunderfähigkeiten und
ihre Aktivierungsstufen bleiben erhalten, einschließlich Legacy-Olympia.

Neue Dateibundles und Ready-JSON verwenden Partie-/Bundle-/Request-/
Entscheidungskennungen und angebotene Revisionen. Nach Annahme bleibt ein
Request gesperrt; Wiederholung liefert seinen aktuellsten Receipt zurück.
Konfliktantworten überschreiben ihn nicht und werden getrennt nachvollzogen.
Falsche/veraltete Eingaben führen zu keinem Ersatzabwurf. Optionale Fristen,
expliziter Abbruch und interner Fehler erzeugen `aborted` ohne Siegerresultat.
Nur normaler Abschluss schreibt `results.txt` einmal atomar; während einer
kurzen konfigurierbaren Abschlussfrist werden Wiederzustellungen beantwortet.

Der neue Standardbibliotheksclient und sechs Prozessregressionen prüfen den
Dateipfad, komplette Partien einschließlich Halikarnassos, Babylon und
Terminalwiederholung. `decision_resolution` prüft unabhängig erwartete
Ökonomie, Freibaumodi, Quellen-/Zahlungspläne, Pflichtwahl, Queue, Fristen,
Grenzfälle der letzten regulären/siebten Karte und Rückrollen nach einem
bewusst während des Stufenbaus ausgelösten Fehler. Vorhandene Schluss-,
Ressourcen-, Wissenschafts- und Ablagesicherheitstests verwenden den neuen
Kern; die zwei älteren direkten Produktionstests bleiben erhalten.

Keine Kartenwerte oder Wunderstufendefinitionen, keine Wissenschafts- oder
Gildenkopie-Kandidatenwahl, keine GUI-/RL-Laufzeitdateien, Exporte, Features
oder Trainingsstände geändert. Die alte GUI lehnt Version 3 ab; ihre fünf
V2-Prozessprüfungen sind als historische Tests ausgelassen, die Versionsgrenze
ist separat getestet. Keine RL-Übernahme, Pin-/Lock-Änderung oder Veröffentlichung.
Aktiver Vertrag und Herkunftspatch/Inventar sind aktualisiert. Details:
[SIMULTANEOUS_RESOLUTION.md](SIMULTANEOUS_RESOLUTION.md),
[PROTOCOL.md](PROTOCOL.md), [resolution-validation.json](resolution-validation.json).

## ENG-015 – Erzeugte BGA-Kartentabellen im Entwicklungsstand

5. November 2026, Paket 5-I, `v0.2.0-dev`, Protokoll 3 und
`second_edition_implemented=false`. `targets.json` speist einen deterministischen
Standardbibliotheksgenerator, einen einkompilierten C++17-Header und den
sprachneutralen Export `data/generated/cards-v1.json` (Format 1, Quellenhash).
Keine Runtime-JSON-, RL- oder Bildabhängigkeit; `--check` ist schreibfrei.

Fünf gezielte rote Laufzeitfälle gegen `6840e94` belegen Altar-Punkte,
Aqueduct-Kosten, Pantheon-Kette, Town-Hall-Mengen und den alten Strategists-Pool.
Alle 80 Zeitalterzeilen/77 Typen und 25 Ketten sind übernommen. Bestehende IDs
bleiben stabil, 73 reserviert; Ludus 76, Castrum 77 und Decorators 78 sind
angehängt. Graue Altersvarianten bleiben getrennt; mehrdeutige Namenssuche
verlangt ein Alter. Pro Ära 21 Karten, fünf aus zehn Gilden bei drei Spielern.

`Game::CreateDecks`, `Card::FromDefinition`, `Player::ApplyBuilding`, die
festen Wissenschaftssymbole und Punkte-/Münzfaktoren sowie
`DecisionCore::Sources` verwenden die erzeugten Werte. Damit teilen Planung
und Anwendung Produktionsmengen, Wahlalternativen und Handelbarkeit. Die
bisherigen Wissenschaftsoptimierungs- und Gildenkopie-Kandidatenalgorithmen
bleiben erhalten. Der alte synthetische Card-Testkonstruktor bleibt verfügbar;
reale Deckkarten verwenden die strikte ID-/Altersfabrik.

Ludus-/Decorators-Zielmetadaten sind vollständig enthalten, ihre neuen
Algorithmen jedoch ausdrücklich für 5-II gesperrt. Status und Vertrag nennen
`tables_complete=true`, `effects_complete=false`, fehlende IDs 76/78; Decorators
ist im Deckpool, wertet derzeit auch mit vollendetem Wunder null. Castrum
wendet seine festen drei Schilde an. Kein vollständiger Kartenregelstand.
Wunderdefinitionen, GUI/RL und Parent-Pin/Lock bleiben unverändert.

Zwölf CTests und 26 Python-Prüfungen bestehen; fünf historische V2-GUI-
Prozessprüfungen bleiben abgegrenzt. Der neue C++-Test baut alle Zeilen im
echten Entscheidungsweg, prüft alle Ketten, exakte/unzureichende Kosten,
Produktion und die Grenze zu 5-II. Python vergleicht alle kompilierten Felder
mit den Zielen und 40 reale Deckziehungen. Generator-Doppelgenerierung,
Schreibfreiheit und isolierte Pfade sind geprüft; die vorhandenen
Auflösungs-/Wissenschafts-/Ablagetests bleiben grün. Fünf vollständige reale
Partien einschließlich Pflicht-Ablagewahl und Terminalwiederholung sind
protokolliert. Ziel- und Herkunftsaudit grün; 58 C++-/Build-/Testdateien im
Inventar. Historischer Workspace-Audit nutzt unveränderte Originalbytes aus
dem Legacy-Commit, keine aktualisierten Herkunftshashes. Details:
[CARD_DATA.md](CARD_DATA.md), [card-data-validation.json](card-data-validation.json).

## ENG-016 – Ludus (Paket 5-IIa)

Ludus aktiviert die generierten eigenen Militärzähler: drei Münzen je roter
Karte beim Bau, ein Endpunkt je eigener roter Karte. Regulärer Bau und freier
Ablagebau zahlen genau einmal; spätere rote Karten verändern nur die Wertung.
Der neue Test zeigte vor der Aktivierung fehlende spätere Endpunkte. Null bis
drei eigene und fremde rote Karten, Replay und Ablageentnahme sind geprüft.
Alle 13 CTests bestehen. Decorators bleibt hier ausdrücklich offen. CMake
kompiliert den gemeinsamen Kern einmal als Bibliothek für alle Programme.

## ENG-017 – Decorators Guild (Paket 5-IIb)

Sieben Punkte genau bei vollständigem eigenem Wunder, auch wenn die letzte
Stufe erst nach dem Gildenbau entsteht. `Wonder::IsComplete` verwendet die
jeweilige Stufenzahl. Teilstände, drei-/vierstufige Wunder und wiederholte
Wertung geprüft; vor der Korrektur schlug spätere Vollendung fehl. 13 CTests
grün, keine Karteneffekte mehr gesperrt; Wunderumstellung folgt separat.

## ENG-018 – Einfache Wunderfamilien (Paket 6a)

Gizah, Rhodos, Ephesos und Alexandria beider Seiten verwenden dieselbe
Generatorquelle wie Karten. Neue Tabellen/Export enthalten die 14 Ziele;
nur diese acht Seiten sind in diesem Schritt aktiviert. Ein gemeinsamer
Stufenpfad prüft aktuelle Kosten ohne veraltete Restkosten, klont die
Prüfökonomie und wendet Punkte/Münzen/Schilde einmalig an. Alexandria-
Wahlproduktion bleibt einmalig und nicht handelbar. Alle Stufen werden
gegen die eingefrorenen Ziele und über echte Angebote/Auflösung geprüft.
Roter Gizah-Kostenfall, danach 15 CTests grün. Die alte Alexandria-B-
Produktionsfixture erhält das jetzt erforderliche Erz statt alter Holzannahme.

## ENG-019 – Babylon (Paket 6b)

Beide Seiten auf erzeugte Ziele umgestellt. Babylon B: zwei Stufen,
siebte Karte ab erster Stufe, Joker ab zweiter; Aktivierung noch im sechsten
Zug erhält den eigenen siebten Zug vor der Ärenbarriere. Wissenschaft liest
die tatsächlichen aufgebauten Stufeneffekte, die Optimierung bleibt gleich.
Roter Aktivierungsfall nachgewiesen, anschließend alle 15 CTests grün.

## ENG-020 – Olympia (Paket 6c)

Erste Farbe nach Tag-Stufe zwei, unter Berücksichtigung schon vorher gebauter
Farben; erste beziehungsweise sechste Ärenkarte nach Nacht-Stufe eins/zwei.
Optionen benennen den Freibaumodus ausdrücklich; Zahlungen und Ketten bleiben
getrennt, Erwerb ist erst in späteren Angeboten sichtbar. Nacht gewährt keinen
alten Rabatt oder Gildenkopie; die geerbte Kopierfunktion ist für migrierte
Wunder deaktiviert. Historische Kopiertests prüfen jetzt die entfernte
Fähigkeit; optimale eigene/Babylon-Joker und Schlusskategorien bleiben geprüft.
Roter neuer Modus vor Implementierung; anschließend 15 CTests grün.

## Abgrenzung

Es findet keine pauschale Behauptung vollständiger Regelkorrektheit statt. Insbesondere werden die Wunder-Ausbauschritte nicht zur neuen Edition migriert. Die Kartentabellenmigration in ENG-015 bleibt von den noch offenen neuen Sondereffekten getrennt. Falls frühere Kartenänderungen nicht im exakten Unterschied zu diesem Upstream-Commit erscheinen, werden sie nicht ohne Nachweis als Änderungen dieser C++-Fassung behauptet. Für jede zukünftige Änderung ist ein neuer Eintrag mit Anlass, genauer Funktion, Wirkung, Test und Grenzen anzulegen und der exakte Nachweis neu zu erzeugen.
