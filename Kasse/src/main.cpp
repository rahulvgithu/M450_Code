#include <Arduino.h>

// ========================================
// PARKSYSTEM - PICO 1 KASSE
// Version 1.1
//
// Aenderungen gegenueber Version 1.0 (siehe Testprotokoll):
//  F-001  Kommunikation mit Pico 2 ueber UART (Serial1)
//  F-002  Uhrzeit stellen (T), Einfahrtszeit anzeigen, Liste (L)
//  F-003  Pruefung des Kennzeichenformats (Kanton + Nummer)
//  F-004  Fehlgeschlagene / abgebrochene Zahlung (X)
//  F-005  Ausfahrtszeit, Historie und Ereignisprotokoll (P)
//  F-007  Hilferuf von Pico 2 wird angezeigt und protokolliert
//
// Verkabelung Pico 1 <-> Pico 2 (gekreuzt):
//  Pico 1 GP0 (TX)  ->  Pico 2 GP1 (RX)
//  Pico 1 GP1 (RX)  <-  Pico 2 GP0 (TX)
//  Pico 1 GND       --  Pico 2 GND
// ========================================

const char *VERSION = "1.1";

const int MAX_PARKPLAETZE = 10;
const int MAX_HISTORIE = 20;
const int MAX_LOG = 30;

const unsigned long UART_BAUD = 9600;

// Tarife in Rappen
const unsigned long STARTGEBUEHR_RAPPEN = 100;    // CHF 1.00
const unsigned long PREIS_PRO_MINUTE_RAPPEN = 10; // CHF 0.10

struct Parkvorgang {
    String kennzeichen;
    unsigned long einfahrtMs;
    unsigned long ausfahrtMs;
    unsigned long betragRappen;
    bool aktiv;
    bool bezahlt;
};

Parkvorgang parkplaetze[MAX_PARKPLAETZE];

Parkvorgang historie[MAX_HISTORIE];
int historieAnzahl = 0;
int historieNaechster = 0;

String logbuch[MAX_LOG];
int logAnzahl = 0;
int logNaechster = 0;

// Uhr (wird mit dem Befehl T gesetzt)
bool uhrGesetzt = false;
unsigned long uhrBasisSek = 0; // Sekunden seit Mitternacht
unsigned long uhrBasisMs = 0;  // millis() beim Stellen
String datum = "";

String uartPuffer = "";

const char *KANTONE[] = {
    "AG", "AI", "AR", "BE", "BL", "BS", "FR", "GE", "GL", "GR", "JU", "LU", "NE",
    "NW", "OW", "SG", "SH", "SO", "SZ", "TG", "TI", "UR", "VD", "VS", "ZG", "ZH"};
const int ANZAHL_KANTONE = sizeof(KANTONE) / sizeof(KANTONE[0]);

// ========================================
// HILFSFUNKTIONEN
// ========================================

String zweistellig(unsigned long wert) {
    return (wert < 10 ? "0" : "") + String(wert);
}

String zeitText(unsigned long ms) {
    if (!uhrGesetzt) {
        return String(ms / 1000UL) + " s nach Start";
    }
    unsigned long sek = (uhrBasisSek + (ms - uhrBasisMs) / 1000UL) % 86400UL;
    String text = zweistellig(sek / 3600UL) + ":" +
                  zweistellig((sek / 60UL) % 60UL) + ":" +
                  zweistellig(sek % 60UL);
    if (datum.length() > 0) {
        text = datum + " " + text;
    }
    return text;
}

String betragText(unsigned long rappen) {
    return "CHF " + String(rappen / 100UL) + "." + zweistellig(rappen % 100UL);
}

unsigned long berechneGebuehr(unsigned long dauerMs) {
    // Angefangene Minuten werden aufgerundet
    unsigned long minuten = (dauerMs + 59999UL) / 60000UL;
    return STARTGEBUEHR_RAPPEN + minuten * PREIS_PRO_MINUTE_RAPPEN;
}

void protokolliere(String text) {
    String eintrag = "[" + zeitText(millis()) + "] " + text;
    logbuch[logNaechster] = eintrag;
    logNaechster = (logNaechster + 1) % MAX_LOG;
    if (logAnzahl < MAX_LOG) {
        logAnzahl++;
    }
    Serial.println("LOG " + eintrag);
}

// Entfernt Leerzeichen und Bindestriche, Grossschreibung
String normalisiere(String kennzeichen) {
    kennzeichen.trim();
    kennzeichen.toUpperCase();
    kennzeichen.replace(" ", "");
    kennzeichen.replace("-", "");
    return kennzeichen;
}

// Gueltig: Kantonskuerzel + 1 bis 6 Ziffern, erste Ziffer nicht 0
bool istGueltig(const String &kz) {
    if (kz.length() < 3 || kz.length() > 8) {
        return false;
    }
    String kanton = kz.substring(0, 2);
    bool kantonOk = false;
    for (int i = 0; i < ANZAHL_KANTONE; i++) {
        if (kanton == KANTONE[i]) {
            kantonOk = true;
            break;
        }
    }
    if (!kantonOk || kz.charAt(2) == '0') {
        return false;
    }
    for (unsigned int i = 2; i < kz.length(); i++) {
        if (!isDigit(kz.charAt(i))) {
            return false;
        }
    }
    return true;
}

int findeFahrzeug(String kennzeichen) {
    kennzeichen = normalisiere(kennzeichen);
    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        if (parkplaetze[i].aktiv && parkplaetze[i].kennzeichen == kennzeichen) {
            return i;
        }
    }
    return -1;
}

int belegtePlaetze() {
    int anzahl = 0;
    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        if (parkplaetze[i].aktiv) {
            anzahl++;
        }
    }
    return anzahl;
}

// ========================================
// EINFAHRT (Anfrage von Pico 2)
// Rueckgabe: leer = OK, sonst Fehlertext
// ========================================

String registrieren(String kennzeichen) {
    kennzeichen = normalisiere(kennzeichen);

    if (kennzeichen.length() == 0) {
        return "Kennzeichen nicht erkannt";
    }
    if (!istGueltig(kennzeichen)) {
        protokolliere("EINFAHRT ABGELEHNT " + kennzeichen + " (ungueltiges Kennzeichen)");
        return "Ungueltiges Kennzeichen";
    }
    if (findeFahrzeug(kennzeichen) != -1) {
        protokolliere("EINFAHRT ABGELEHNT " + kennzeichen + " (bereits im Parkhaus)");
        return "Fahrzeug bereits im Parkhaus";
    }
    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        if (!parkplaetze[i].aktiv) {
            parkplaetze[i].kennzeichen = kennzeichen;
            parkplaetze[i].einfahrtMs = millis();
            parkplaetze[i].ausfahrtMs = 0;
            parkplaetze[i].betragRappen = 0;
            parkplaetze[i].aktiv = true;
            parkplaetze[i].bezahlt = false;
            protokolliere("EINFAHRT " + kennzeichen + " Platz " + String(i + 1) +
                          " (" + String(belegtePlaetze()) + "/" + String(MAX_PARKPLAETZE) + " belegt)");
            return "";
        }
    }
    protokolliere("EINFAHRT ABGELEHNT " + kennzeichen + " (Parkhaus voll)");
    return "Parkhaus voll";
}

// ========================================
// AUSFAHRT (Anfrage von Pico 2)
// Rueckgabe: leer = OK, sonst Fehlertext
// ========================================

String ausfahrt(String kennzeichen) {
    kennzeichen = normalisiere(kennzeichen);
    int index = findeFahrzeug(kennzeichen);

    if (index == -1) {
        protokolliere("AUSFAHRT ABGELEHNT " + kennzeichen + " (kein aktiver Parkvorgang)");
        return "Kein aktiver Parkvorgang";
    }
    if (!parkplaetze[index].bezahlt) {
        unsigned long betrag = berechneGebuehr(millis() - parkplaetze[index].einfahrtMs);
        protokolliere("AUSFAHRT ABGELEHNT " + kennzeichen + " (unbezahlt)");
        return "Bitte zuerst an der Kasse bezahlen (" + betragText(betrag) + ")";
    }

    parkplaetze[index].ausfahrtMs = millis();
    parkplaetze[index].aktiv = false;

    historie[historieNaechster] = parkplaetze[index];
    historieNaechster = (historieNaechster + 1) % MAX_HISTORIE;
    if (historieAnzahl < MAX_HISTORIE) {
        historieAnzahl++;
    }

    protokolliere("AUSFAHRT " + kennzeichen + " abgeschlossen");
    return "";
}

// ========================================
// KASSE: GEBUEHR, ZAHLUNG, LISTE
// ========================================

void gebuehrAnzeigen(String kennzeichen) {
    int index = findeFahrzeug(kennzeichen);
    if (index == -1) {
        Serial.println("FEHLER: Kein aktiver Parkvorgang.");
        return;
    }
    Parkvorgang &p = parkplaetze[index];
    unsigned long dauerMs = millis() - p.einfahrtMs;

    Serial.println("Kennzeichen:    " + p.kennzeichen);
    Serial.println("Einfahrt:       " + zeitText(p.einfahrtMs));
    Serial.println("Parkdauer:      " + String(dauerMs / 1000UL) + " Sekunden");
    Serial.println("Gebuehr:        " + betragText(p.bezahlt ? p.betragRappen : berechneGebuehr(dauerMs)));
    Serial.println(String("Zahlungsstatus: ") + (p.bezahlt ? "BEZAHLT" : "UNBEZAHLT"));
}

void bezahlen(String kennzeichen) {
    int index = findeFahrzeug(kennzeichen);
    if (index == -1) {
        Serial.println("FEHLER: Kein aktiver Parkvorgang.");
        return;
    }
    if (parkplaetze[index].bezahlt) {
        Serial.println("INFO: Bereits bezahlt.");
        return;
    }
    unsigned long betrag = berechneGebuehr(millis() - parkplaetze[index].einfahrtMs);
    parkplaetze[index].betragRappen = betrag;
    parkplaetze[index].bezahlt = true;
    Serial.println("Zahlungsmethode: Karte");
    Serial.println("ZAHLUNG ERFOLGREICH: " + betragText(betrag));
    protokolliere("ZAHLUNG " + parkplaetze[index].kennzeichen + " " + betragText(betrag) + " erfolgreich");
}

void zahlungAbbrechen(String kennzeichen) {
    int index = findeFahrzeug(kennzeichen);
    if (index == -1) {
        Serial.println("FEHLER: Kein aktiver Parkvorgang.");
        return;
    }
    if (parkplaetze[index].bezahlt) {
        Serial.println("INFO: Bereits bezahlt.");
        return;
    }
    Serial.println("ZAHLUNG FEHLGESCHLAGEN: Vorgang abgebrochen. Status bleibt UNBEZAHLT.");
    protokolliere("ZAHLUNG " + parkplaetze[index].kennzeichen + " fehlgeschlagen/abgebrochen");
}

void listeAnzeigen() {
    Serial.println("Aktive Parkvorgaenge (" + String(belegtePlaetze()) + "/" + String(MAX_PARKPLAETZE) + "):");
    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        if (parkplaetze[i].aktiv) {
            Serial.println("  Platz " + String(i + 1) + ": " + parkplaetze[i].kennzeichen +
                           " | Einfahrt " + zeitText(parkplaetze[i].einfahrtMs) +
                           " | IM PARKHAUS | " + (parkplaetze[i].bezahlt ? "BEZAHLT" : "UNBEZAHLT"));
        }
    }
}

void protokollAnzeigen() {
    Serial.println("Abgeschlossene Parkvorgaenge:");
    for (int i = 0; i < historieAnzahl; i++) {
        int idx = (historieNaechster - historieAnzahl + i + MAX_HISTORIE) % MAX_HISTORIE;
        Parkvorgang &p = historie[idx];
        Serial.println("  " + p.kennzeichen + " | Einfahrt " + zeitText(p.einfahrtMs) +
                       " | Ausfahrt " + zeitText(p.ausfahrtMs) + " | " +
                       betragText(p.betragRappen) + " | ABGESCHLOSSEN");
    }
    Serial.println("Ereignisprotokoll:");
    for (int i = 0; i < logAnzahl; i++) {
        int idx = (logNaechster - logAnzahl + i + MAX_LOG) % MAX_LOG;
        Serial.println("  " + logbuch[idx]);
    }
}

// Format: T HH:MM:SS  oder  T TT.MM.JJJJ HH:MM:SS
void uhrStellen(String argument) {
    argument.trim();
    String zeit = argument;
    String neuesDatum = "";
    int leer = argument.indexOf(' ');
    if (leer > 0) {
        neuesDatum = argument.substring(0, leer);
        zeit = argument.substring(leer + 1);
        zeit.trim();
    }
    if (zeit.length() != 8 || zeit.charAt(2) != ':' || zeit.charAt(5) != ':') {
        Serial.println("FEHLER: Format T HH:MM:SS oder T TT.MM.JJJJ HH:MM:SS");
        return;
    }
    unsigned long h = zeit.substring(0, 2).toInt();
    unsigned long m = zeit.substring(3, 5).toInt();
    unsigned long s = zeit.substring(6, 8).toInt();
    if (h > 23 || m > 59 || s > 59) {
        Serial.println("FEHLER: Ungueltige Uhrzeit.");
        return;
    }
    uhrBasisSek = h * 3600UL + m * 60UL + s;
    uhrBasisMs = millis();
    uhrGesetzt = true;
    if (neuesDatum.length() > 0) {
        datum = neuesDatum;
    }
    Serial.println("OK: Uhr gestellt auf " + zeitText(millis()));
}

void hilfe() {
    Serial.println();
    Serial.println("=== PARKSYSTEM: PICO 1 KASSE v" + String(VERSION) + " ===");
    Serial.println("T 05.11.2026 13:30:00 - Datum/Uhrzeit stellen");
    Serial.println("G LU12345 - Gebuehr anzeigen");
    Serial.println("B LU12345 - Zahlung mit Karte (erfolgreich)");
    Serial.println("X LU12345 - Zahlung abbrechen / fehlgeschlagen");
    Serial.println("L         - Aktive Parkvorgaenge anzeigen");
    Serial.println("P         - Historie und Ereignisprotokoll");
    Serial.println("H         - Hilfe");
    Serial.println("Ein- und Ausfahrt erfolgen ueber Pico 2 (Schranke).");
    Serial.println();
}

// ========================================
// UART: ANFRAGEN VON PICO 2
// ========================================

void verarbeiteUart(String zeile) {
    zeile.trim();
    if (zeile.length() == 0) {
        return;
    }
    if (zeile == "PING") {
        Serial1.println("PONG");
        return;
    }
    if (zeile == "HELP") {
        protokolliere("HILFERUF von Pico 2 (Schranke)");
        Serial.println("!!! HILFERUF AN DER SCHRANKE !!!");
        Serial1.println("OK HELP");
        return;
    }
    if (zeile.startsWith("REG")) {
        String kz = normalisiere(zeile.substring(3));
        String fehler = registrieren(kz);
        Serial1.println(fehler.length() == 0 ? "OK REG " + kz : "ERR REG " + fehler);
        return;
    }
    if (zeile.startsWith("EXIT")) {
        String kz = normalisiere(zeile.substring(4));
        String fehler = ausfahrt(kz);
        Serial1.println(fehler.length() == 0 ? "OK EXIT " + kz : "ERR EXIT " + fehler);
        return;
    }
    Serial1.println("ERR Unbekannte Anfrage");
}

void leseUart() {
    while (Serial1.available()) {
        char c = Serial1.read();
        if (c == '\n') {
            verarbeiteUart(uartPuffer);
            uartPuffer = "";
        } else if (c != '\r' && uartPuffer.length() < 64) {
            uartPuffer += c;
        }
    }
}

// ========================================
// START
// ========================================

void setup() {
    Serial.begin(115200);

    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(UART_BAUD);

    // Warten, damit der Serial Monitor geoeffnet werden kann
    delay(2000);

    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        parkplaetze[i].kennzeichen = "";
        parkplaetze[i].einfahrtMs = 0;
        parkplaetze[i].ausfahrtMs = 0;
        parkplaetze[i].betragRappen = 0;
        parkplaetze[i].aktiv = false;
        parkplaetze[i].bezahlt = false;
    }

    Serial.println();
    Serial.println("********************************");
    Serial.println("PARKSYSTEM WIRD GESTARTET");
    Serial.println("PICO 1 - KASSE v" + String(VERSION));
    Serial.println("********************************");

    hilfe();
}

// ========================================
// HAUPTSCHLEIFE
// ========================================

void loop() {
    leseUart();

    if (!Serial.available()) {
        return;
    }

    String eingabe = Serial.readStringUntil('\n');
    eingabe.trim();
    if (eingabe.length() == 0) {
        return;
    }

    char befehl = toupper(eingabe.charAt(0));
    String argument = eingabe.length() > 1 ? eingabe.substring(1) : "";
    argument.trim();

    switch (befehl) {
        case 'T':
            uhrStellen(argument);
            break;
        case 'G':
            gebuehrAnzeigen(argument);
            break;
        case 'B':
            bezahlen(argument);
            break;
        case 'X':
            zahlungAbbrechen(argument);
            break;
        case 'L':
            listeAnzeigen();
            break;
        case 'P':
            protokollAnzeigen();
            break;
        case 'H':
            hilfe();
            break;
        default:
            Serial.println("FEHLER: Unbekannter Befehl.");
            Serial.println("Gib H fuer Hilfe ein.");
            break;
    }
}
