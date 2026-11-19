#pragma once
#include <string>

namespace kennzeichen {

// Grossschreibung, Leerzeichen und Bindestriche entfernen: "lu 12345" -> "LU12345"
std::string normalisiere(const std::string &eingabe);

// Gueltig: Schweizer Kantonskuerzel + 1 bis 6 Ziffern, erste Ziffer nicht 0
bool istGueltig(const std::string &normalisiert);

} // namespace kennzeichen
