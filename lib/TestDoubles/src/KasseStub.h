#pragma once
#include <string>
#include <Schnittstellen.h>

// STUB fuer IKasse (ersetzt die UART-Verbindung zu Pico 1)
// Keine Logik, ignoriert das Kennzeichen und liefert immer die fest
// eingestellte Antwort. So kann die Schranke isoliert getestet werden,
// auch fuer schwer herstellbare Faelle wie "keine Verbindung".
class KasseStub : public IKasse {
public:
    KassenAntwort einfahrt(const std::string &) override { return antwortEinfahrt; }
    KassenAntwort ausfahrt(const std::string &) override { return antwortAusfahrt; }
    bool verbunden() override { return istVerbunden; }
    bool hilfeRufen() override { return istVerbunden; }

    static KasseStub erlaubtAlles() {
        return KasseStub({KassenAntwort::OK, ""}, {KassenAntwort::OK, ""}, true);
    }
    static KasseStub lehntAb(const std::string &grund) {
        return KasseStub({KassenAntwort::ABGELEHNT, grund}, {KassenAntwort::ABGELEHNT, grund}, true);
    }
    static KasseStub ohneVerbindung() {
        return KasseStub({KassenAntwort::KEINE_VERBINDUNG, ""}, {KassenAntwort::KEINE_VERBINDUNG, ""}, false);
    }

    KassenAntwort antwortEinfahrt;
    KassenAntwort antwortAusfahrt;
    bool istVerbunden;

private:
    KasseStub(KassenAntwort ein, KassenAntwort aus, bool verbunden)
        : antwortEinfahrt(ein), antwortAusfahrt(aus), istVerbunden(verbunden) {}
};
