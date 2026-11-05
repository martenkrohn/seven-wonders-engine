# Aktueller Gesamtauftrag: Pakete bis einschließlich 9

Der Nutzer hat ausdrücklich alle verbleibenden Pakete nacheinander freigegeben.
Die früheren Einzelpaket-Stopps sind aufgehoben. Keine erneute Bestätigung
für bereits autorisierte Folgepakete verlangen. Je Paket sorgfältig prüfen,
protokollieren und lokal committen.

5-IIa/5-IIb und sämtliche 6er-Wunderfamilien sind umgesetzt und getestet.
Aktueller nächster Schritt ist 7: neue Entscheidungs-GUI und BGA-Bilder/Layout,
dann 8: bewusste RL-Anbindung mit neuen Beobachtungs-/Aktions-/Checkpoint-
Verträgen und Kurztraining, dann 9: gemeinsame Abnahme und Veröffentlichung.
Siehe MIGRATION_PLAN.md und SIMULATOR_CHANGES.md für konkrete Commits/Tests.

Engine: /home/marten/dev/7Wonders/engine, eigener Git-Stand; Parent-RL:
/home/marten/dev/7Wonders, Branch refactor/separate-engine-20261105.
Frozen Targets/Referenz/Provenance bleiben unverändert. Alle Karten und Wunder
sind generiert; aktiver Transport 3, decision-v1, Entwicklungsregeln, noch keine
vollständige Consumer-/Releaseabnahme. RQ-1/RQ-2 bleiben nein/nein.
Legacy-Tags und alte Checkpoints erhalten, alte Checkpoints nicht stillschweigend
für neue Verträge zulassen. Pin/Lock erst bei bewusster Übernahme aktualisieren.
Bilder extern ../references/bga-sevenwonders/img, echte MIME-Typen beachten.
