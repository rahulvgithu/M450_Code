#include <Arduino.h>
#include <KasseLogik.h>
#include <RamParkSpeicher.h>

// ========================================
// PARKSYSTEM - PICO 1 KASSE  (Version 1.2)
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

const unsigned long UART_BAUD = 9600;
const int MAX_PARKPLAETZE = 10;

// ---------- Adapter: echte Implementierungen der Schnittstellen ----------

// IUhr -> millis()
class MillisUhr : public IUhr {
public:
    unsigned long jetztMs() const override { return millis(); }
};

// IAnzeige -> serieller Monitor (USB)
class SerialAnzeige : public IAnzeige {
public:
    void zeige(const std::string &text) override { Serial.println(text.c_str()); }
};

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
        } else if (c != '\r' && uartPuffer.length() < 64) {
            uartPuffer += c;
        }
    }
}

void setup() {
    Serial.begin(115200);
    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(UART_BAUD);
    delay(2000); // Zeit, um den Serial Monitor zu oeffnen
    kasse.start();
}

void loop() {
    leseUart();
    if (Serial.available()) {
        String eingabe = Serial.readStringUntil('\n');
        kasse.befehl(eingabe.c_str());
    }
}
