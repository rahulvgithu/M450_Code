#include <Arduino.h>
#include <Servo.h>

// ========================================
// PARKSYSTEM - PICO 2 SCHRANKE
// Version 1.1
//
// Aenderungen gegenueber Version 1.0 (siehe Testprotokoll):
//  F-001  Einfahrt/Ausfahrt fragen Pico 1 ueber UART an,
//         Schranke oeffnet nur nach Freigabe durch die Kasse.
//         Timeout 2 s: ohne Antwort bleibt die Schranke zu.
//  F-006  Freies Oeffnen mit Befehl "O" entfernt
//  F-007  Hilferuf (R) an die Kasse (US10)
//
// Verkabelung:
//  Servo Einfahrt: Signal GP8, Servo Ausfahrt: Signal GP9 (5 V an VBUS)
//  Pico 2 GP0 (TX) -> Pico 1 GP1 (RX)
//  Pico 2 GP1 (RX) <- Pico 1 GP0 (TX)
//  GND -- GND
// ========================================

const char *VERSION = "1.1";

const int SERVO_EIN_PIN = 8;
const int SERVO_AUS_PIN = 9;

const int WINKEL_GESCHLOSSEN = 0;
const int WINKEL_OFFEN = 90;

const unsigned long UART_BAUD = 9600;
const unsigned long ANTWORT_TIMEOUT_MS = 2000;
const unsigned long OFFEN_DAUER_MS = 5000; // simulierte Durchfahrt

struct Schranke {
    const char *name;
    Servo servo;
    bool offen;
    unsigned long geoeffnetMs;
};

Schranke einfahrtSchranke = {"Einfahrt", Servo(), false, 0};
Schranke ausfahrtSchranke = {"Ausfahrt", Servo(), false, 0};

void oeffne(Schranke &s) {
    s.servo.write(WINKEL_OFFEN);
    s.offen = true;
    s.geoeffnetMs = millis();
    Serial.println(String(s.name) + "sschranke OFFEN");
}

void schliesse(Schranke &s) {
    s.servo.write(WINKEL_GESCHLOSSEN);
    if (s.offen) {
        Serial.println(String(s.name) + "sschranke GESCHLOSSEN");
    }
    s.offen = false;
}

void automatischSchliessen(Schranke &s) {
    if (s.offen && millis() - s.geoeffnetMs >= OFFEN_DAUER_MS) {
        Serial.println("Durchfahrt erfolgt.");
        schliesse(s);
    }
}

// Sendet eine Anfrage an Pico 1 und wartet auf die Antwort.
// Rueckgabe: Antwortzeile oder "" bei Timeout.
String anfrage(String text) {
    while (Serial1.available()) {
        Serial1.read();
    }
    Serial1.println(text);

    String antwort = "";
    unsigned long start = millis();
    while (millis() - start < ANTWORT_TIMEOUT_MS) {
        if (Serial1.available()) {
            char c = Serial1.read();
            if (c == '\n') {
                antwort.trim();
                return antwort;
            }
            if (c != '\r') {
                antwort += c;
            }
        }
    }
    return "";
}

String normalisiere(String kennzeichen) {
    kennzeichen.trim();
    kennzeichen.toUpperCase();
    kennzeichen.replace(" ", "");
    kennzeichen.replace("-", "");
    return kennzeichen;
}

void bearbeite(bool istEinfahrt, String kennzeichen) {
    Schranke &s = istEinfahrt ? einfahrtSchranke : ausfahrtSchranke;
    kennzeichen = normalisiere(kennzeichen);

    if (kennzeichen.length() == 0) {
        Serial.println("FEHLER: Kennzeichen nicht erkannt. Bitte erneut eingeben oder mit R Hilfe rufen.");
        Serial.println(String(s.name) + "sschranke bleibt GESCHLOSSEN");
        return;
    }

    String antwort = anfrage((istEinfahrt ? "REG " : "EXIT ") + kennzeichen);

    if (antwort.length() == 0) {
        Serial.println("FEHLER: Keine Verbindung zur Kasse (Pico 1). Bitte Personal rufen.");
        Serial.println(String(s.name) + "sschranke bleibt GESCHLOSSEN");
        return;
    }
    if (antwort.startsWith("OK")) {
        Serial.println(istEinfahrt ? "Willkommen " + kennzeichen + "."
                                   : "Gute Fahrt " + kennzeichen + ".");
        oeffne(s);
        return;
    }

    // Antwort "ERR REG <Text>" bzw. "ERR EXIT <Text>"
    String grund = antwort;
    int pos = grund.indexOf(' ', 4);
    if (pos > 0) {
        grund = grund.substring(pos + 1);
    }
    Serial.println("ABGELEHNT: " + grund + ".");
    Serial.println(String(s.name) + "sschranke bleibt GESCHLOSSEN");
}

void status() {
    Serial.println(String("Einfahrtsschranke: ") + (einfahrtSchranke.offen ? "OFFEN" : "GESCHLOSSEN"));
    Serial.println(String("Ausfahrtsschranke: ") + (ausfahrtSchranke.offen ? "OFFEN" : "GESCHLOSSEN"));
    String antwort = anfrage("PING");
    Serial.println(String("Verbindung Kasse:  ") + (antwort == "PONG" ? "OK" : "KEINE VERBINDUNG"));
}

void hilfe() {
    Serial.println();
    Serial.println("=== PARKSYSTEM: PICO 2 SCHRANKE v" + String(VERSION) + " ===");
    Serial.println("E LU12345 - Einfahrt (Kennzeichen erfassen)");
    Serial.println("A LU12345 - Ausfahrt");
    Serial.println("S         - Beide Schranken schliessen");
    Serial.println("Z         - Status Schranken und Verbindung");
    Serial.println("R         - Hilfe vom Personal anfordern");
    Serial.println("H         - Hilfe");
    Serial.println();
}

void setup() {
    Serial.begin(115200);

    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(UART_BAUD);

    delay(2000);

    einfahrtSchranke.servo.attach(SERVO_EIN_PIN);
    ausfahrtSchranke.servo.attach(SERVO_AUS_PIN);
    schliesse(einfahrtSchranke);
    schliesse(ausfahrtSchranke);
    delay(1000);

    Serial.println("PARKSYSTEM - PICO 2 SCHRANKE v" + String(VERSION));
    hilfe();
}

void loop() {
    automatischSchliessen(einfahrtSchranke);
    automatischSchliessen(ausfahrtSchranke);

    if (Serial.available() == 0) {
        return;
    }

    String eingabe = Serial.readStringUntil('\n');
    eingabe.trim();
    if (eingabe.length() == 0) {
        return;
    }

    char befehl = toupper(eingabe.charAt(0));
    String argument = eingabe.length() > 1 ? eingabe.substring(1) : "";

    switch (befehl) {
        case 'E':
            bearbeite(true, argument);
            break;
        case 'A':
            bearbeite(false, argument);
            break;
        case 'S':
            schliesse(einfahrtSchranke);
            schliesse(ausfahrtSchranke);
            Serial.println("Beide Schranken GESCHLOSSEN");
            break;
        case 'Z':
            status();
            break;
        case 'R':
            Serial.println(anfrage("HELP") == "OK HELP"
                               ? "Hilferuf gesendet. Das Personal wurde informiert."
                               : "Keine Verbindung zur Kasse. Bitte Notruftaste benutzen.");
            break;
        case 'H':
            hilfe();
            break;
        default:
            Serial.println("Unbekannter Befehl. H fuer Hilfe.");
            break;
    }
}
