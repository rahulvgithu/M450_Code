#pragma once
#include <string>
#include <vector>
#include <Schnittstellen.h>

// DUMMY fuer IAnzeige (ersetzt Serial.println())
// Platzhalter fuer Tests, in denen die Meldungen keine Rolle spielen.
// Er wird nur uebergeben, weil der Konstruktor eine Anzeige verlangt.
class AnzeigeDummy : public IAnzeige {
public:
    void zeige(const std::string &) override {}
};

// MOCK fuer IAnzeige
// Merkt sich jede ausgegebene Zeile (wie oft, mit welchem Text), damit der
// Test pruefen kann, ob der Fahrer die richtige Meldung erhalten hat.
class AnzeigeMock : public IAnzeige {
public:
    void zeige(const std::string &text) override { zeilen.push_back(text); }

    bool enthaelt(const std::string &teil) const {
        for (const auto &z : zeilen) {
            if (z.find(teil) != std::string::npos) return true;
        }
        return false;
    }

    int anzahl(const std::string &teil) const {
        int n = 0;
        for (const auto &z : zeilen) {
            if (z.find(teil) != std::string::npos) n++;
        }
        return n;
    }

    void leeren() { zeilen.clear(); }

    std::vector<std::string> zeilen;
};
