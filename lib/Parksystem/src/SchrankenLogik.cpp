#include "SchrankenLogik.h"
#include "Kennzeichen.h"
#include "Text.h"
#include "Version.h"


SchrankenLogik::SchrankenLogik(ISchranke &einfahrt, ISchranke &ausfahrt, IKasse &kasse, IUhr &uhr,
                               IAnzeige &anzeige)
    : einfahrt_{"Einfahrtsschranke", Richtung::Einfahrt, "Willkommen", einfahrt, false, 0},
      ausfahrt_{"Ausfahrtsschranke", Richtung::Ausfahrt, "Gute Fahrt", ausfahrt, false, 0},
      kasse_(kasse),
      uhr_(uhr),
      anzeige_(anzeige) {}

void SchrankenLogik::start() {
    einfahrt_.schranke.schliessen();
    ausfahrt_.schranke.schliessen();
    einfahrt_.offen = ausfahrt_.offen = false;
    anzeige_.zeige(std::string("PARKSYSTEM - PICO 2 SCHRANKE v") + parksystem::VERSION);
    hilfe();
}

void SchrankenLogik::oeffne(GesteuerteSchranke &s) {
    s.schranke.oeffnen();
    s.offen = true;
    s.geoeffnetMs = uhr_.jetztMs();
    anzeige_.zeige(std::string(s.name) + " OFFEN");
}

void SchrankenLogik::schliesse(GesteuerteSchranke &s) {
    s.schranke.schliessen();
    if (s.offen) anzeige_.zeige(std::string(s.name) + " GESCHLOSSEN");
    s.offen = false;
}

void SchrankenLogik::meldeBleibtGeschlossen(const GesteuerteSchranke &s) {
    anzeige_.zeige(std::string(s.name) + " bleibt GESCHLOSSEN");
}

void SchrankenLogik::aktualisieren() {
    for (GesteuerteSchranke *s : {&einfahrt_, &ausfahrt_}) {
        if (s->offen && uhr_.jetztMs() - s->geoeffnetMs >= OFFEN_DAUER_MS) {
            anzeige_.zeige("Durchfahrt erfolgt.");
            schliesse(*s);
        }
    }
}

KassenAntwort SchrankenLogik::frageKasse(const GesteuerteSchranke &s, const std::string &kz) {
    return s.richtung == Richtung::Einfahrt ? kasse_.einfahrt(kz) : kasse_.ausfahrt(kz);
}

void SchrankenLogik::bearbeite(GesteuerteSchranke &s, const std::string &eingabe) {
    std::string kz = kennzeichen::normalisiere(eingabe);
    if (kz.empty()) {
        anzeige_.zeige("FEHLER: Kennzeichen nicht erkannt. Bitte erneut eingeben oder mit R Hilfe rufen.");
        meldeBleibtGeschlossen(s);
        return;
    }
    KassenAntwort antwort = frageKasse(s, kz);
    switch (antwort.status) {
        case KassenAntwort::OK:
            anzeige_.zeige(std::string(s.begruessung) + " " + kz + ".");
            oeffne(s);
            break;
        case KassenAntwort::ABGELEHNT:
            anzeige_.zeige("ABGELEHNT: " + antwort.grund + ".");
            meldeBleibtGeschlossen(s);
            break;
        case KassenAntwort::KEINE_VERBINDUNG:
            anzeige_.zeige("FEHLER: Keine Verbindung zur Kasse (Pico 1). Bitte Personal rufen.");
            meldeBleibtGeschlossen(s);
            break;
    }
}

void SchrankenLogik::fahrzeugAnEinfahrt(const std::string &kz) { bearbeite(einfahrt_, kz); }
void SchrankenLogik::fahrzeugAnAusfahrt(const std::string &kz) { bearbeite(ausfahrt_, kz); }

void SchrankenLogik::status() {
    anzeige_.zeige(std::string("Einfahrtsschranke: ") + (einfahrt_.offen ? "OFFEN" : "GESCHLOSSEN"));
    anzeige_.zeige(std::string("Ausfahrtsschranke: ") + (ausfahrt_.offen ? "OFFEN" : "GESCHLOSSEN"));
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

void SchrankenLogik::alleSchliessen() {
    schliesse(einfahrt_);
    schliesse(ausfahrt_);
    anzeige_.zeige("Beide Schranken GESCHLOSSEN");
}

void SchrankenLogik::hilfeRufen() {
    anzeige_.zeige(kasse_.hilfeRufen() ? "Hilferuf gesendet. Das Personal wurde informiert."
                                       : "Keine Verbindung zur Kasse. Bitte Notruftaste benutzen.");
}

void SchrankenLogik::befehl(const std::string &eingabe) {
    text::Befehl b;
    if (!text::zerlegeBefehl(eingabe, b)) return;
    switch (b.buchstabe) {
        case 'E': fahrzeugAnEinfahrt(b.argument); break;
        case 'A': fahrzeugAnAusfahrt(b.argument); break;
        case 'S': alleSchliessen(); break;
        case 'Z': status(); break;
        case 'R': hilfeRufen(); break;
        case 'H': hilfe(); break;
        default: anzeige_.zeige("Unbekannter Befehl. H fuer Hilfe."); break;
    }
}
