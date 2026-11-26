#pragma once
// ============================================================
// Logik von Pico 2 (Schranke). Keine Hardware-Aufrufe:
// Schranken (Servo), Kasse (UART), Uhr und Anzeige werden
// ueber den Konstruktor uebergeben (Dependency Injection).
// ============================================================
#include <string>
#include "Schnittstellen.h"

class SchrankenLogik {
public:
    static const unsigned long OFFEN_DAUER_MS = 5000; // Durchfahrt

    SchrankenLogik(ISchranke &einfahrt, ISchranke &ausfahrt, IKasse &kasse, IUhr &uhr, IAnzeige &anzeige);

    void start();

    // Kennzeichen wurde erkannt (heute: Eingabe im Terminal, spaeter Kamera)
    void fahrzeugAnEinfahrt(const std::string &kennzeichen);
    void fahrzeugAnAusfahrt(const std::string &kennzeichen);

    // Eine Zeile vom Terminal (E, A, S, Z, R, H)
    void befehl(const std::string &eingabe);

    // Regelmaessig aus loop() aufrufen: schliesst offene Schranken nach der Durchfahrt
    void aktualisieren();

    bool einfahrtOffen() const { return einfahrt_.offen; }
    bool ausfahrtOffen() const { return ausfahrt_.offen; }

private:
    enum class Richtung { Einfahrt, Ausfahrt };

    struct GesteuerteSchranke {
        const char *name;        // "Einfahrtsschranke"
        Richtung richtung;
        const char *begruessung; // "Willkommen" / "Gute Fahrt"
        ISchranke &schranke;
        bool offen;
        unsigned long geoeffnetMs;
    };

    void bearbeite(GesteuerteSchranke &s, const std::string &kennzeichen);
    KassenAntwort frageKasse(const GesteuerteSchranke &s, const std::string &kennzeichen);
    void oeffne(GesteuerteSchranke &s);
    void schliesse(GesteuerteSchranke &s);
    void meldeBleibtGeschlossen(const GesteuerteSchranke &s);
    void status();
    void hilfe();

    GesteuerteSchranke einfahrt_;
    GesteuerteSchranke ausfahrt_;
    IKasse &kasse_;
    IUhr &uhr_;
    IAnzeige &anzeige_;
};
