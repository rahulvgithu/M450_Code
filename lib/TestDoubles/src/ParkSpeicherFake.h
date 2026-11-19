#pragma once
#include <map>
#include <string>
#include <Schnittstellen.h>

// FAKE fuer IParkSpeicher
// Vereinfachte Speicherung in einer std::map statt fester Plaetze im RAM
// (spaeter evtl. Flash). Die Kapazitaet ist frei waehlbar und der Test kann
// einen Zustand direkt vorbereiten, z. B. "LU12345 steht seit 2 Stunden im
// Parkhaus und hat bezahlt", ohne den ganzen Ablauf durchzuspielen.
class ParkSpeicherFake : public IParkSpeicher {
public:
    explicit ParkSpeicherFake(int kapazitaet = 10) : kapazitaet_(kapazitaet) {}

    bool hinzufuegen(const Parkvorgang &vorgang, int &platz) override {
        for (int nr = 1; nr <= kapazitaet_; nr++) {
            if (plaetze_.count(nr) == 0) {
                plaetze_[nr] = vorgang;
                platz = nr;
                return true;
            }
        }
        return false;
    }

    Parkvorgang *finde(const std::string &kz) override {
        for (auto &e : plaetze_) {
            if (e.second.kennzeichen == kz) return &e.second;
        }
        return nullptr;
    }

    bool entfernen(const std::string &kz) override {
        for (auto it = plaetze_.begin(); it != plaetze_.end(); ++it) {
            if (it->second.kennzeichen == kz) {
                plaetze_.erase(it);
                return true;
            }
        }
        return false;
    }

    int anzahl() const override { return (int)plaetze_.size(); }
    int kapazitaet() const override { return kapazitaet_; }

    const Parkvorgang *platz(int nummer) const override {
        auto it = plaetze_.find(nummer);
        return it == plaetze_.end() ? nullptr : &it->second;
    }

    // Testhilfe: Zustand direkt herstellen
    void vorbereiten(const std::string &kz, unsigned long einfahrtMs, bool bezahlt = false,
                     unsigned long betragRappen = 0) {
        Parkvorgang p;
        p.kennzeichen = kz;
        p.einfahrtMs = einfahrtMs;
        p.bezahlt = bezahlt;
        p.betragRappen = betragRappen;
        int nr;
        hinzufuegen(p, nr);
    }

private:
    int kapazitaet_;
    std::map<int, Parkvorgang> plaetze_;
};
