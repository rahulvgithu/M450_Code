#include "KasseLogik.h"
#include "KassenProtokoll.h"
#include "Kennzeichen.h"
#include "Tarif.h"
#include "Text.h"
#include "Version.h"

KasseLogik::KasseLogik(IUhr &uhr, IParkSpeicher &speicher, IAnzeige &anzeige)
    : uhr_(uhr), speicher_(speicher), anzeige_(anzeige), wanduhr_(uhr) {}

Parkvorgang *KasseLogik::finde(const std::string &kz) const {
    return speicher_.finde(kennzeichen::normalisiere(kz));
}

// ------------------------------------------------------------
// Zeit
// ------------------------------------------------------------

std::string KasseLogik::zeitText(unsigned long ms) const { return wanduhr_.text(ms); }

void KasseLogik::uhrStellen(const std::string &argument) {
    std::string fehler = wanduhr_.stellen(argument);
    anzeige_.zeige(fehler.empty() ? "OK: Uhr gestellt auf " + zeitText(uhr_.jetztMs()) : fehler);
}

void KasseLogik::protokolliere(const std::string &t) {
    std::string eintrag = "[" + zeitText(uhr_.jetztMs()) + "] " + t;
    log_.push_back(eintrag);
    if ((int)log_.size() > MAX_LOG) log_.pop_front();
    anzeige_.zeige("LOG " + eintrag);
}

// ------------------------------------------------------------
// Einfahrt / Ausfahrt (Anfragen von Pico 2)
// ------------------------------------------------------------

std::string KasseLogik::einfahrt(const std::string &eingabe) {
    std::string kz = kennzeichen::normalisiere(eingabe);
    if (kz.empty()) return "Kennzeichen nicht erkannt";
    if (!kennzeichen::istGueltig(kz)) {
        protokolliere("EINFAHRT ABGELEHNT " + kz + " (ungueltiges Kennzeichen)");
        return "Ungueltiges Kennzeichen";
    }
    if (speicher_.finde(kz) != nullptr) {
        protokolliere("EINFAHRT ABGELEHNT " + kz + " (bereits im Parkhaus)");
        return "Fahrzeug bereits im Parkhaus";
    }
    Parkvorgang neu;
    neu.kennzeichen = kz;
    neu.einfahrtMs = uhr_.jetztMs();
    int platz = 0;
    if (!speicher_.hinzufuegen(neu, platz)) {
        protokolliere("EINFAHRT ABGELEHNT " + kz + " (Parkhaus voll)");
        return "Parkhaus voll";
    }
    protokolliere("EINFAHRT " + kz + " Platz " + text::zahl(platz) + " (" + text::zahl(speicher_.anzahl()) + "/" +
                  text::zahl(speicher_.kapazitaet()) + " belegt)");
    return "";
}

std::string KasseLogik::ausfahrt(const std::string &eingabe) {
    std::string kz = kennzeichen::normalisiere(eingabe);
    Parkvorgang *p = speicher_.finde(kz);
    if (p == nullptr) {
        protokolliere("AUSFAHRT ABGELEHNT " + kz + " (kein aktiver Parkvorgang)");
        return "Kein aktiver Parkvorgang";
    }
    if (!p->bezahlt) {
        protokolliere("AUSFAHRT ABGELEHNT " + kz + " (unbezahlt)");
        return "Bitte zuerst an der Kasse bezahlen (" + text::betrag(gebuehrRappen(kz)) + ")";
    }
    Parkvorgang abgeschlossen = *p;
    abgeschlossen.ausfahrtMs = uhr_.jetztMs();
    speicher_.entfernen(kz);
    historie_.push_back(abgeschlossen);
    if ((int)historie_.size() > MAX_HISTORIE) historie_.pop_front();
    protokolliere("AUSFAHRT " + kz + " abgeschlossen");
    return "";
}

std::string KasseLogik::verarbeiteAnfrage(const std::string &eingabe) {
    std::string zeile = text::trim(eingabe);
    if (zeile.empty()) return "";
    if (zeile == protokoll::PING) return protokoll::PONG;
    if (zeile == protokoll::HILFE) {
        protokolliere("HILFERUF von Pico 2 (Schranke)");
        anzeige_.zeige("!!! HILFERUF AN DER SCHRANKE !!!");
        return protokoll::okAntwort(protokoll::HILFE);
    }
    if (protokoll::istAnfrage(zeile, protokoll::EINFAHRT)) {
        std::string kz = kennzeichen::normalisiere(protokoll::argument(zeile, protokoll::EINFAHRT));
        std::string fehler = einfahrt(kz);
        return fehler.empty() ? protokoll::okAntwort(protokoll::EINFAHRT, kz)
                              : protokoll::fehlerAntwort(protokoll::EINFAHRT, fehler);
    }
    if (protokoll::istAnfrage(zeile, protokoll::AUSFAHRT)) {
        std::string kz = kennzeichen::normalisiere(protokoll::argument(zeile, protokoll::AUSFAHRT));
        std::string fehler = ausfahrt(kz);
        return fehler.empty() ? protokoll::okAntwort(protokoll::AUSFAHRT, kz)
                              : protokoll::fehlerAntwort(protokoll::AUSFAHRT, fehler);
    }
    return std::string(protokoll::FEHLER) + " Unbekannte Anfrage";
}

// ------------------------------------------------------------
// Kasse: Gebuehr, Zahlung, Liste
// ------------------------------------------------------------

unsigned long KasseLogik::gebuehrRappen(const std::string &kz) const {
    Parkvorgang *p = finde(kz);
    if (p == nullptr) return 0;
    return p->bezahlt ? p->betragRappen : aktuelleGebuehr(*p);
}

unsigned long KasseLogik::aktuelleGebuehr(const Parkvorgang &p) const {
    return tarif::gebuehrRappen(uhr_.jetztMs() - p.einfahrtMs);
}

Parkvorgang *KasseLogik::findeOderMelde(const std::string &kz) {
    Parkvorgang *p = finde(kz);
    if (p == nullptr) anzeige_.zeige("FEHLER: Kein aktiver Parkvorgang.");
    return p;
}

Parkvorgang *KasseLogik::findeUnbezahltOderMelde(const std::string &kz) {
    Parkvorgang *p = findeOderMelde(kz);
    if (p != nullptr && p->bezahlt) {
        anzeige_.zeige("INFO: Bereits bezahlt.");
        return nullptr;
    }
    return p;
}

void KasseLogik::gebuehrAnzeigen(const std::string &kz) {
    Parkvorgang *p = findeOderMelde(kz);
    if (p == nullptr) return;
    unsigned long dauerMs = uhr_.jetztMs() - p->einfahrtMs;
    anzeige_.zeige("Kennzeichen:    " + p->kennzeichen);
    anzeige_.zeige("Einfahrt:       " + zeitText(p->einfahrtMs));
    anzeige_.zeige("Parkdauer:      " + text::zahl(dauerMs / 1000UL) + " Sekunden");
    anzeige_.zeige("Gebuehr:        " + text::betrag(gebuehrRappen(kz)));
    anzeige_.zeige(std::string("Zahlungsstatus: ") + (p->bezahlt ? "BEZAHLT" : "UNBEZAHLT"));
}

void KasseLogik::bezahlen(const std::string &kz) {
    Parkvorgang *p = findeUnbezahltOderMelde(kz);
    if (p == nullptr) return;
    p->betragRappen = aktuelleGebuehr(*p);
    p->bezahlt = true;
    anzeige_.zeige("Zahlungsmethode: Karte");
    anzeige_.zeige("ZAHLUNG ERFOLGREICH: " + text::betrag(p->betragRappen));
    protokolliere("ZAHLUNG " + p->kennzeichen + " " + text::betrag(p->betragRappen) + " erfolgreich");
}

void KasseLogik::zahlungAbbrechen(const std::string &kz) {
    Parkvorgang *p = findeUnbezahltOderMelde(kz);
    if (p == nullptr) return;
    anzeige_.zeige("ZAHLUNG FEHLGESCHLAGEN: Vorgang abgebrochen. Status bleibt UNBEZAHLT.");
    protokolliere("ZAHLUNG " + p->kennzeichen + " fehlgeschlagen/abgebrochen");
}

void KasseLogik::listeAnzeigen() {
    anzeige_.zeige("Aktive Parkvorgaenge (" + text::zahl(speicher_.anzahl()) + "/" +
                   text::zahl(speicher_.kapazitaet()) + "):");
    for (int i = 1; i <= speicher_.kapazitaet(); i++) {
        const Parkvorgang *p = speicher_.platz(i);
        if (p == nullptr) continue;
        anzeige_.zeige("  Platz " + text::zahl(i) + ": " + p->kennzeichen + " | Einfahrt " + zeitText(p->einfahrtMs) +
                       " | IM PARKHAUS | " + (p->bezahlt ? "BEZAHLT" : "UNBEZAHLT"));
    }
}

void KasseLogik::protokollAnzeigen() {
    anzeige_.zeige("Abgeschlossene Parkvorgaenge:");
    for (const Parkvorgang &p : historie_) {
        anzeige_.zeige("  " + p.kennzeichen + " | Einfahrt " + zeitText(p.einfahrtMs) + " | Ausfahrt " +
                       zeitText(p.ausfahrtMs) + " | " + text::betrag(p.betragRappen) + " | ABGESCHLOSSEN");
    }
    anzeige_.zeige("Ereignisprotokoll:");
    for (const std::string &e : log_) anzeige_.zeige("  " + e);
}

void KasseLogik::hilfe() {
    anzeige_.zeige("");
    anzeige_.zeige(std::string("=== PARKSYSTEM: PICO 1 KASSE v") + parksystem::VERSION + " ===");
    anzeige_.zeige("T 05.11.2026 13:30:00 - Datum/Uhrzeit stellen");
    anzeige_.zeige("G LU12345 - Gebuehr anzeigen");
    anzeige_.zeige("B LU12345 - Zahlung mit Karte (erfolgreich)");
    anzeige_.zeige("X LU12345 - Zahlung abbrechen / fehlgeschlagen");
    anzeige_.zeige("L         - Aktive Parkvorgaenge anzeigen");
    anzeige_.zeige("P         - Historie und Ereignisprotokoll");
    anzeige_.zeige("H         - Hilfe");
    anzeige_.zeige("Ein- und Ausfahrt erfolgen ueber Pico 2 (Schranke).");
    anzeige_.zeige("");
}

void KasseLogik::start() {
    anzeige_.zeige("");
    anzeige_.zeige("********************************");
    anzeige_.zeige("PARKSYSTEM WIRD GESTARTET");
    anzeige_.zeige(std::string("PICO 1 - KASSE v") + parksystem::VERSION);
    anzeige_.zeige("********************************");
    hilfe();
}

void KasseLogik::befehl(const std::string &eingabe) {
    std::string zeile = text::trim(eingabe);
    if (zeile.empty()) return;
    char b = (char)std::toupper((unsigned char)zeile[0]);
    std::string argument = text::trim(zeile.substr(1));
    switch (b) {
        case 'T': uhrStellen(argument); break;
        case 'G': gebuehrAnzeigen(argument); break;
        case 'B': bezahlen(argument); break;
        case 'X': zahlungAbbrechen(argument); break;
        case 'L': listeAnzeigen(); break;
        case 'P': protokollAnzeigen(); break;
        case 'H': hilfe(); break;
        default:
            anzeige_.zeige("FEHLER: Unbekannter Befehl.");
            anzeige_.zeige("Gib H fuer Hilfe ein.");
            break;
    }
}
