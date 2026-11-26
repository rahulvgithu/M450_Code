#include <Arduino.h>
#include <Servo.h>
#include <KassenProtokoll.h>
#include <SchrankenLogik.h>

// ========================================
// PARKSYSTEM - PICO 2 SCHRANKE  (Version: siehe lib/Parksystem/src/Version.h)
//
// Diese Datei enthaelt nur noch die Hardware-Adapter.
// Die gesamte Logik steht in lib/Parksystem/src/SchrankenLogik.cpp
// und wird mit Test Doubles automatisiert getestet (test/).
//
// Verkabelung:
//  Servo Einfahrt: Signal GP8, Servo Ausfahrt: Signal GP9 (5 V an VBUS)
//  Pico 2 GP0 (TX) -> Pico 1 GP1 (RX)
//  Pico 2 GP1 (RX) <- Pico 1 GP0 (TX)
//  GND -- GND
// ========================================

const int SERVO_EIN_PIN = 8;
const int SERVO_AUS_PIN = 9;
const int WINKEL_GESCHLOSSEN = 0;
const int WINKEL_OFFEN = 90;
const unsigned long UART_BAUD = 9600;
const unsigned long ANTWORT_TIMEOUT_MS = 2000;

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

// ISchranke -> Servo SG90
class ServoSchranke : public ISchranke {
public:
    explicit ServoSchranke(int pin) : pin_(pin) {}
    void begin() { servo_.attach(pin_); }
    void oeffnen() override { servo_.write(WINKEL_OFFEN); }
    void schliessen() override { servo_.write(WINKEL_GESCHLOSSEN); }

private:
    int pin_;
    Servo servo_;
};

// IKasse -> UART-Verbindung zu Pico 1
class UartKasse : public IKasse {
public:
    KassenAntwort einfahrt(const std::string &kz) override {
        return protokoll::auswerten(anfrage(protokoll::anfrage(protokoll::EINFAHRT, kz)));
    }
    KassenAntwort ausfahrt(const std::string &kz) override {
        return protokoll::auswerten(anfrage(protokoll::anfrage(protokoll::AUSFAHRT, kz)));
    }
    bool verbunden() override { return anfrage(protokoll::PING) == protokoll::PONG; }
    bool hilfeRufen() override { return anfrage(protokoll::HILFE) == protokoll::okAntwort(protokoll::HILFE); }

private:
    // Sendet eine Zeile und wartet max. 2 s auf die Antwort ("" bei Timeout)
    std::string anfrage(const std::string &text) {
        while (Serial1.available()) Serial1.read();
        Serial1.println(text.c_str());
        std::string antwort;
        unsigned long start = millis();
        while (millis() - start < ANTWORT_TIMEOUT_MS) {
            if (!Serial1.available()) continue;
            char c = Serial1.read();
            if (c == '\n') return antwort;
            if (c != '\r') antwort += c;
        }
        return "";
    }
};

MillisUhr uhr;
SerialAnzeige anzeige;
ServoSchranke einfahrtSchranke(SERVO_EIN_PIN);
ServoSchranke ausfahrtSchranke(SERVO_AUS_PIN);
UartKasse kasse;
SchrankenLogik logik(einfahrtSchranke, ausfahrtSchranke, kasse, uhr, anzeige);

void setup() {
    Serial.begin(115200);
    Serial1.setTX(0);
    Serial1.setRX(1);
    Serial1.begin(UART_BAUD);
    delay(2000);
    einfahrtSchranke.begin();
    ausfahrtSchranke.begin();
    logik.start(); // schliesst beide Schranken
    delay(1000);
}

void loop() {
    logik.aktualisieren();
    if (Serial.available()) {
        String eingabe = Serial.readStringUntil('\n');
        logik.befehl(eingabe.c_str());
    }
}
