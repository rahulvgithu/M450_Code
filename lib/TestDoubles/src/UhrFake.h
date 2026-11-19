#pragma once
#include <Schnittstellen.h>

// FAKE fuer IUhr (ersetzt millis())
// Vereinfachte, aber funktionierende Uhr, die nur weiterlaeuft, wenn der Test
// es sagt. So kann eine Parkdauer von 10 Minuten in 0 ms "gewartet" werden und
// das Ergebnis ist immer gleich (deterministisch).
class UhrFake : public IUhr {
public:
    explicit UhrFake(unsigned long startMs = 0) : jetzt_(startMs) {}
    unsigned long jetztMs() const override { return jetzt_; }

    void vorspulen(unsigned long ms) { jetzt_ += ms; }
    void vorspulenSekunden(unsigned long s) { jetzt_ += s * 1000UL; }
    void setze(unsigned long ms) { jetzt_ = ms; }

private:
    unsigned long jetzt_;
};
