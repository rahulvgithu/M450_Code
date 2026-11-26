#include "Kennzeichen.h"
#include "Text.h"

namespace kennzeichen {

static const char *KANTONE[] = {
    "AG", "AI", "AR", "BE", "BL", "BS", "FR", "GE", "GL", "GR", "JU", "LU", "NE",
    "NW", "OW", "SG", "SH", "SO", "SZ", "TG", "TI", "UR", "VD", "VS", "ZG", "ZH"};

std::string normalisiere(const std::string &eingabe) {
    std::string ergebnis;
    for (char c : text::gross(text::trim(eingabe))) {
        if (c != ' ' && c != '-') ergebnis += c;
    }
    return ergebnis;
}

static const size_t KANTON_LAENGE = 2;
static const size_t MIN_ZIFFERN = 1;
static const size_t MAX_ZIFFERN = 6;

static bool istKanton(const std::string &kuerzel) {
    for (const char *k : KANTONE) {
        if (kuerzel == k) return true;
    }
    return false;
}

static bool sindNurZiffern(const std::string &s) {
    for (char c : s) {
        if (c < '0' || c > '9') return false;
    }
    return true;
}

bool istGueltig(const std::string &kz) {
    if (kz.size() < KANTON_LAENGE + MIN_ZIFFERN || kz.size() > KANTON_LAENGE + MAX_ZIFFERN) return false;
    std::string nummer = kz.substr(KANTON_LAENGE);
    return istKanton(kz.substr(0, KANTON_LAENGE)) && nummer[0] != '0' && sindNurZiffern(nummer);
}

} // namespace kennzeichen
