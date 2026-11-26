#include "Wanduhr.h"
#include "Text.h"

static const unsigned long MS_PRO_SEKUNDE = 1000UL;
static const unsigned long SEKUNDEN_PRO_MINUTE = 60UL;
static const unsigned long SEKUNDEN_PRO_STUNDE = 3600UL;
static const unsigned long SEKUNDEN_PRO_TAG = 86400UL;

static const char *FEHLER_FORMAT = "FEHLER: Format T HH:MM:SS oder T TT.MM.JJJJ HH:MM:SS";
static const char *FEHLER_UHRZEIT = "FEHLER: Ungueltige Uhrzeit.";

std::string Wanduhr::stellen(const std::string &eingabe) {
    std::string zeit = text::trim(eingabe);
    std::string neuesDatum;
    size_t leerzeichen = zeit.find(' ');
    if (leerzeichen != std::string::npos && leerzeichen > 0) {
        neuesDatum = zeit.substr(0, leerzeichen);
        zeit = text::trim(zeit.substr(leerzeichen + 1));
    }

    unsigned long sekunden = 0;
    std::string fehler;
    if (!zerlegeUhrzeit(zeit, sekunden, fehler)) return fehler;

    sekundenBeimStellen_ = sekunden;
    laufzeitBeimStellenMs_ = laufzeit_.jetztMs();
    gestellt_ = true;
    if (!neuesDatum.empty()) datum_ = neuesDatum;
    return "";
}

bool Wanduhr::zerlegeUhrzeit(const std::string &zeit, unsigned long &sekundenSeitMitternacht, std::string &fehler) {
    // Format HH:MM:SS
    bool formatOk = zeit.size() == 8 && zeit[2] == ':' && zeit[5] == ':';
    for (int i : {0, 1, 3, 4, 6, 7}) {
        if (formatOk && (zeit[i] < '0' || zeit[i] > '9')) formatOk = false;
    }
    if (!formatOk) {
        fehler = FEHLER_FORMAT;
        return false;
    }
    auto zweiZiffern = [&](int i) { return (unsigned long)((zeit[i] - '0') * 10 + (zeit[i + 1] - '0')); };
    unsigned long stunden = zweiZiffern(0);
    unsigned long minuten = zweiZiffern(3);
    unsigned long sekunden = zweiZiffern(6);
    if (stunden > 23 || minuten > 59 || sekunden > 59) {
        fehler = FEHLER_UHRZEIT;
        return false;
    }
    sekundenSeitMitternacht = stunden * SEKUNDEN_PRO_STUNDE + minuten * SEKUNDEN_PRO_MINUTE + sekunden;
    return true;
}

std::string Wanduhr::text(unsigned long laufzeitMs) const {
    if (!gestellt_) return text::zahl(laufzeitMs / MS_PRO_SEKUNDE) + " s nach Start";
    unsigned long vergangen = (laufzeitMs - laufzeitBeimStellenMs_) / MS_PRO_SEKUNDE;
    unsigned long sek = (sekundenBeimStellen_ + vergangen) % SEKUNDEN_PRO_TAG;
    std::string uhrzeit = text::zweistellig(sek / SEKUNDEN_PRO_STUNDE) + ":" +
                          text::zweistellig((sek / SEKUNDEN_PRO_MINUTE) % 60UL) + ":" +
                          text::zweistellig(sek % SEKUNDEN_PRO_MINUTE);
    return datum_.empty() ? uhrzeit : datum_ + " " + uhrzeit;
}
