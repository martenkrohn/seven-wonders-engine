# Audit der GUI-Wiedereinbindung

Aktueller Stand vom 6. November 2026: Karten, Wunder, GUI und RL sind migriert.
Die aktuelle Abnahme und ihre Grenzen stehen in [RELEASE_BGA.md](RELEASE_BGA.md).
Die folgenden Paketnachweise sind historisch und behalten ihre damaligen
Regelnamen, Beispiele und offenen Folgearbeiten.


Stand: 5. November 2026. Umfang: alte Oberfläche wieder nutzbar machen,
Kompatibilität mit dem ausgegliederten Simulator prüfen und Folgearbeit
abgrenzen. Spielregeln, Kartendefinitionen und Wunder werden hier nicht geändert.

## Herkunft und Ergebnis

Ausgangspunkt ist `Game.py` aus Upstream-Commit
`a0bc333a7a7f21f83156de8a2d522e7baa913f0a`. Dessen Tk-Canvas und grundlegende
Darstellungsfunktionen sind in `gui/legacy_view.py` erhalten. Der neue Launcher
verwendet dieselbe Oberfläche mit einem getrennt prüfbaren Protokollclient.
Es handelt sich um eine angepasste Wiederherstellung, nicht um unverändert
kopierten, ungeprüften Altcode. Die lokale alte `../Game.py` bleibt unangetastet.

Die Oberfläche ist für den bisherigen Drei-Spieler-Regelstand wieder nutzbar.
Sie ist ein optionaler Engine-Bestandteil; C++-Build und Botschnittstelle
benötigen weder Tk noch das RL-Projekt. Anleitung: [GUI.md](GUI.md).

## Gefundene Kompatibilitätsprobleme

| Befund im alten Client | Auswirkung | Stand nach Wiedereinbindung |
| --- | --- | --- |
| Pfade relativ zum aufrufenden Verzeichnis | Binary/Bilder fehlen bei anderem Startort | Launcher löst Engine-Pfad auf; Bilderordner explizit wählbar |
| Gemeinsamer Ordner `./io` | Mehrere Spiele/RL-Prozesse können sich überschreiben | Jede GUI-Partie erhält ein eigenes temporäres Laufzeitverzeichnis |
| Feste Pause von 0,5 s | Alter Status bei langsamem Start oder Zugwechsel | Nicht blockierendes Tk-Polling auf erwartete Runde mit Timeout |
| Unvollständige/nicht atomare Veröffentlichung | Engine kann unfertige Aktionsdateien lesen | Alle drei Aktionen prüfen, atomar schreiben, Ready-Signal zuletzt |
| Keine explizite Rundennummer | Veraltete Aktionen schwer erkennbar | Jeder Befehl trägt die erwartete Runde |
| Schließen vor Verarbeitung der Runde 20 | Vollständige Schlusswertung wird nicht angezeigt | Phase `finished`, Runde 21 und Endpunkte bleiben sichtbar |
| Engine-Thread ohne geordnetes Beenden | Engine kann nach Fensterschluss warten | Prozess wird beendet und eingesammelt; temporäre Dateien entfernt |
| `num_bots` überspringt nur menschliche Eingabe | Keine eigenen Botaktionen; kein spielbarer Begleiter garantiert | Einfache deterministische Begleiter schreiben vollständige Aktionen |
| Auswahl-/Aktionszustand bleibt über Züge erhalten | Ungültige Freibaumodi/alte Kartenauswahl möglich | Auswahl wird pro neuem Status zurückgesetzt und vollständig validiert |
| Freibaurecht wird mit jeder Handkarte angeboten | Doppelte Karte kann abgelehnt werden und das alte Engine-Recht trotzdem verbrauchen | Bereits gebaute Karten vor Freibaueingabe zurückweisen |
| Siebte Karte ohne Sonderbehandlung | Unnötige Eingabe oder vorzeitiger Abbruch | Automatische Quittierung, aktive alte Babylon-B-Fähigkeit berücksichtigt |
| Ständiges Hinzufügen von Widgets | Verdeckte/alte Elemente sammeln sich an | Alte Rundenansicht wird vor Neuzeichnen entfernt; Bildobjekte gecacht |
| Graue Karten werden nach Namen bebildert | Falsche Altersvariante, insbesondere Glassworks | Alterszuordnung für Hand und Zeitpunkt des Baus gespeichert |
| Bilddatei oder externe Sortiertabelle fehlt | GUI startet nicht | Beschriftete Bildplatzhalter; Sortierung mit IDs aus Engine-Status |
| 18er-Grenze bei gespielten Karten | Zusätzliche Babylon-Karten können sich überlagern | Drei kompaktere Spalten mit je sieben Karten |

Eine Engine-Ersatzaktion (`fallback_discard`) wird im Fenster kenntlich
gemacht. Dieser Hinweis repariert nicht die zugrunde liegende Spielregel.

## Offene Baustellen

| Bereich | Befund | Zuständiger weiterer Schritt |
| --- | --- | --- |
| Halikarnassos | `extra` ist bisher keine vollständige Entscheidung; Engine hat unsichere Stapel-/Handentnahme | Protokoll und Engine zuerst reparieren, dann eigene GUI-Auswahl |
| Siebte Karte | Client kennt alte Babylon-B-ID 8 und Stufe 2; die neue Edition gibt die Fähigkeit früher | Explizite Engine-Entscheidungsdaten statt ID-/Stufenheuristik |
| Spielregeln | Gierige Wissenschaftsjoker, sequenzielle Auflösung und vier bekannte Kartendatenabweichungen bestehen | Eigene fachliche Engine-Arbeitspakete |
| Tableaus | Alte absolute Positionen; Stufenmarkierungen für zwei/vier Stufen nicht tafelspezifisch | Darstellung aus kanonischen Stufenplänen aufbauen |
| Neue Kartenbilder | BGA-Sprites sind hochkant (255 × 393); alte Bilder sind quer (625 × 266) | Hand-/Kartenlayout bewusst anpassen; Austausch von Dateinamen reicht nicht |
| Neue Tableaus | Neue Höhe 223 statt 198, Babylon B hat zwei Stufen | Neue Geometrie und dynamische Stufenanzeige |
| Bildformate | Acht lokale BGA-Downloads enthalten WebP trotz anderer Dateiendung | MIME prüfen; geeigneten Decoder/Importweg wählen, Tk-PNG-Lader nicht ungeprüft verwenden |
| Namen/IDs | BGA-ID, Engine-ID und Sprite-Index sind verschiedene Schlüssel; BGA-Kartenbilder haben keine Namensaufschrift | Geprüfte Zuordnung nach Karte/Zeitalter, Namen als GUI-Overlay |
| Darstellung/Bedienung | Festes Desktoplayout, alle Hände offen, Grundtexte überwiegend Englisch | Spätere eigene GUI-Arbeit; keine privaten Mehrspieler-/Netzwerkansichten zugesagt |
| Bildbereitstellung | Alte und neue Bildreferenzen liegen lokal außerhalb der versionierten Engine | Externer Bilderordner bleibt unterstützt; Grafikpaket ist kein impliziter Release-Bestandteil |

## Nachweise

`tests/test_gui_session.py` prüft acht Verträge ohne Display. Drei
Bedienungsmodi (0/2/3 Bots) liefern mit denselben Entscheidungen exakt den
Spielverlaufs-Hash des vorhandenen Protokoll-Smoke-Clients. Weitere Fälle:
ungültige Eingaben vor Ready, Prozessabbruch, Timeout, isolierte Sessions,
Prozessbereinigung, alte Babylon-B-Grenze und graue Bildvarianten.

`scripts/smoke_gui.py` bedient das tatsächliche Tk-Fenster mit Klickereignissen
und Aktionsbuttons bis zur vollständigen Schlussanzeige. Prüfungen und ihre
konkreten Grenzen werden in [gui-validation.json](gui-validation.json) festgehalten.
Sie ersetzen keine vollständige manuelle Abnahme sämtlicher Wunderfähigkeiten.

Der bestehende C++-Herkunftsaudit und die fünf CTests bleiben maßgeblich. Die
Spielquelldateien und `CMakeLists.txt` bleiben gegenüber `v0.1.0-legacy`
unverändert. Die BGA-Definitionen wurden durch diese Arbeit nicht implementiert.
