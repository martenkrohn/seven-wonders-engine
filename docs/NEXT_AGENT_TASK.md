# Nächster Agent: ausschließlich Paket 4b-I – sichere Ablageentnahme

Arbeite im Workspace `/home/marten/dev/7Wonders`. Die Engine ist das
Git-Submodule `engine/`; das RL-Projekt liegt auf
`refactor/separate-engine-20261105`. Prüfe beide Git-Stände, erhalte vorhandene
Änderungen und lies den aktuellen Engine-HEAD. Der gespeicherte RL-Pin und
`engine.lock.json` bleiben bei `f39c1d7` (`v0.1.1-legacy`); nicht zurücksetzen
oder aktualisieren.

Paket 4a ist als Spezifikation abgeschlossen:
`0aaea3651915b071b95f2ae0f87d243b1845c126`. Es hat keine Laufzeitlogik geändert.
Der aktuelle Vertrag bleibt `v0.2.0-dev`,
`ruleset=bga-second-edition-development`, `second_edition_implemented=false`,
Transportversion **2**. Der dokumentierte Protokoll-3-Entwurf ist nicht aktiv.

Lies zuerst, relativ zum Engine-Verzeichnis:

1. `docs/MIGRATION_PLAN.md`, besonders die Abnahme von Paket 4a und die Trennung
   von 4b-I/4b-II; `engine-contract.json`, `docs/KNOWN_ISSUES.md` und das
   implementierte `docs/PROTOCOL.md`.
2. `docs/DECISION_PROTOCOL_DRAFT.md`, besonders Identitäten, Ablageentnahme,
   Grantverbrauch und die beiden offenen Regelfragen; dazu
   `docs/decision-protocol-examples.json` (D01–D16 und D36) sowie
   `docs/decision-protocol-validation.json`. Die Beispiele kürzen Optionen und
   öffentliche Ökonomiefelder; sie sind keine vollständigen Laufzeitfixtures.
3. `src/player.cpp::BuildDiscardFree/BuildStructure`, deren Deklarationen,
   `Card::Equal`, Kartenkopien und Deck-/Ablagebesitz sowie die Aufrufstellen
   in `main.cpp::Loop`. Prüfe die tatsächliche Handentnahme und alle
   Gebäudeeffekte, bevor du den Baupfad teilst.
4. Bestehende C++-Regressionen, `CMakeLists.txt`, `docs/SIMULATOR_CHANGES.md`
   und den Herkunftsaudit unter `scripts/audit_simulator.py`.

**Ziel:** Die interne Grundlage für kostenlosen Gebäudebau aus der zentralen
Ablage sicher machen. Dieses Paket implementiert weder den vollständigen
Entscheidungsautomaten noch simultane Runden-/Zahlungsauflösung.

Lieferumfang:

- Zuerst aussagekräftige rote C++-Fälle für die geerbten Fehler nachweisen:
  Ablage wird per Wert übergeben, eine fremde Ablageposition löscht aus der
  Hand, und eine erfolglose Wahl verbraucht das Freibaurecht. Keine undefinierte
  Iteratoroperation als einzigen Testbeleg verwenden; klare Bestands- und
  Effektinvarianten prüfen.
- Die zentrale Ablage als tatsächlichen Eigentümer behandeln. Eine Auswahl
  bindet eine stabile physische Instanz, getrennt von Kartentyp, Zeitalter,
  Name und Vektorindex. Kopieren/Weiterreichen/Entnehmen erhält Identitäten;
  verschiedene physische Kopien dürfen nicht dieselbe Kennung erhalten.
  Keine Datenbankwerte, Kartenmengen oder Wunderdefinitionen verändern.
- Gebäudeanwendung und Entnahme aus der Quelle fachlich trennen. Ablagebau
  darf niemals Handkarten löschen. Karte genau einmal aus der zentralen
  Ablage entfernen, genau einmal zur Stadt hinzufügen und Effekte genau
  einmal anwenden; Kosten und Ketten sind dabei irrelevant. Doppelte eigene
  Gebäudetypen auch über graue Altersvarianten ablehnen. Wunderbau/Verkauf
  einer Ablagekarte nicht als Ersatz zulassen.
- Rechtverbrauch an einen erfolgreichen atomaren Bau binden. Falsche oder
  fehlende Instanz, falscher Besitzer/fehlendes Recht, doppelte Karte und
  erneuter Aufruf nach Erfolg ändern keine Bestände/Effekte. Bei leerer oder
  ausschließlich unbaubarer Ablage eine interne automatische Erledigung
  ohne Münzen und ohne Handaktion ermöglichen; kein altes Recht in eine
  spätere Runde verschleppen oder beim Lesen eines alten Stufenstands neu
  erzeugen. Die bestehenden Legacy-Aktivierungsstufen hier nicht migrieren.
- Einen internen Fehlerfall an einer sinnvollen Transaktionsgrenze prüfen:
  kein Teilzustand mit entnommener Karte, fehlender Handkarte oder vorzeitig
  verbrauchtem Recht. Wiederholungen dürfen keine zweite Entnahme oder
  Effektanwendung bewirken. Die vollständige Request-/Receipt-Deduplikation
  des Entwurfs bleibt für 4b-II; Kernoperationen sind direkt zu testen.
- Aufrufstellen nur soweit anpassen, wie sichere Eigentümerschaft,
  Instanzbindung und Sitzzuordnung es verlangen. Kein global überschriebenes
  `extra` als Auswahl für einen anderen Sitz verwenden. Falls der alte
  namensbasierte V2-Pfad erhalten bleibt, seine Grenzen ausdrücklich
  dokumentieren und eine nicht eindeutig bindbare Auswahl ablehnen.
  Keine sichtbare neue Ablagewahl oder zusätzliche Dateihandshakes einführen;
  keine verdeckte Vorauswahl als vollständigen BGA-Pfad ausgeben.
- Gezielt prüfen: verschiedene Hand-/Ablagepositionen, Hand bleibt unverändert,
  zwei physische Kopien in synthetischem Bestand, graue Altersduplikate,
  fehlende/verbrauchte Berechtigung, ungültige Auswahl mit anschließendem
  gültigem Versuch, Wiederholung nach Erfolg, null legale Optionen sowie
  unmittelbare Gebäudeanwendung ohne doppelte Effekte. Die eigentliche
  Ablageabfrage und Mehrfach-Grant-Warteschlange bleiben für 4b-II.
- Release-Build und alle CTests, Herkunftsaudit und BGA-Zielaudit ausführen;
  betroffene bestehende Python-/Sessiontests nur soweit Schnittstellen
  berührt werden. C++-Inventar/Patch und `docs/SIMULATOR_CHANGES.md` mit der
  konkreten Verhaltensgrenze aktualisieren. Keine neue Transportversion oder
  fertige BGA-Edition behaupten. Ergebnis, Fehlernachweise, Prüfungen, Commit
  und Grenzen im Plan dokumentieren und lokal committen.

**Offene Fachfragen:** RQ-1 (reguläre Münzzähler bei späterer Ablagekonstruktion)
und RQ-2 (freiwilliger Verzicht trotz legaler Option) benötigen gezielte
Bestätigung vor den im Entwurf genannten Freigaben. Sie blockieren dieses
interne Sicherheitspaket nicht. Hier keinen unbekannten BGA-Regelwert erraten
und keine Gesamt-Recherche wiederholen.

**Grenzen:** Keine neue Runden-/Zahlungsauflösung, keine Restkarten-/Babylon-/
Ärenbarrieren, keine Protokoll-3-/GUI-/RL-Implementierung. Keine neuen
Karten-/Wunderdaten, keine Wissenschafts-/Gildenkopierkorrektur und keine
Export-/Feature-/Trainingsänderung. Kein Push/Release, kein Submodule-/Lock-
Update, keine Checkpointmigration. Nach Abschluss ausschließlich Paket 4b-II
als eigenen nächsten Auftrag vorbereiten und stoppen; kein weiteres Paket
automatisch beginnen.
