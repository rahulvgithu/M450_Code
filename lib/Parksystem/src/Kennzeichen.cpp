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

bool istGueltig(const std::string &kz) {
    if (kz.size() < 3 || kz.size() > 8) return false;
    std::string kanton = kz.substr(0, 2);
    bool kantonOk = false;
    for (const char *k : KANTONE) {
        if (kanton == k) {
            kantonOk = true;
            break;
        }
    }
    if (!kantonOk || kz[2] == '0') return false;
    for (size_t i = 2; i < kz.size(); i++) {
        if (kz[i] < '0' || kz[i] > '9') return false;
    }
    return true;
}

} // namespace kennzeichen
