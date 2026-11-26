#include <Arduino.h>
#include <ArduinoAdapter.h>
#include <KasseLogik.h>
#include <RamParkSpeicher.h>

// ========================================
// PARKSYSTEM - PICO 1 KASSE  (Version: siehe lib/Parksystem/src/Version.h)
//
// Diese Datei enthaelt nur noch die Hardware-Adapter.
// Die gesamte Logik steht in lib/Parksystem/src/KasseLogik.cpp
// und wird mit Test Doubles automatisiert getestet (test/).
//
// Verkabelung Pico 1 <-> Pico 2 (gekreuzt):
//  Pico 1 GP0 (TX)  ->  Pico 2 GP1 (RX)
//  Pico 1 GP1 (RX)  <-  Pico 2 GP0 (TX)
//  Pico 1 GND       --  Pico 2 GND
// ========================================

const int MAX_PARKPLAETZE = 10;
const unsigned int MAX_ZEILENLAENGE = 64; // laengere UART-Zeilen werden abgeschnitten

MillisUhr uhr;
SerialAnzeige anzeige;
RamParkSpeicher<MAX_PARKPLAETZE> speicher;
KasseLogik kasse(uhr, speicher, anzeige);

String uartPuffer;

void leseUart() {
    while (Serial1.available()) {
        char c = Serial1.read();
        if (c == '\n') {
            std::string antwort = kasse.verarbeiteAnfrage(uartPuffer.c_str());
            if (!antwort.empty()) Serial1.println(antwort.c_str());
            uartPuffer = "";
        } else if (c != '\r' && uartPuffer.length() < MAX_ZEILENLAENGE) {
            uartPuffer += c;
        }
    }
}

void setup() {
    pico::starteSchnittstellen();
    kasse.start();
}

void loop() {
    leseUart();
    if (Serial.available()) {
        String eingabe = Serial.readStringUntil('\n');
        kasse.befehl(eingabe.c_str());
    }
}
