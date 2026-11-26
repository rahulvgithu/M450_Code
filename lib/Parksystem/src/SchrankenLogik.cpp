#include "SchrankenLogik.h"
#include "Kennzeichen.h"
#include "Text.h"
#include "Version.h"


SchrankenLogik::SchrankenLogik(ISchranke &einfahrt, ISchranke &ausfahrt, IKasse &kasse, IUhr &uhr,
                               IAnzeige &anzeige)
    : ein_{"Einfahrt", einfahrt, false, 0},
      aus_{"Ausfahrt", ausfahrt, false, 0},
      kasse_(kasse),
      uhr_(uhr),
      anzeige_(anzeige) {}

void SchrankenLogik::start() {
    ein_.schranke.schliessen();
    aus_.schranke.schliessen();
    ein_.offen = aus_.offen = false;
    anzeige_.zeige(std::string("PARKSYSTEM - PICO 2 SCHRANKE v") + parksystem::VERSION);
    hilfe();
}

void SchrankenLogik::oeffne(Zustand &s) {
    s.schranke.oeffnen();
    s.offen = true;
    s.geoeffnetMs = uhr_.jetztMs();
    anzeige_.zeige(std::string(s.name) + "sschranke OFFEN");
}

void SchrankenLogik::schliesse(Zustand &s) {
    s.schranke.schliessen();
    if (s.offen) anzeige_.zeige(std::string(s.name) + "sschranke GESCHLOSSEN");
    s.offen = false;
}

void SchrankenLogik::aktualisieren() {
    for (Zustand *s : {&ein_, &aus_}) {
        if (s->offen && uhr_.jetztMs() - s->geoeffnetMs >= OFFEN_DAUER_MS) {
            anzeige_.zeige("Durchfahrt erfolgt.");
            schliesse(*s);
        }
    }
}

void SchrankenLogik::bearbeite(Zustand &s, bool istEinfahrt, const std::string &eingabe) {
    std::string kz = kennzeichen::normalisiere(eingabe);
    if (kz.empty()) {
        anzeige_.zeige("FEHLER: Kennzeichen nicht erkannt. Bitte erneut eingeben oder mit R Hilfe rufen.");
        anzeige_.zeige(std::string(s.name) + "sschranke bleibt GESCHLOSSEN");
        return;
    }
    KassenAntwort antwort = istEinfahrt ? kasse_.einfahrt(kz) : kasse_.ausfahrt(kz);
    switch (antwort.status) {
        case KassenAntwort::OK:
            anzeige_.zeige((istEinfahrt ? "Willkommen " : "Gute Fahrt ") + kz + ".");
            oeffne(s);
            break;
        case KassenAntwort::ABGELEHNT:
            anzeige_.zeige("ABGELEHNT: " + antwort.grund + ".");
            anzeige_.zeige(std::string(s.name) + "sschranke bleibt GESCHLOSSEN");
            break;
        case KassenAntwort::KEINE_VERBINDUNG:
            anzeige_.zeige("FEHLER: Keine Verbindung zur Kasse (Pico 1). Bitte Personal rufen.");
            anzeige_.zeige(std::string(s.name) + "sschranke bleibt GESCHLOSSEN");
            break;
    }
}

void SchrankenLogik::fahrzeugAnEinfahrt(const std::string &kz) { bearbeite(ein_, true, kz); }
void SchrankenLogik::fahrzeugAnAusfahrt(const std::string &kz) { bearbeite(aus_, false, kz); }

void SchrankenLogik::status() {
    anzeige_.zeige(std::string("Einfahrtsschranke: ") + (ein_.offen ? "OFFEN" : "GESCHLOSSEN"));
    anzeige_.zeige(std::string("Ausfahrtsschranke: ") + (aus_.offen ? "OFFEN" : "GESCHLOSSEN"));
    anzeige_.zeige(std::string("Verbindung Kasse:  ") + (kasse_.verbunden() ? "OK" : "KEINE VERBINDUNG"));
}

void SchrankenLogik::hilfe() {
    anzeige_.zeige("");
    anzeige_.zeige(std::string("=== PARKSYSTEM: PICO 2 SCHRANKE v") + parksystem::VERSION + " ===");
    anzeige_.zeige("E LU12345 - Einfahrt (Kennzeichen erfassen)");
    anzeige_.zeige("A LU12345 - Ausfahrt");
    anzeige_.zeige("S         - Beide Schranken schliessen");
    anzeige_.zeige("Z         - Status Schranken und Verbindung");
    anzeige_.zeige("R         - Hilfe vom Personal anfordern");
    anzeige_.zeige("H         - Hilfe");
    anzeige_.zeige("");
}

void SchrankenLogik::befehl(const std::string &eingabe) {
    std::string zeile = text::trim(eingabe);
    if (zeile.empty()) return;
    char b = (char)std::toupper((unsigned char)zeile[0]);
    std::string argument = zeile.substr(1);
    switch (b) {
        case 'E': fahrzeugAnEinfahrt(argument); break;
        case 'A': fahrzeugAnAusfahrt(argument); break;
        case 'S':
            schliesse(ein_);
            schliesse(aus_);
            anzeige_.zeige("Beide Schranken GESCHLOSSEN");
            break;
        case 'Z': status(); break;
        case 'R':
            anzeige_.zeige(kasse_.hilfeRufen() ? "Hilferuf gesendet. Das Personal wurde informiert."
                                               : "Keine Verbindung zur Kasse. Bitte Notruftaste benutzen.");
            break;
        case 'H': hilfe(); break;
        default: anzeige_.zeige("Unbekannter Befehl. H fuer Hilfe."); break;
    }
}
