# M450 – Automatisiertes Parksystem

**Gruppenmitglied:** Rahul Vaikunthavasan (Einzelarbeit, INA24cL)
**Modul:** M450 – Applikationen testen
**Aktuelle Version:** 1.1 (nach Testdurchführung und Fehlerbehebung, siehe Testprotokoll)

## Kurze Beschreibung der Umsetzung

Das Parksystem besteht aus zwei Raspberry Pi Pico W, programmiert mit Arduino (PlatformIO, Core *earlephilhower*).

| Komponente | Ordner | Aufgabe |
|---|---|---|
| Pico 1 – Kasse | [`Kasse`](Kasse/src/main.cpp) | Verwaltet die Parkvorgänge (max. 10 Plätze), prüft Kennzeichen, berechnet die Gebühr, verarbeitet Zahlungen, führt Historie und Ereignisprotokoll |
| Pico 2 – Schranke | [`Parkhaus_Schranke`](Parkhaus_Schranke/src/main.cpp) | Terminal an Ein- und Ausfahrt: erfasst das Kennzeichen, fragt die Kasse an und steuert die beiden Schranken (Servo SG90) |

Ablauf: Einfahrt (Kennzeichen an Pico 2) → Pico 1 prüft und registriert → Einfahrtsschranke öffnet → Gebühr anzeigen und bezahlen an Pico 1 → Ausfahrt (Kennzeichen an Pico 2) → Pico 1 prüft Zahlung und schliesst den Parkvorgang ab → Ausfahrtsschranke öffnet. Die Schranken schliessen nach 5 Sekunden (Durchfahrt) automatisch.

Die Kennzeichenerkennung wird, wie in der Aufgabenstellung vorgesehen, durch die Eingabe im seriellen Monitor ersetzt (keine Kamera).

### Tarif

CHF 1.00 Grundgebühr + CHF 0.10 pro angefangene Minute.

### Verkabelung

| Verbindung | Pico 1 (Kasse) | Pico 2 (Schranke) |
|---|---|---|
| UART TX → RX | GP0 (Pin 1) | GP1 (Pin 2) |
| UART RX ← TX | GP1 (Pin 2) | GP0 (Pin 1) |
| Masse | GND | GND |
| Servo Einfahrt (Signal) | – | GP8 (Pin 11) |
| Servo Ausfahrt (Signal) | – | GP9 (Pin 12) |
| Servo VCC / GND | – | VBUS (5 V) / GND |

UART: 9600 Baud. USB-Seriell (Serial Monitor): 115200 Baud.

### Bedienung

**Pico 1 – Kasse**

| Befehl | Funktion |
|---|---|
| `T 05.11.2026 13:30:00` | Datum und Uhrzeit stellen (nach jedem Start) |
| `G LU12345` | Einfahrtszeit, Parkdauer, Gebühr und Zahlungsstatus anzeigen |
| `B LU12345` | Zahlung mit Karte (erfolgreich) |
| `X LU12345` | Zahlung abbrechen / fehlgeschlagen |
| `L` | Aktive Parkvorgänge anzeigen |
| `P` | Abgeschlossene Parkvorgänge und Ereignisprotokoll |
| `H` | Hilfe |

**Pico 2 – Schranke**

| Befehl | Funktion |
|---|---|
| `E LU12345` | Einfahrt |
| `A LU12345` | Ausfahrt |
| `S` | Beide Schranken schliessen |
| `Z` | Status der Schranken und der Verbindung zur Kasse |
| `R` | Hilfe vom Personal anfordern |
| `H` | Hilfe |

Gültige Kennzeichen: Kantonskürzel + 1 bis 6 Ziffern (z. B. `LU12345`, `ZH54321`). Leerzeichen werden ignoriert. `XX00000` wird abgelehnt.

## Umgesetzte User Stories / Anforderungen

| User Story | Umsetzung | Status |
|---|---|---|
| US01 – Fahrzeug bei der Einfahrt erkennen | Kennzeichen an Pico 2, Speicherung mit Datum/Uhrzeit auf Pico 1 | umgesetzt |
| US02 – Einfahrt mit gültigem Kennzeichen | Einfahrtsschranke öffnet nach Freigabe durch Pico 1 und schliesst nach 5 s | umgesetzt |
| US03 – Ungültige Kennzeichen ablehnen | Formatprüfung, Meldung, Eintrag im Ereignisprotokoll | umgesetzt |
| US04 – Parkvorgang registrieren | Parkvorgang mit Kennzeichen, Einfahrtszeit, Status (Liste `L`) | umgesetzt |
| US05 – Parkgebühr berechnen | Tarif pro angefangene Minute (`G`) | umgesetzt |
| US06 – Parkgebühr bezahlen | Zahlung mit Karte (`B`), fehlgeschlagene Zahlung (`X`) | umgesetzt |
| US07 – Ausfahrt nach erfolgreicher Zahlung | Pico 2 fragt Zahlungsstatus bei Pico 1 ab, Ausfahrtsschranke öffnet | umgesetzt |
| US08 – Unbezahlte Ausfahrt verhindern | Ausfahrt abgelehnt mit Meldung und offenem Betrag | umgesetzt |
| US09 – Parkvorgang bei der Ausfahrt abschliessen | Ausfahrtszeit, Historie (`P`) | umgesetzt |
| US10 – Fehler bei der Kennzeichenerkennung behandeln | Meldung bei fehlendem Kennzeichen, Hilferuf (`R`), Timeout bei Verbindungsunterbruch | umgesetzt |

### Bekannte Einschränkungen

- Bedienung über den seriellen Monitor statt über eine Weboberfläche.
- Parkdaten werden nur im Arbeitsspeicher gehalten und gehen bei einem Neustart verloren.
- Die Uhrzeit muss nach jedem Start mit `T` gestellt werden (kein RTC).
- Bricht die Verbindung genau nach der Freigabe einer Ausfahrt ab, ist der Parkvorgang abgeschlossen, die Schranke öffnet aber nicht (Hilferuf nötig). Geplant für Version 1.2.

## Versionen

| Version | Datum | Änderungen |
|---|---|---|
| 1.0 | 04.11.2026 | Erste Umsetzung: Kasse mit Registrierung, Gebühr, Zahlung, Ausfahrt; Schranke manuell (O/S) |
| 1.1 | 14.11.2026 | Fehlerbehebung nach Testdurchführung: UART-Kommunikation (F-001), Uhrzeit und Liste (F-002), Kennzeichenprüfung (F-003), fehlgeschlagene Zahlung (F-004), Ausfahrtszeit und Historie (F-005), freies Öffnen entfernt (F-006), Hilferuf (F-007), Gebühr nach Zahlung fixiert (F-008) |

Der Code von Version 1.0 liegt jeweils im Ordner `backup_v1.0`.
