# Sichere interne Ablageentnahme – Paket 4b-I

Aktueller Stand vom 6. November 2026: Karten, Wunder, GUI und RL sind migriert.
Die aktuelle Abnahme und ihre Grenzen stehen in [RELEASE_BGA.md](RELEASE_BGA.md).
Die folgenden Paketnachweise sind historisch und behalten ihre damaligen
Regelnamen, Beispiele und offenen Folgearbeiten.


Historischer Abnahmestand von Paket 4b-I. Paket 4b-II implementiert inzwischen
die vollständige Pflichtauswahl und ersetzt den unten beschriebenen V2-Pfad;
aktueller Stand: [SIMULTANEOUS_RESOLUTION.md](SIMULTANEOUS_RESOLUTION.md).

Stand: 5. November 2026. Entwicklungs-Regelstand `v0.2.0-dev`,
`bga-second-edition-development`, `second_edition_implemented=false`,
weiterhin Transportversion **2**. Der [Entscheidungsvertrag](DECISION_PROTOCOL_DRAFT.md)
ist ein Entwurf für die spätere vollständige Auflösung in 4b-II.

## Nachgewiesene Fehler und Korrektur

Vor der Korrektur scheitern drei getrennte Fälle von `test_discard_safety`:

| Aufruf | Geerbter Fehler |
| --- | --- |
| `build/test_discard_safety pile` | Erfolgreicher Bau lässt die gewählte Karte in der zentralen Ablage liegen: `BuildDiscardFree` erhält eine Kopie des Vektors. |
| `build/test_discard_safety hand` | Der Index in der Ablage löscht eine andere Handkarte in `BuildStructure`. Der Test wählt einen gültigen Index in beiden Beständen und prüft die Bestände ohne undefinierten Zugriff. |
| `build/test_discard_safety right` | Eine fehlende Ablagekarte verbraucht das Recht vor der Validierung; der anschließende gültige Versuch scheitert. |

`Card::CreateInstance()` erzeugt aus einer Definition eine physische Kopie mit
eindeutiger, nichtnuller Kennung. `Game::CreateDecks()` vergibt sie einmal pro
Partie, für jede der 63 Karten. Normales Kopieren, Mischen, Austeilen und
Weiterreichen erhält diese Kennung. `Card::Equal()` bleibt der Vergleich des
logischen Gebäudetyps; graue Altersvarianten gelten weiterhin als Duplikate.
Die Kennung ist parteilokal und noch kein neues V2-Beobachtungsfeld.

`Player::ApplyBuilding()` wendet die bisherigen Gebäudeeffekte und den
Stadteintrag an, ohne aus einer Quelle zu löschen. Der Handbau bindet seine
Entnahme an die tatsächliche Hand. `BuildDiscardFree()` bindet dagegen eine
physische Kennung ausschließlich an den zentralen Ablagevektor per Referenz.
Die gespeicherte Karte bestimmt Alter und Effekte; ein übergebener Kartenname
oder Kartentyp kann sie nicht ersetzen. Kosten und Ketten werden beim
kostenlosen Gebäudebau nicht geprüft oder bezahlt.

Der Ablagebau prüft Recht, eindeutige Instanz und eigene Gebäudeduplikate.
Anschließend erzeugt er einen Entwurf des Spielers und des verbleibenden
Ablagebestands. Effekte, Stadtzugang, Entnahme und Rechtverbrauch erfolgen
zunächst dort. Erst danach werden beide Ergebnisse ohne werfende Operation
übernommen. Ausnahmen davor lassen den Originalzustand unverändert; ein
erneuter Versuch kann genau einmal erfolgreich sein. Es wird keine Handkarte
entnommen. Die unmittelbaren Gebäudeeffekte werden geteilt, ihre Legacy-Werte
und die weiterhin sequenzielle reguläre Auszählung werden nicht migriert.

## Recht und alter Aufrufpfad

Die vorhandenen erfolgreichen Wunderstufen erteilen das Recht weiterhin an
den alten Aktivierungsgrenzen: Halikarnassos A Stufe 2, B Stufen 1–3. Eine
bereits verbrauchte oder abgelaufene Quellstufe erteilt es nicht erneut.
Ungültige Einzelaufrufe lassen das aktive Recht unverändert; bei null legalen
Optionen schließt `CloseDiscardIfNoOptions()` es automatisch ohne Münzen.
Dies implementiert noch nicht das vollständige Grant-/Request-Ledger.

Die alte V2-Eingabe `extra` wird pro Sitz gespeichert und erst an diesem Sitz
verwendet. Eine Namensvorwahl muss genau eine physische Ablagekarte bezeichnen;
mehrdeutige oder fehlende Vorwahlen bauen nichts. Die Engine wählt keine
Ersatzkarte. Nach dem V2-Zugfenster verfällt ein unerfülltes Recht, statt in
einen späteren Zug verschleppt zu werden. Auch bei einer internen Ausnahme
schließt der aufrufende Koordinator das Fenster und beendet den Lauf.

**Diese V2-Grenze ist keine regelkonforme Verzichtsoption.** V2 kann noch keine
Auswahl nach Veröffentlichung der vollständigen Ablage anfordern. Es erzwingt
daher auch nicht die jetzt festgelegte Wahlpflicht. Wartende Entscheidungen,
Warteschlange, Restkartenzeitpunkt, Babylon-/Ärenbarrieren, sichere
Quittierungen und Abbruchstatus gehören in 4b-II. Interne Fehler dürfen dort
keinen normalen Endzustand oder Terminalreward erzeugen. GUI und RL behalten
ihre getrennten Integrationspakete.

## Entschiedene Fachfragen

Der Nutzer hat am 5. November 2026 beide Fragen mit **nein** entschieden:

- **RQ-1:** Ein nachgelagerter Ablagebau verändert die zuvor abgerechneten
  regulären Vineyard-/Bazar-Auszahlungen nicht rückwirkend.
- **RQ-2:** Bei mindestens einer legalen Ablageoption ist eine Auswahl Pflicht;
  kein freiwilliges `skip`. Ohne legale Option automatische Erledigung.

Der Entwurf und D14/D26 der Nachrichtenbeispiele tragen diese verbindlichen
Projektvorgaben. Das ist kein unabhängiger Nachweis aus BGA-Servercode. Es
sind keine weiteren Quellenfreigaben für diese beiden Entscheidungen nötig.

## Prüfungen und verbleibende Grenzen

CTest `discard_safety` prüft zentrale Entnahme, Handisolation, gültigen Versuch
nach ungültiger Eingabe, zwei physische Kopien, graue Altersduplikate,
fehlendes/fremdes/verbrauchtes Recht, kostenlose Kosten-/Effektanwendung,
leere und unbaubare Ablage, echte Halikarnassos-Quellstufen, erneutes Lesen
und abgelaufene Rechte. Eine vollständige echte V2-Schlussrunde mit zusätzlichem
Ablagebau prüft die Sitzbindung in beiden Richtungen; mehrdeutige Namen werden
separat abgelehnt. Die 63 realen Deckkennungen und ihre Erhaltung beim
Austeilen und Weiterreichen werden geprüft.

Der Test erzwingt an **63 Allokationsstellen** vor der Transaktionsübernahme
je einen Speicherfehler. In jedem Fall bleiben Hand, Stadt, Ablage, Münzen
und Recht unverändert; ein anschließender Versuch wendet den Effekt genau
einmal an. Das ist ein gezielter Ausnahmesicherheitsnachweis, kein behaupteter
gesamter Spielabsturz-/Wiederaufnahmevertrag.

Release-Build, alle sieben CTests, die 19 bestehenden Python-/Sessiontests
und der eigenständige BGA-Zielaudit bestehen. Patch/Inventar und Herkunftsaudit
sind aktualisiert. Der optionale `--workspace ..`-Audit bindet historische
Paket-1-C++-Prüfsummen und meldet nach diesem erlaubten Umbau eine Abweichung.
Ein isolierter Vergleichs-Workspace mit den tatsächlich eingefrorenen
Legacy-Quelldateien prüft die unveränderten Originalziele weiterhin vollständig.
Diese historischen Prüfsummen werden nicht durch die neuen Werte ersetzt.

Keine Karten-/Wunderwerte, Wissenschaftsregeln, GUI-/RL-/Feature-/Export- oder
Trainingsdateien geändert. Die alten Python-Exporter bleiben an Legacy gebunden;
der ausgelagerte Effektcode benötigt bei einer späteren Entwicklungs-Engine-
Übernahme einen angepassten Exportweg. Gildenkopien und deren bestehende
Quellreferenzen sind nicht Teil dieser Ablagekorrektur. Kein Release/Push,
keine Aktualisierung von RL-Submodule-Pin oder `engine.lock.json`.

Maschinenlesbarer Nachweis: [discard-safety-validation.json](discard-safety-validation.json).
