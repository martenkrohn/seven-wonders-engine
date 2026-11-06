# Simultane Auflösung und Pflichtentscheidungen – Paket 4b-II

Aktueller Stand vom 6. November 2026: Karten, Wunder, GUI und RL sind migriert.
Die aktuelle Abnahme und ihre Grenzen stehen in [RELEASE_BGA.md](RELEASE_BGA.md).
Die folgenden Paketnachweise sind historisch und behalten ihre damaligen
Regelnamen, Beispiele und offenen Folgearbeiten.


Stand: 5. November 2026. Implementiert: `decision-v1`, Transportversion 3.
Release/Regeln bleiben `v0.2.0-dev`, `bga-second-edition-development`,
`second_edition_implemented=false`. Dies ist die fachliche Grundlage für
spätere Kartendaten-, Wunder-, GUI- und RL-Pakete, keine fertige neue Edition.

## Nachgewiesener Ausgangsfehler

Vor der Korrektur scheiterten vier gezielte, getrennte Versuche gegen
Engine-Commit `da93178`:

| Fall | Nachweis im alten sequenziellen Pfad |
| --- | --- |
| Neue Produktion | Ein später bearbeiteter Sitz konnte Holz von einer gerade erst gebauten Lumber Yard kaufen. |
| Neue Einnahmen | Ein Verkäufer konnte die gerade erhaltenen zwei Handelsmünzen für seine reguläre Aktion verwenden. |
| Gleichzeitige Auszählung | Ein vorher gebauter Vineyard erhielt keine Münze für die anschließend errichtete Nachbar-Lumber-Yard. |
| Schlussbarriere | Eine letzte Halikarnassos-Stufe mit legaler Ablagekarte endete ohne Pflichtauswahl in `finished`. |

Die roten Versuche verwendeten die damaligen Player-/Game-Aufrufe mit echten
Nachbarn und eindeutigen Beständen; die drei Ökonomiefälle wurden unabhängig
vom Protokoll geprüft. Der neue CTest `decision_resolution` prüft die erwarteten
Regeln über Angebote, Requests und veröffentlichte Zustände des neuen Kerns.

## Ein gemeinsamer Anfang und ein gemeinsamer Commit

`DecisionEngine` besitzt einen tief kopierten Zustand mit drei Spielern,
polymorph kopierten Wundern, Kartenorten und Zufallszustand. Spieler- und
Nachbarzeiger werden auf die jeweilige Kopie gebunden. Reguläre Annahmen
ändern nur das Request-Ledger. Erst wenn alle erforderlichen Sitze angenommen
wurden, wird auf einer isolierten Kopie die ganze Auflösung vorbereitet und
über einen nicht werfenden Besitzwechsel übernommen. Ein interner Fehler
behält den vorherigen konsistenten Zustand und bricht kontrolliert ab.

Die Produktions-/Zahlungsplanung liest ausschließlich den Rundenanfang. Eine
Memoisierung über Quelle und verbleibende sieben Ressourcen findet einen
vollständigen, kanonischen Plan. Eigene Auswahlquellen werden je Aktion einmal
belegt; Nachbarquellen werden je Käufer unabhängig geplant. Der Verkäufer
behält seine eigene Produktion. Handelbar sind Startressource und braune/graue
Karten, keine gelben oder Wunder-Auswahlquellen. Quellen mit mehreren festen
Einheiten dürfen teilweise gekauft werden. Ausgabe und Empfängersitze stehen
im Angebot; Clients können sie nicht überschreiben.

Wunderkosten werden aus dem bestehenden `CanAddStage()`-Pfad auf einer
isolierten, produktionslosen Probe erfasst. Dies liest nur die tatsächlich
für diese Stufe geprüften Ressourcen und vermeidet die geerbten alten
Kosteneinträge im nächsten Stufenobjekt. Nach bestätigtem Zahlungsplan führt
`AddStage()` dieselbe alte Stufe ohne erneute sequenzielle Einkaufsentscheidung
aus. Ihre Kosten, Erträge und Aktivierungsgrenzen sind nicht migriert.

Gemeinsam werden Karten verschoben, Ausgaben abgezogen und Empfängermünzen
addiert. Erst nach allen regulären Konstruktionen werden Sofortmünzen aus
dem gemeinsamen Bauzustand gezählt. Lighthouse zählt dabei sich selbst.
Spätere Ablagebauten ändern diese bereits bestimmten Zähler nicht rückwirkend
(RQ-1, ausdrücklich vom Nutzer entschieden). Ein aus der Ablage gebautes
Gebäude erhält seinen eigenen unmittelbaren Effekt genau einmal.

## Folgeentscheidungen und Ärenbarrieren

Nur ein tatsächlich erfolgreicher Quell-Stufenbau erzeugt einen Grant mit
Sitz, Stufe und Ereignis. Nach den regulären Konstruktionen und fälligen
Restkarten wird die Warteschlange geöffnet. Der aktive Sitz erhält aktuelle
Ablageinstanzen. Eigene Gebäudeduplikate bleiben sichtbar, aber unselektierbar;
bei mindestens einer legalen Option muss gewählt werden (RQ-2: kein Verzicht).
Ohne legale Option erfolgt automatische Erledigung ohne Ersatzmünzen.
`BuildDiscardFree()` aus 4b-I übernimmt die sichere einmalige Entnahme.

Im Grundspiel gibt es eine Halikarnassos-Familie. Synthetische Tests mit mehreren
Familienkopien prüfen die technische Reihenfolge nach Sitz und die erneute
Berechnung nach einer Entnahme. Das behauptet keine Erweiterungsregeln und
schaltet keine zusätzlichen Quellen frei.

Nach dem sechsten regulären Zug werden Restkarten ohne Recht abgelegt;
Babylon behält seine Restkarte, auch wenn das bisherige Recht erst durch die
aktuelle Stufe entsteht. Die Ablagewahl dieses Zugs muss zuerst schließen.
Dann entsteht gegebenenfalls ein eigener siebter Zug mit frischem Anfang,
ausschließlich für berechtigte Sitze. Neue Einnahmen und Produktion des
sechsten Zugs sind jetzt verfügbar. Andere Sitze erhalten keine Fake-Aktion.
Ohne berechtigten Sitz wird der siebte Slot übersprungen.

Handweitergabe wartet auf alle Folgeentscheidungen. Konflikte laufen nach
regulärem Schlusszug, Restkarten, siebter Karte und letzter Ablagewahl genau
einmal. Nach Ära III folgen vorhandene Gildenkopien und Endwertung; erst dann
normaler Endstatus und Ergebnisdatei. Kopierte Gilden werden im Protokoll als
Effektreferenzen und nicht als weitere physische Karten dargestellt. Die
bisherige Kandidatenwahl und Wissenschaftswertung bleiben erhalten.

## Protokoll und Prüfungen

[PROTOCOL.md](PROTOCOL.md) beschreibt Status, Bundle, Eingaben, Quittierungen,
Fristen und Abbruch. Angebote und Eingaben sind durch Partie, Entscheidung,
angebotene Revision, Request und Karteninstanz gebunden. Die angebotene Revision
bleibt gültig, wenn ein anderer Sitz angenommen wurde. Eine Wiederholung nutzt
das gespeicherte Ergebnis, keine zweite Auflösung. Identitätskonflikte lassen
den ursprünglichen Request samt Receipt unverändert. Nicht valide Nachrichten
werden zurückgewiesen und nicht durch Abwerfen ersetzt.

Der Headless-Client liest Revisionen und echte Entscheidungen statt `turn+1`.
Er prüft in realen Partien physische Kartenerhaltung, gültige angewandte
Aktionen und Konsistenz von Punktekategorien, Gesamtresultat und `results.txt`.
Die Prozessregressionen prüfen außerdem Teilannahme, Wiederholung,
Request-/Bundlekonflikt, alte Partie bei gleichem Seed, veraltete Eingabe,
Kostenüberschreibung, fehlerhafte/mismatching Dateien, Ignorieren alter
V2-Dateien, expliziten Abbruch und Engine-Timeout ohne Ergebnisdatei.

C++-Fälle prüfen unabhängig erwartete Münzbeträge, minimale geteilte Zahlungen,
Käuferunabhängigkeit, Lighthouse/Vineyard, Pflichtauswahl und ihre Fehlerfälle,
mehrere Entscheidungen in einem Zug, letzte reguläre und siebte Ablagewahl,
synthetische Queue, automatische Leerfälle, neu aktiviertes Babylon, keine
rückwirkenden Bazar-Münzen, Timeout und Rückrollen einer absichtlich nach der
Stufenmutation geworfenen Ausnahme. Die bisherigen Wissenschafts-, Ressourcen-
und Schlusswertungsregressionen sind auf den aktuellen Kern angepasst;
die beiden älteren direkten Auswahlproduktionstests bleiben erhalten.

Maschinenlesbarer Nachweis: [resolution-validation.json](resolution-validation.json).

## Verbleibende Grenzen

- Der verbindliche Entscheidungsablauf ist implementiert, die BGA-Karten-
  und Wunderdefinitionen noch nicht. Babylon verwendet weiter seine alten
  Aktivierungsstufen; Olympia weiter das bisherige Einmal-pro-Ära-Recht.
  Es werden keine künftigen ersten/sechsten Freibaurechte beworben. Der Status
  kennzeichnet diese Fähigkeit als `legacy_olympia_once` und führt die
  Capability `legacy_wonder_abilities_v1`.
- Der alte GUI-Client lehnt Protokoll 3 ausdrücklich ab. Seine fünf echten
  V2-Integrationstests werden im aktuellen Baum als historische Tests ausgelassen;
  die alten GUI-Vertragsprüfungen bleiben erhalten und eine neue Prüfung weist
  die Ablehnung von Version 3 nach. Keine GUI- oder RL-Laufzeitdatei geändert.
- Frühere Python-Exporter und RL-Checkpoints werden nicht automatisch übernommen.
  Die getrennten Wissenschafts-/Gildenkopie- und Vorschaugrenzen bleiben offen.
- Keine Remote-Authentisierung, persistente Wiederaufnahme oder andere
  Spielerzahlen. Ein lokaler Koordinator erhält alle Hände; spätere Adapter
  müssen die vereinbarten Sitzansichten und getrennten Lernentscheidungen bilden.
- Kein Push/Release und keine Änderung von Parent-Submodule-Pin oder Lockdatei.
  Nach der Abnahme ist ausschließlich Paket 5-I (Kartendatentabellen) vorbereitet.
