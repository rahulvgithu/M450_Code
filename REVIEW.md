# Code-Review und Refactoring Parksystem (A04-05)

**Gruppenmitglied:** Rahul Vaikunthavasan (Einzelarbeit, INA24cL)
**Reviewter Stand:** Branch `testdoubles`, Commit `fdf0200` (Version 1.2)
**Refactoring:** Branch `refactoring`
**Datum:** 26.11.2026
**Vorgehen:** Datei für Datei durchgegangen. Da es eine Einzelarbeit ist, war es ein Selbstreview, und Driver/Navigator konnten nicht zwischen zwei Personen wechseln. Als Ersatz für den Navigator diente der Regressionstest: Nach **jedem** Schritt liefen alle automatisierten Tests und beide Pico-Builds. Zeilennummern beziehen sich auf den reviewten Stand `fdf0200`.

Prioritäten: **H** = hoch (Fehlerquelle oder stark erschwerte Lesbarkeit), **M** = mittel, **N** = niedrig.

## Findings

| ID | Datei | Zeile | Problem | Vorschlag | Prio | Status |
|---|---|---|---|---|---|---|
| R-01 | `lib/Parksystem/src/KasseLogik.cpp` / `SchrankenLogik.cpp` / beide `main.cpp` | 6 / 5 / 6–7 | Versionsnummer `"1.2"` steht doppelt im Code und zusätzlich in Kommentaren. Bei einem Update wird eine Stelle vergessen. | Eine Konstante `VERSION` in `Version.h`, Kommentare ohne Nummer | M | ✅ `d90c29f` |
| R-02 | `KasseLogik.cpp`, `Parkhaus_Schranke/src/main.cpp`, `lib/TestDoubles/src/KasseSpy.h` | 118–134 / 57–60 / 17–20 | Protokoll-Befehle `"REG"`, `"EXIT"`, `"PING"`, `"PONG"`, `"HELP"`, `"OK HELP"` sind als Magic Strings in drei Dateien verteilt. `substr(3)` / `substr(4)` hängen von der Länge des Strings ab. Ein Tippfehler bricht die Kommunikation, ohne dass der Compiler warnt. | Konstanten und Hilfsfunktionen im Namespace `protokoll` (`KassenProtokoll.h`) | H | ✅ `925fc65` |
| R-03 | `Kasse/src/main.cpp`, `Parkhaus_Schranke/src/main.cpp` | 18, 57–60 / 24, 88–91, 95 | Magic Numbers: UART-Pins `0`/`1`, Baudrate `9600` (muss auf beiden Picos gleich sein, ist aber zweimal definiert), Wartezeiten `delay(2000)` / `delay(1000)`. | Gemeinsame Konstanten `UART_BAUD`, `UART_TX_PIN`, `UART_RX_PIN`; benannte Wartezeiten | H | ✅ `e9475e5` |
| R-04 | `Kasse/src/main.cpp`, `Parkhaus_Schranke/src/main.cpp` | 23–33 / 29–39 | Duplizierter Code: `MillisUhr` und `SerialAnzeige` sind in beiden `main.cpp` identisch. | Gemeinsame Bibliothek `lib/ArduinoAdapter` (nur für den Pico) | M | ✅ `f72f397` |
| R-05 | `Kasse/src/main.cpp` | 49 | Magic Number `64` (maximale Zeilenlänge im UART-Puffer). | Konstante `MAX_ZEILENLAENGE` | N | ✅ `e9475e5` |
| R-06 | `KasseLogik.cpp` | 21–23, 53 | Magic Numbers `86400`, `3600`, `60`, `1000` in der Zeitberechnung. | Benannte Konstanten (`SEKUNDEN_PRO_TAG` …) | M | ✅ `ba44cc9` |
| R-07 | `lib/Parksystem/src/Kennzeichen.cpp` | 21, 31 | Magic Numbers `3`, `8`, `2` (Länge und Position der Nummer im Kennzeichen). | Konstanten `KANTON_LAENGE`, `MIN_ZIFFERN`, `MAX_ZIFFERN` | N | ✅ `f265395` |
| R-08 | `KasseLogik.cpp` | 19–58 | `KasseLogik` hat zu viele Verantwortungen (255 Zeilen): Parkvorgänge, Zahlung, Protokoll **und** Uhr stellen, Uhrzeit parsen und formatieren. `uhrStellen()` ist 30 Zeilen lang und macht vier Dinge (zerlegen, prüfen, setzen, ausgeben). | Eigene Klasse `Wanduhr` (stellen, parsen, formatieren), `KasseLogik` nutzt sie | H | ✅ `ba44cc9` |
| R-09 | `KasseLogik.cpp` | 145, 172 | Duplizierte Gebührenberechnung `tarif::gebuehrRappen(uhr_.jetztMs() - p->einfahrtMs)`. | Methode `aktuelleGebuehr(const Parkvorgang&)` | M | ✅ `c54a450` |
| R-10 | `KasseLogik.cpp` | 149–153, 163–171, 180–188 | Duplizierte Prüfungen «Kein aktiver Parkvorgang» und «Bereits bezahlt» in drei Methoden. | Methoden `findeOderMelde()` und `findeUnbezahltOderMelde()` | M | ✅ `5fc02d5` |
| R-11 | `KasseLogik.cpp` | 62–63, 109–110 | Duplizierte Logik «an Liste anhängen und älteste Einträge entfernen» (Protokoll und Historie). | Hilfsfunktion `begrenztAnhaengen()` | N | ✅ `2e4628c` |
| R-12 | `lib/Parksystem/src/SchrankenLogik.h` | 30–35, 47–48 | Unklare Namen: Die Struktur `Zustand` beschreibt eigentlich eine gesteuerte Schranke; die Member heissen `ein_` / `aus_`. | `Zustand` → `GesteuerteSchranke`, `ein_` → `einfahrt_`, `aus_` → `ausfahrt_` | M | ✅ `27f4f97` |
| R-13 | `SchrankenLogik.cpp` | 27, 32, 49, 60, 64 | Text-Trick `name + "sschranke"` («Einfahrt» + «sschranke» = «Einfahrtsschranke») ist schwer verständlich und fünfmal dupliziert. | Vollständigen Namen speichern, Methode `meldeBleibtGeschlossen()` | H | ✅ `30e0828` |
| R-14 | `SchrankenLogik.cpp` | 45–70 | Flag-Parameter `bool istEinfahrt` plus drei `? :`-Verzweigungen in `bearbeite()`. | Richtung (`Einfahrt`/`Ausfahrt`) und Begrüssung in der Schranke speichern; `bearbeite()` ohne bool | M | ✅ `30e0828` |
| R-15 | `KasseLogik.cpp` / `SchrankenLogik.cpp` | 237–255 / 90–111 | Duplizierte Befehlszerlegung (trim, erster Buchstabe gross, Rest als Argument); im `switch` stehen teils mehrere Anweisungen pro `case`. | `text::zerlegeBefehl()`; Fälle mit mehreren Anweisungen in Methoden (`alleSchliessen()`, `hilfeRufen()`) | N | ✅ `a9ccf27` |
| R-16 | `KasseLogik.cpp` | 71–93, 95–113 | Fehler werden als `std::string` zurückgegeben, leerer String = OK. Diese Konvention ist nicht selbsterklärend. | Rückgabetyp `Ergebnis { bool ok; std::string fehler; }` | M | ❌ nicht umgesetzt |
| R-17 | `backup_v1.0/` (beide Projekte) | – | Toter Code: alter Stand v1.0 als Kopie im Repository, wird nie kompiliert. Git-Historie (Commit `586dd67`) enthält ihn bereits. | Ordner löschen | N | ❌ nicht umgesetzt |
| R-18 | `Parkhaus_Schranke/src/main.cpp` | 94 | Kommentar `// schliesst beide Schranken` erklärt, was der Methodenname `start()` nicht sagt. | Kommentar entfernen oder Methode umbenennen | N | ❌ nicht umgesetzt |
| R-19 | `Parkhaus_Schranke/src/main.cpp` | 64–76 | `UartKasse::anfrage()` (Timeout, Puffer) ist hardwareabhängig und ohne Test. | Zeilenleser von `Serial1` trennen, um ihn testen zu können | M | ❌ nicht umgesetzt |

**Ergebnis:** 15 von 19 Findings umgesetzt (alle mit Priorität H), in 12 Refactoring-Commits.

## Nicht umgesetzte Findings und Begründung

| ID | Warum nicht? |
|---|---|
| R-16 | Ändert die öffentliche Schnittstelle von `KasseLogik` (`einfahrt()`, `ausfahrt()`), damit auch `verarbeiteAnfrage()` und rund 20 Tests. Das ist ein grösserer Umbau mit Risiko, aber wenig Nutzen: Die Konvention ist im Header dokumentiert. Geplant für eine spätere Version. |
| R-17 | Das abgegebene Testprotokoll (A01-04) verweist auf den Ordner `backup_v1.0`. Löschen würde diesen Verweis brechen. |
| R-18 | `start()` macht mehr als schliessen (auch Startmeldung und Hilfe). Ein neuer Name wäre länger und nicht klarer; der Kommentar ist hier hilfreich. |
| R-19 | Der UART-Teil hängt direkt an `Serial1`. Ohne Pico kann man nicht prüfen, ob nach dem Umbau noch alles funktioniert (keine Tests, keine Hardware vorhanden). Ein Refactoring ohne Sicherheitsnetz wäre riskant. |

## Regressionstest nach jedem Schritt

- Vor dem ersten Refactoring wurden **8 Charakterisierungstests** ergänzt (Commit `8fbc75d`). Sie halten die genauen Ausgaben fest (Startmeldung, Hilfe, Schrankenmeldungen, Uhrzeit, Begrenzung von Protokoll und Historie).
- Nach jedem Commit: `pio test -e native` in beiden Projekten (**62 Tests**, Bezug zu TC01–TC20 aus dem Testkonzept) und `pio run -e pico` für beide Picos.
- Ergebnis: nach **jedem** der 12 Refactoring-Commits 62/62 Tests grün und beide Firmwares gebaut. Das Verhalten ist unverändert.

## Vorher / Nachher

**Beispiel 1: Text-Trick und Flag-Parameter (R-13, R-14)**

```cpp
// VORHER (SchrankenLogik.cpp)
void SchrankenLogik::bearbeite(Zustand &s, bool istEinfahrt, const std::string &eingabe) {
    ...
    KassenAntwort antwort = istEinfahrt ? kasse_.einfahrt(kz) : kasse_.ausfahrt(kz);
    switch (antwort.status) {
        case KassenAntwort::OK:
            anzeige_.zeige((istEinfahrt ? "Willkommen " : "Gute Fahrt ") + kz + ".");
            ...
        case KassenAntwort::ABGELEHNT:
            anzeige_.zeige("ABGELEHNT: " + antwort.grund + ".");
            anzeige_.zeige(std::string(s.name) + "sschranke bleibt GESCHLOSSEN"); // "Einfahrt"+"sschranke"
```

```cpp
// NACHHER
void SchrankenLogik::bearbeite(GesteuerteSchranke &s, const std::string &eingabe) {
    ...
    KassenAntwort antwort = frageKasse(s, kz);
    switch (antwort.status) {
        case KassenAntwort::OK:
            anzeige_.zeige(std::string(s.begruessung) + " " + kz + ".");
            ...
        case KassenAntwort::ABGELEHNT:
            anzeige_.zeige("ABGELEHNT: " + antwort.grund + ".");
            meldeBleibtGeschlossen(s);
```

**Beispiel 2: Magic Strings im Protokoll (R-02)**

```cpp
// VORHER: in 3 Dateien verteilt, substr(3) haengt an der Laenge von "REG"
if (text::beginntMit(zeile, "REG")) {
    std::string kz = kennzeichen::normalisiere(zeile.substr(3));
    return fehler.empty() ? "OK REG " + kz : "ERR REG " + fehler;
```

```cpp
// NACHHER: eine Stelle (KassenProtokoll.h), Compiler findet Tippfehler
if (protokoll::istAnfrage(zeile, protokoll::EINFAHRT)) {
    std::string kz = kennzeichen::normalisiere(protokoll::argument(zeile, protokoll::EINFAHRT));
    return fehler.empty() ? protokoll::okAntwort(protokoll::EINFAHRT, kz)
                          : protokoll::fehlerAntwort(protokoll::EINFAHRT, fehler);
```

## Reflexion

**Was hat das Refactoring erleichtert?**
- Die **automatisierten Tests aus A02** (Test Doubles). Jeder Schritt war in rund 10 Sekunden ohne Pico überprüft. Ohne sie hätte jeder Schritt einen Upload und eine manuelle Testdurchführung gebraucht.
- Die **Charakterisierungstests** vor dem Start. Sie haben genau die Texte festgehalten, die sich beim Umbau der Meldungen (R-13) nicht ändern durften.
- Die Trennung von Logik und Hardware aus A02. Fast alle Findings lagen in der Logik und waren dadurch testbar.
- **Kleine Schritte mit je einem Commit:** Bei einem Fehler wäre klar gewesen, welcher Schritt ihn verursacht hat.

**Was hat es erschwert?**
- **Hardwareabhängigkeit:** Code in den `main.cpp` (UART, Servo) ist nicht automatisch testbar, und es war kein Pico verfügbar. Dort konnte nur geprüft werden, dass die Firmware kompiliert. Deshalb wurde R-19 bewusst nicht umgesetzt.
- **Einzelarbeit:** Ohne zweite Person fehlt beim Review der fremde Blick, und Pair Programming mit Rollenwechsel war nicht möglich.
- **Abhängigkeit zu abgegebenen Dokumenten:** Das Testprotokoll verweist auf Dateien (R-17), deshalb konnte nicht alles aufgeräumt werden.
- Ein Werkzeugproblem: Ein Schritt (R-02) wurde zuerst nur teilweise übernommen. Die Tests waren trotzdem grün, weil der alte und der neue Code dasselbe taten. Das ist bei einem Refactoring korrekt, zeigt aber: Grüne Tests beweisen nicht, dass das Refactoring vollständig ist. Deshalb wurde mit `grep` geprüft, dass keine Magic Strings mehr übrig sind.
