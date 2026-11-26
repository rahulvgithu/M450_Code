#pragma once
// Gemeinsame Hardware-Adapter fuer Pico 1 (Kasse) und Pico 2 (Schranke).
// Echte Implementierungen der Schnittstellen aus lib/Parksystem.
#include <Arduino.h>
#include <Schnittstellen.h>

namespace pico {

const unsigned long USB_BAUD = 115200;                // serieller Monitor
const unsigned long UART_BAUD = 9600;                 // Verbindung Pico 1 <-> Pico 2, auf beiden gleich
const int UART_TX_PIN = 0;                            // GP0
const int UART_RX_PIN = 1;                            // GP1
const unsigned long WARTEZEIT_SERIAL_MONITOR_MS = 2000; // Zeit, um den Monitor nach dem Start zu oeffnen

// USB-Monitor und UART zum anderen Pico starten
inline void starteSchnittstellen() {
    Serial.begin(USB_BAUD);
    Serial1.setTX(UART_TX_PIN);
    Serial1.setRX(UART_RX_PIN);
    Serial1.begin(UART_BAUD);
    delay(WARTEZEIT_SERIAL_MONITOR_MS);
}

} // namespace pico

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
