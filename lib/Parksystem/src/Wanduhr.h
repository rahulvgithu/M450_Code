#pragma once
// Uhrzeit und Datum fuer Anzeige und Protokoll.
// Der Pico hat keine Echtzeituhr: Die Uhr wird einmal gestellt (Befehl T)
// und laeuft danach mit der Laufzeit (IUhr) weiter.
#include <string>
#include "Schnittstellen.h"

class Wanduhr {
public:
    explicit Wanduhr(const IUhr &laufzeit) : laufzeit_(laufzeit) {}

    // Eingabe "HH:MM:SS" oder "TT.MM.JJJJ HH:MM:SS".
    // Rueckgabe: leer = gestellt, sonst Fehlermeldung.
    std::string stellen(const std::string &eingabe);

    // "14.11.2026 13:36:12", oder "65 s nach Start", solange die Uhr nicht gestellt ist
    std::string text(unsigned long laufzeitMs) const;

private:
    static bool zerlegeUhrzeit(const std::string &zeit, unsigned long &sekundenSeitMitternacht, std::string &fehler);

    const IUhr &laufzeit_;
    bool gestellt_ = false;
    unsigned long sekundenBeimStellen_ = 0; // Sekunden seit Mitternacht
    unsigned long laufzeitBeimStellenMs_ = 0;
    std::string datum_;
};
