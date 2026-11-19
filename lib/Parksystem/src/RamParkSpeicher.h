#pragma once
#include "Schnittstellen.h"

// Produktive Speicherung: feste Anzahl Plaetze im Arbeitsspeicher.
template <int KAPAZITAET>
class RamParkSpeicher : public IParkSpeicher {
public:
    bool hinzufuegen(const Parkvorgang &vorgang, int &platzNr) override {
        for (int i = 0; i < KAPAZITAET; i++) {
            if (!belegt[i]) {
                plaetze[i] = vorgang;
                belegt[i] = true;
                platzNr = i + 1;
                return true;
            }
        }
        return false;
    }

    Parkvorgang *finde(const std::string &kz) override {
        for (int i = 0; i < KAPAZITAET; i++) {
            if (belegt[i] && plaetze[i].kennzeichen == kz) return &plaetze[i];
        }
        return nullptr;
    }

    bool entfernen(const std::string &kz) override {
        for (int i = 0; i < KAPAZITAET; i++) {
            if (belegt[i] && plaetze[i].kennzeichen == kz) {
                belegt[i] = false;
                plaetze[i] = Parkvorgang();
                return true;
            }
        }
        return false;
    }

    int anzahl() const override {
        int n = 0;
        for (int i = 0; i < KAPAZITAET; i++) n += belegt[i] ? 1 : 0;
        return n;
    }

    int kapazitaet() const override { return KAPAZITAET; }

    const Parkvorgang *platz(int nummer) const override {
        if (nummer < 1 || nummer > KAPAZITAET || !belegt[nummer - 1]) return nullptr;
        return &plaetze[nummer - 1];
    }

private:
    Parkvorgang plaetze[KAPAZITAET];
    bool belegt[KAPAZITAET] = {};
};
