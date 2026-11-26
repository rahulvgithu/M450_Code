#pragma once
#include <string>
#include <vector>
#include <KasseLogik.h>
#include <KassenProtokoll.h>
#include <Schnittstellen.h>

// SPY fuer IKasse
// Umschliesst die ECHTE KasseLogik von Pico 1 und das echte Textprotokoll.
// Nur das UART-Kabel faellt weg. Jede Anfrage wird aufgezeichnet.
// Der Test bleibt damit ein Integrationstest (Pico 2 + Pico 1) mit
// zusaetzlicher Pruefung, welche Anfragen gesendet wurden.
class KasseSpy : public IKasse {
public:
    explicit KasseSpy(KasseLogik &echteKasse) : kasse_(echteKasse) {}

    KassenAntwort einfahrt(const std::string &kz) override {
        return protokoll::auswerten(senden(protokoll::anfrage(protokoll::EINFAHRT, kz)));
    }
    KassenAntwort ausfahrt(const std::string &kz) override {
        return protokoll::auswerten(senden(protokoll::anfrage(protokoll::AUSFAHRT, kz)));
    }
    bool verbunden() override { return senden(protokoll::PING) == protokoll::PONG; }
    bool hilfeRufen() override { return senden(protokoll::HILFE) == protokoll::okAntwort(protokoll::HILFE); }

    std::vector<std::string> anfragen;  // was Pico 2 gesendet hat
    std::vector<std::string> antworten; // was Pico 1 geantwortet hat

private:
    std::string senden(const std::string &zeile) {
        anfragen.push_back(zeile);
        std::string antwort = kasse_.verarbeiteAnfrage(zeile);
        antworten.push_back(antwort);
        return antwort;
    }

    KasseLogik &kasse_;
};
