#pragma once
// ============================================================
// Logik von Pico 1 (Kasse). Keine Hardware-Aufrufe:
// Zeit, Ausgabe und Speicherung werden ueber den Konstruktor
// uebergeben (Dependency Injection).
// ============================================================
#include <deque>
#include <string>
#include "Schnittstellen.h"
#include "Wanduhr.h"

class KasseLogik {
public:
    static const int MAX_HISTORIE = 20;
    static const int MAX_LOG = 30;

    KasseLogik(IUhr &uhr, IParkSpeicher &speicher, IAnzeige &anzeige);

    // Startmeldung und Hilfe
    void start();

    // Eine Zeile vom Kassen-Terminal (T, G, B, X, L, P, H)
    void befehl(const std::string &eingabe);

    // Eine Anfrage von Pico 2 (REG, EXIT, PING, HELP). Rueckgabe: Antwortzeile.
    std::string verarbeiteAnfrage(const std::string &zeile);

    // Fachliche Funktionen (Rueckgabe: leer = OK, sonst Fehlertext)
    std::string einfahrt(const std::string &kennzeichen);
    std::string ausfahrt(const std::string &kennzeichen);
    void bezahlen(const std::string &kennzeichen);
    void zahlungAbbrechen(const std::string &kennzeichen);

    // Abfragen fuer Anzeige und Tests
    unsigned long gebuehrRappen(const std::string &kennzeichen) const;
    std::string zeitText(unsigned long ms) const;
    const std::deque<Parkvorgang> &historie() const { return historie_; }
    const std::deque<std::string> &protokoll() const { return log_; }

private:
    void gebuehrAnzeigen(const std::string &kz);
    void listeAnzeigen();
    void protokollAnzeigen();
    void uhrStellen(const std::string &argument);
    void hilfe();
    void protokolliere(const std::string &text);
    Parkvorgang *finde(const std::string &kz) const;
    unsigned long aktuelleGebuehr(const Parkvorgang &vorgang) const;
    Parkvorgang *findeOderMelde(const std::string &kz);
    Parkvorgang *findeUnbezahltOderMelde(const std::string &kz);

    IUhr &uhr_;
    IParkSpeicher &speicher_;
    IAnzeige &anzeige_;
    Wanduhr wanduhr_;

    std::deque<Parkvorgang> historie_;
    std::deque<std::string> log_;
};
