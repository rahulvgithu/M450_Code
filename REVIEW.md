# Code-Review Parksystem (A04-05)

**Gruppenmitglied:** Rahul Vaikunthavasan (Einzelarbeit, INA24cL)
**Reviewter Stand:** Branch `testdoubles`, Commit `fdf0200` (Version 1.2)
**Datum:** 26.11.2026
**Vorgehen:** Datei für Datei durchgegangen (Selbstreview, da Einzelarbeit). Zeilennummern beziehen sich auf den reviewten Stand `fdf0200`.

Prioritäten: **H** = hoch (Fehlerquelle oder stark erschwerte Lesbarkeit), **M** = mittel, **N** = niedrig.

## Findings

| ID | Datei | Zeile | Problem | Vorschlag | Prio | Status |
|---|---|---|---|---|---|---|
| R-01 | `lib/Parksystem/src/KasseLogik.cpp` / `SchrankenLogik.cpp` / beide `main.cpp` | 6 / 5 / 6–7 | Versionsnummer `"1.2"` steht doppelt im Code und zusätzlich in Kommentaren. Bei einem Update wird eine Stelle vergessen. | Eine Konstante `VERSION` in `Version.h`, Kommentare ohne Nummer | M | offen |
| R-02 | `KasseLogik.cpp`, `Parkhaus_Schranke/src/main.cpp`, `lib/TestDoubles/src/KasseSpy.h` | 118–134 / 57–60 / 17–20 | Protokoll-Befehle `"REG"`, `"EXIT"`, `"PING"`, `"PONG"`, `"HELP"`, `"OK HELP"` sind als Magic Strings in drei Dateien verteilt. `substr(3)` / `substr(4)` hängen von der Länge des Strings ab. Ein Tippfehler bricht die Kommunikation, ohne dass der Compiler warnt. | Konstanten und Hilfsfunktionen im Namespace `protokoll` (`KassenProtokoll.h`) | H | offen |
| R-03 | `Kasse/src/main.cpp`, `Parkhaus_Schranke/src/main.cpp` | 18, 57–60 / 24, 88–91, 95 | Magic Numbers: UART-Pins `0`/`1`, Baudrate `9600` (muss auf beiden Picos gleich sein, ist aber zweimal definiert), Wartezeiten `delay(2000)` / `delay(1000)`. | Gemeinsame Konstanten `UART_BAUD`, `UART_TX_PIN`, `UART_RX_PIN`; benannte Wartezeiten | H | offen |
| R-04 | `Kasse/src/main.cpp`, `Parkhaus_Schranke/src/main.cpp` | 23–33 / 29–39 | Duplizierter Code: `MillisUhr` und `SerialAnzeige` sind in beiden `main.cpp` identisch. | Gemeinsame Bibliothek `lib/ArduinoAdapter` (nur für den Pico) | M | offen |
| R-05 | `Kasse/src/main.cpp` | 49 | Magic Number `64` (maximale Zeilenlänge im UART-Puffer). | Konstante `MAX_ZEILENLAENGE` | N | offen |
| R-06 | `KasseLogik.cpp` | 21–23, 53 | Magic Numbers `86400`, `3600`, `60`, `1000` in der Zeitberechnung. | Benannte Konstanten (`SEKUNDEN_PRO_TAG` …) | M | offen |
| R-07 | `lib/Parksystem/src/Kennzeichen.cpp` | 21, 31 | Magic Numbers `3`, `8`, `2` (Länge und Position der Nummer im Kennzeichen). | Konstanten `KANTON_LAENGE`, `MIN_ZIFFERN`, `MAX_ZIFFERN` | N | offen |
| R-08 | `KasseLogik.cpp` | 19–58 | `KasseLogik` hat zu viele Verantwortungen (255 Zeilen): Parkvorgänge, Zahlung, Protokoll **und** Uhr stellen, Uhrzeit parsen und formatieren. `uhrStellen()` ist 30 Zeilen lang und macht vier Dinge (zerlegen, prüfen, setzen, ausgeben). | Eigene Klasse `Wanduhr` (stellen, parsen, formatieren), `KasseLogik` nutzt sie | H | offen |
| R-09 | `KasseLogik.cpp` | 145, 172 | Duplizierte Gebührenberechnung `tarif::gebuehrRappen(uhr_.jetztMs() - p->einfahrtMs)`. | Methode `aktuelleGebuehr(const Parkvorgang&)` | M | offen |
| R-10 | `KasseLogik.cpp` | 149–153, 163–171, 180–188 | Duplizierte Prüfungen «Kein aktiver Parkvorgang» und «Bereits bezahlt» in drei Methoden. | Methoden `findeOderMelde()` und `findeUnbezahltOderMelde()` | M | offen |
| R-11 | `KasseLogik.cpp` | 62–63, 109–110 | Duplizierte Logik «an Liste anhängen und älteste Einträge entfernen» (Protokoll und Historie). | Hilfsfunktion `begrenztAnhaengen()` | N | offen |
| R-12 | `lib/Parksystem/src/SchrankenLogik.h` | 30–35, 47–48 | Unklare Namen: Die Struktur `Zustand` beschreibt eigentlich eine gesteuerte Schranke; die Member heissen `ein_` / `aus_` / `s`. | `Zustand` → `GesteuerteSchranke`, `ein_` → `einfahrt_`, `aus_` → `ausfahrt_` | M | offen |
| R-13 | `SchrankenLogik.cpp` | 27, 32, 49, 60, 64 | Text-Trick `name + "sschranke"` («Einfahrt» + «sschranke» = «Einfahrtsschranke») ist schwer verständlich und fünfmal dupliziert. | Vollständigen Namen speichern, Methode `meldeBleibtGeschlossen()` | H | offen |
| R-14 | `SchrankenLogik.cpp` | 45–70 | Flag-Parameter `bool istEinfahrt` plus drei `? :`-Verzweigungen in `bearbeite()`. Die Methode macht für Ein- und Ausfahrt Verschiedenes, gesteuert von einem bool. | Richtung (`Einfahrt`/`Ausfahrt`) und Begrüssung in der Schranke speichern; `bearbeite()` ohne bool | M | offen |
| R-15 | `KasseLogik.cpp` / `SchrankenLogik.cpp` | 237–255 / 90–111 | Duplizierte Befehlszerlegung (trim, erster Buchstabe gross, Rest als Argument); im `switch` stehen teils mehrere Anweisungen pro `case`. | `text::Befehl zerlege()`; Fälle mit mehreren Anweisungen in Methoden (`alleSchliessen()`, `hilfeRufen()`) | N | offen |
| R-16 | `KasseLogik.cpp` | 71–93, 95–113 | Fehler werden als `std::string` zurückgegeben, leerer String = OK. Diese Konvention ist nicht selbsterklärend. | Rückgabetyp `Ergebnis { bool ok; std::string fehler; }` | M | offen |
| R-17 | `backup_v1.0/` (beide Projekte) | – | Toter Code: alter Stand v1.0 als Kopie im Repository, wird nie kompiliert. Git-Historie (Commit `586dd67`) enthält ihn bereits. | Ordner löschen | N | offen |
| R-18 | `Parkhaus_Schranke/src/main.cpp` | 94 | Kommentar `// schliesst beide Schranken` erklärt, was der Methodenname `start()` nicht sagt. | Kommentar entfernen oder Methode umbenennen | N | offen |
| R-19 | `Parkhaus_Schranke/src/main.cpp` | 64–76 | `UartKasse::anfrage()` (Timeout, Puffer) ist hardwareabhängig und ohne Test. | Zeilenleser von `Serial1` trennen, um ihn testen zu können | M | offen |

## Bereits in A02 (Test Doubles) behoben

Vergleich mit Version 1.1 (Branch `main`). Diese Findings hätte ein Review von v1.1 ergeben. Sie wurden schon beim Umbau für die Test Doubles umgesetzt:

| Datei (v1.1) | Zeile | Problem | Umsetzung in v1.2 |
|---|---|---|---|
| `Kasse/src/main.cpp` | 98, 104, 182, 210, 215, 232–262 | Logik (Gebühr, Zahlung) und Hardware (`millis()`, `Serial.println()`) vermischt | Logik in `lib/Parksystem`, Hardware hinter Schnittstellen (`IUhr`, `IAnzeige`) |
| `Kasse/src/main.cpp` | 1–478 | Eine Datei mit 478 Zeilen und globalen Variablen | `KasseLogik` als Klasse, `main.cpp` nur noch 70 Zeilen |
| `Parkhaus_Schranke/src/main.cpp` | 68–80, 99–109 | UART-Zugriffe mitten in der Schrankenlogik | `IKasse` mit `UartKasse` (Pico) und `KasseStub`/`KasseSpy` (Test) |
| `Parkhaus_Schranke/src/main.cpp` | 171–215 | Grosses `loop()` mit `if/else`-Kette über alle Befehle | `SchrankenLogik::befehl()`, `loop()` hat 5 Zeilen |
