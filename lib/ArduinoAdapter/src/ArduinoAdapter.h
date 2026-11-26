#pragma once
// Gemeinsame Hardware-Adapter fuer Pico 1 (Kasse) und Pico 2 (Schranke).
// Echte Implementierungen der Schnittstellen aus lib/Parksystem.
#include <Arduino.h>
#include <Schnittstellen.h>

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
