# Wissenschaftswertung – Paket 3

Stand: 5. November 2026. Der Entwicklungsstand `v0.2.0-dev` mit Regelnamen
`bga-second-edition-development` maximiert die Wissenschaftspunkte über alle
Jokerverteilungen. `second_edition_implemented=false` und Transportversion 2
bleiben bestehen. Karten, Wunder, GUI und RL-Daten wurden nicht migriert;
`v0.1.1-legacy` und der gespeicherte RL-Pin bleiben unverändert.

## Fehlerfall und Korrektur

Workshop und Laboratory liefern zwei Zahnräder, Apothecary einen Zirkel,
Scientists Guild einen Joker. Die alte Heuristik ergänzt das häufigste Symbol:
3/1/0 ergibt 10 Punkte. Ein Tafelsymbol ergibt dagegen 2/1/1 und
`4 + 1 + 1 + 7 = 13` Punkte. Der neue CTest `science_scoring` wurde zunächst
gegen den unveränderten Algorithmus ausgeführt und scheiterte mit
`expected 13, got 10`.

`Player::CalculateScientificScore()` zählt feste Symbole und Joker wie bisher
aus den gebauten Karten und den alten Babylon-Stufenmarkern. Anschließend
werden für `J` Joker alle nichtnegativen Anteile für Zahnräder und Zirkel
durchlaufen; der Rest wird zu Tafeln. Für jede Verteilung wird
`g² + c² + t² + 7 * min(g,c,t)` bewertet und das Maximum zurückgegeben.
Damit wird jeder Joker genau einmal zugeordnet, ohne feste Symbole oder
Spielzustand zu ändern. Die Laufzeit beträgt O(J²), zusätzlicher Speicher O(1).

Es zählen weiterhin ein Joker pro gebauter oder tatsächlich kopierter
Scientists Guild sowie der alte Babylon-Joker (A ab Stufe 2, B ab Stufe 3).
Gildenkopien stehen bereits als eigene Karteneinträge im Spielzustand; ein
eigener zusätzlicher Kopierbonus würde denselben Joker doppelt zählen.
Die Quellen-/Stufenregeln selbst bleiben unverändert und werden durch diese
Korrektur nicht vorzeitig auf die BGA-Stufen migriert.

## Abdeckung

`tests/test_science_scoring.cpp` verwendet Karten und echte Spielerschnittstellen;
eine neue öffentliche Engine-Schnittstelle ist dafür nicht nötig. Ein unabhängiger rekursiver
Prüfer ordnet jeden Joker einzeln zu und vergleicht 375 Bestände:
0–4 feste Symbole jeder Art und 0–2 Joker. Darunter sind leere Bestände,
fehlende Symbole, gleiche Optima und mehrere Joker aus Scientists Guild plus
Babylon. Jeder Bestand wird mehrfach über Einzel-, Kategorie- und Gesamtwertung
geprüft; Ressourcen, Kartenanzahl, Hand und Wunderstufe bleiben gleich.

Zusätzlich werden alle alten Babylon-A-/B-Stufengrenzen 0–3 geprüft. Eine
synthetische Bestandsprüfung mit eigener plus kopierter Scientists Guild
sichert das getrennte Zählen von Karteneinträgen ab; sie behauptet keine
zusätzliche Gildenkopie im regulären Deck. Zwei Joker bei 2/1/0 Symbolen
ergeben optimal 18 Punkte. Die kontrollierte Schlussrundenprüfung veröffentlicht
18 Punkte für Babylon plus eigene Gilde und nach der tatsächlichen
Gildenkopie 13 Punkte für Olympia. Kategorien, Gesamtpunkte und `results.txt`
müssen übereinstimmen; eine wiederholte Schlussveröffentlichung bleibt gleich.

Reproduzierbar aus dem Engine-Verzeichnis:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 scripts/audit_simulator.py --check
```

## Getrennte verbleibende Befunde

Die alte `CopyGuild()`-Auswahl bewertet die Scientists Guild weiterhin mit
`AmountOfType(scientific)` statt mit dem tatsächlichen Wissenschaftszuwachs.
Beispielsweise kann Olympia B ohne feste Wissenschaftssymbole die einzige
benachbarte Scientists Guild nicht kopieren, obwohl ihr Joker einen Punkt
wert wäre. Die Korrektur wertet eine tatsächlich kopierte Gilde optimal;
sie repariert nicht die gesamte alte Wunder-/Gildenkopierentscheidung.
Insbesondere wird `CopyGuild()` durch das wiederholte Berechnen von Punkten
nicht aufgerufen; seine eigene Wiederholbarkeit ist ein getrennter Altbefund.

Die Python-Diagnose `ScienceDiagnostics.finish()` übernimmt die veröffentlichten
Engine-Wissenschaftspunkte. `science_state()` führt feste Symbole und Joker
getrennt; `science_score(g,c,t)` berechnet die richtige Formel für bereits
zugeordnete Symbole. Diese Pfade brauchen für die Engine-Korrektur keinen Umbau.
Die heuristische `science_marginal_vp()` berücksichtigt jedoch keine vorhandenen
Joker. Bei 2/1/0 plus einem Joker beträgt der optimale Zuwachs eines weiteren
Tafelsymbols 5 (18 minus 13); die Formel nur für feste Symbole liefert 8
(13 minus 5). Dieser vorhandene Vorschau-/Policy-Vertrag wird hier ausdrücklich
abgegrenzt und gehört vor einer Übernahme zur RL-Anpassung in Paket 8.

Halikarnassos, simultane Auflösung, neue Karten und Wunder, vollständige
BGA-Regeln und die bewusste RL-Übernahme bleiben offen. Die Quelldaten und
Modelle des RL-Projekts werden in diesem Paket nicht verändert.
