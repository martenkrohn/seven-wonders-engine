# Gesamtabnahme und ausstehende Veröffentlichung

Der Nutzer hat alle Pakete bis einschließlich 9 freigegeben. Pakete 5–8 sind
implementiert und einzeln geprüft/committed. Die gemeinsame Abnahme enthält
zusätzlich die korrigierte Handelsrichtung, stabile Release-Metadaten und
Vertragstests. Siehe RELEASE_BGA.md und den Consumer-Abnahmenachweis.

Regeln: v0.2.0-bga / bga-second-edition / Protokoll 3 / decision-v1,
second_edition_implemented=true. Frozen Targets/Provenance/Referenzen und
Legacy-Tags bleiben unverändert. RQ-1/RQ-2 bleiben nein/nein.

Veröffentlichung ist noch nicht erfolgt. Die automatische Freigabeprüfung
hatte sogar den Remote-Push-Dry-run abgelehnt: konkretes Ziel nicht explizit
freigegeben und Gesamtabnahme damals nicht vollständig grün. Dies darf nicht
über andere Zugriffswege umgangen werden. Alle lokalen Arbeiten zuerst fertig
prüfen; dann konkrete Engine-/Consumer-Commits und Tags zur ausdrücklichen
Zielfreigabe vorlegen. Nutzerfortsetzung allein ist keine konkrete Push-Freigabe.
Engine zuerst veröffentlichen, danach den exakt passenden Consumer-Pin.
Keine Force-Pushes oder Änderungen alter Tags.
