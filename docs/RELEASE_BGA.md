# BGA-Grundspiel: gemeinsame Abnahme

Stand: 6. November 2026. Releasevertrag **v0.2.0-bga**, Regeln
**bga-second-edition**, **Protokoll 3 / decision-v1**, genau drei Spieler,
`second_edition_implemented=true`. Die lokale Veröffentlichungsvorbereitung
ist von der noch ausstehenden Remote-Freigabe getrennt.

| Bereich | Nachweis |
| --- | --- |
| Referenz | 80 Karten-/Zeitalterzeilen, 77 Typen, 25 Ketten, 14 Wunderseiten; unabhängiger Target-Audit und Vergleich der kompilierten Definitionen |
| Regeln | 15 CTests, darunter optimale Science-Joker, Simultaneität, minimale Zahlungen, Kartenidentität, Ablagesicherheit und Abschlussbarrieren |
| Neue Effekte | Ludus, Decorators-Vollendung, alle Wunderstufen, Babylon-Siebtkarte, Olympia-Freimodi, Pflichtablage |
| Handelsrichtung | Roter Gegenbeweis vor Korrektur; zwölf Sitz/Karte/Verkäufer-Kombinationen im Angebot, Settlement und direkten Kauf |
| GUI | 17 Prüfungen: zwölf aktiv bestanden, fünf historische V2-Prozessfälle ausgelassen; vier echte Tk-Klickpartien 8/19/6/9, Bilder und Fallback, verdeckte Handübergabe |
| Headless | 60 vollständige Partien, 3277 bestätigte Entscheidungen und 13 Pflichtablagewahlen; Kartenerhaltung, Enddatei, Exit und Seed-/Terminal-Replay |
| RL | Paket 8 im Consumer-Commit a91ade5: 238 bestanden, ein Node-Test lokal ausgelassen; dieser JS-Harness anschließend im V8-Isolat bestanden. Acht neue BGA-Tests nach stabiler Umstellung erneut grün |
| Paket/Training | Wheel enthält 80/14 Ressourcen und lädt außerhalb des Repositorys; frischer PPO, Gewichtsänderung, Reload/Resume, Reproduzierbarkeit und Ablehnung alter Checkpoints geprüft |

Der endgültige Consumer-Nachweis `docs/reference/bga-release-validation.json`
im RL-Repository hält den genauen Engine-Pin, den sauberen Binary-Stand,
Builds und das abschließende Training fest. Historische Nachweise bleiben
unverändert; ihre damaligen offenen Punkte beschreiben ihren damaligen Commit.

## Reproduzieren

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel 2
ctest --test-dir build --output-on-failure
python3 scripts/audit_simulator.py --check
python3 scripts/audit_bga_targets.py --check
python3 scripts/generate_card_data.py --check
python3 scripts/accept_bga_release.py --games 60 --output /tmp/bga-games.json
xvfb-run -a python3 -m unittest discover -s tests -p 'test_gui*.py' -v
```

Die neutralen Exporte und das native Release-Header verwenden denselben
operativen Vertrag. Die eingefrorenen Targets, Rohreferenzen und Provenance
sind bytegleich erhalten. Nach lokalem Commit muss CMake neu konfigurieren,
damit das Binary den endgültigen Commit und einen sauberen Quellstand meldet.

## Grenzen und Veröffentlichung

Unterstützt ist ausschließlich das Grundspiel mit drei Spielern. Kein Vergleich
mit BGA-Servercode, keine Erweiterungen, kein Nachweis gelernter Spielstärke.
RQ-1/RQ-2 sind die expliziten Nutzerentscheidungen nein/nein. Bilder bleiben
optional und extern. Einzelheiten stehen in KNOWN_ISSUES.md.

Vorgesehener Tag in beiden Repositories: `v0.2.0-bga`. Die automatische
Freigabeprüfung hat den Remote-Dry-run abgelehnt, weil das konkrete Ziel nicht
freigegeben und die Gesamtabnahme damals noch nicht grün war. Erst nach
fertiger lokaler Prüfung und ausdrücklicher Zielfreigabe veröffentlichen:
Engine `martenkrohn/seven-wonders-engine`, Branch `main`, dann Consumer
`martenkrohn/seven-wonders-rl`, Branch `refactor/separate-engine-20261105`.
Der lokale Consumer-Pin ist kein Nachweis bereits erfolgter Veröffentlichung.
