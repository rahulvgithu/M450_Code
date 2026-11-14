
#include <Arduino.h>

// ========================================
// PARKSYSTEM - PICO 1 KASSE
// ========================================

const int MAX_PARKPLAETZE = 10;

// Tarife in Rappen
const unsigned long STARTGEBUEHR_RAPPEN = 100; // CHF 1.00
const unsigned long PREIS_PRO_MINUTE_RAPPEN = 10; // CHF 0.10

struct Parkvorgang {
    String kennzeichen;
    unsigned long einfahrtszeit;
    bool aktiv;
    bool bezahlt;
};

Parkvorgang parkplaetze[MAX_PARKPLAETZE];

// ========================================
// FAHRZEUG SUCHEN
// ========================================

int findeFahrzeug(String kennzeichen) {
    kennzeichen.trim();
    kennzeichen.toUpperCase();

    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        if (parkplaetze[i].aktiv &&
            parkplaetze[i].kennzeichen == kennzeichen) {
            return i;
        }
    }

    return -1;
}

// ========================================
// FAHRZEUG REGISTRIEREN
// ========================================

void registrieren(String kennzeichen) {
    kennzeichen.trim();
    kennzeichen.toUpperCase();

    if (kennzeichen.length() == 0) {
        Serial.println("FEHLER: Kennzeichen fehlt.");
        return;
    }

    if (findeFahrzeug(kennzeichen) != -1) {
        Serial.println("FEHLER: Fahrzeug bereits registriert.");
        return;
    }

    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        if (!parkplaetze[i].aktiv) {
            parkplaetze[i].kennzeichen = kennzeichen;
            parkplaetze[i].einfahrtszeit = millis();
            parkplaetze[i].aktiv = true;
            parkplaetze[i].bezahlt = false;

            Serial.print("OK: Fahrzeug registriert: ");
            Serial.println(kennzeichen);
            return;
        }
    }

    Serial.println("FEHLER: Keine freien Parkplaetze.");
}

// ========================================
// PARKGEBUEHR BERECHNEN
// ========================================

void gebuehrAnzeigen(String kennzeichen) {
    int index = findeFahrzeug(kennzeichen);

    if (index == -1) {
        Serial.println("FEHLER: Kein aktiver Parkvorgang.");
        return;
    }

    unsigned long vergangeneMillis =
        millis() - parkplaetze[index].einfahrtszeit;

    // Angefangene Minuten werden aufgerundet
    unsigned long minuten =
        (vergangeneMillis + 59999UL) / 60000UL;

    unsigned long betrag =
        STARTGEBUEHR_RAPPEN +
        minuten * PREIS_PRO_MINUTE_RAPPEN;

    Serial.print("Kennzeichen: ");
    Serial.println(parkplaetze[index].kennzeichen);

    Serial.print("Parkdauer: ");
    Serial.print(vergangeneMillis / 1000UL);
    Serial.println(" Sekunden");

    Serial.print("Gebuehr: CHF ");
    Serial.print(betrag / 100UL);
    Serial.print(".");
    if (betrag % 100UL < 10) {
        Serial.print("0");
    }
    Serial.println(betrag % 100UL);

    Serial.print("Zahlungsstatus: ");
    Serial.println(parkplaetze[index].bezahlt
                   ? "BEZAHLT" : "UNBEZAHLT");
}

// ========================================
// ZAHLUNG SIMULIEREN
// ========================================

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

    parkplaetze[index].bezahlt = true;
    Serial.println("ZAHLUNG ERFOLGREICH (SIMULATION).");
}

// ========================================
// AUSFAHRT ABSCHLIESSEN
// ========================================

void ausfahrt(String kennzeichen) {
    int index = findeFahrzeug(kennzeichen);

    if (index == -1) {
        Serial.println("FEHLER: Kein aktiver Parkvorgang.");
        return;
    }

    if (!parkplaetze[index].bezahlt) {
        Serial.println("ABGELEHNT: Bitte zuerst bezahlen.");
        return;
    }

    parkplaetze[index].aktiv = false;

    Serial.println("OK: Parkvorgang abgeschlossen.");
    Serial.println("Fahrzeug darf ausfahren.");
}

// ========================================
// HILFE
// ========================================

void hilfe() {
    Serial.println();
    Serial.println("=== PARKSYSTEM: PICO 1 KASSE ===");
    Serial.println("E LU12345 - Einfahrt registrieren");
    Serial.println("G LU12345 - Gebuehr anzeigen");
    Serial.println("B LU12345 - Zahlung simulieren");
    Serial.println("A LU12345 - Ausfahrt abschliessen");
    Serial.println("H         - Hilfe");
    Serial.println();
}

// ========================================
// START
// ========================================

void setup() {
    Serial.begin(115200);

    // Warten, damit der Serial Monitor geöffnet werden kann
    delay(2000);

    for (int i = 0; i < MAX_PARKPLAETZE; i++) {
        parkplaetze[i].kennzeichen = "";
        parkplaetze[i].einfahrtszeit = 0;
        parkplaetze[i].aktiv = false;
        parkplaetze[i].bezahlt = false;
    }

    Serial.println();
    Serial.println("********************************");
    Serial.println("PARKSYSTEM WIRD GESTARTET");
    Serial.println("PICO 1 - KASSE");
    Serial.println("********************************");

    hilfe();
}

// ========================================
// HAUPTSCHLEIFE
// ========================================

void loop() {
    if (!Serial.available()) {
        return;
    }

    String eingabe = Serial.readStringUntil('\n');
    eingabe.trim();

    if (eingabe.length() == 0) {
        return;
    }

    eingabe.toUpperCase();

    char befehl = eingabe.charAt(0);

    String kennzeichen = "";
    if (eingabe.length() > 1) {
        kennzeichen = eingabe.substring(1);
        kennzeichen.trim();
    }

    switch (befehl) {
        case 'E':
            registrieren(kennzeichen);
            break;

        case 'G':
            gebuehrAnzeigen(kennzeichen);
            break;

        case 'B':
            bezahlen(kennzeichen);
            break;

        case 'A':
            ausfahrt(kennzeichen);
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
