#pragma once
// Textprotokoll zwischen Pico 2 (Schranke) und Pico 1 (Kasse) ueber UART.
//   Anfrage:  REG <KZ> | EXIT <KZ> | PING | HELP
//   Antwort:  OK REG <KZ> | OK EXIT <KZ> | ERR REG <Grund> | ERR EXIT <Grund> | PONG | OK HELP
#include "Schnittstellen.h"
#include "Text.h"

namespace protokoll {

// Wandelt die Antwortzeile von Pico 1 in eine KassenAntwort um.
// Leere Antwort (Timeout) = KEINE_VERBINDUNG.
inline KassenAntwort auswerten(const std::string &zeile) {
    std::string z = text::trim(zeile);
    if (z.empty()) return {KassenAntwort::KEINE_VERBINDUNG, ""};
    if (text::beginntMit(z, "OK")) return {KassenAntwort::OK, ""};
    // "ERR REG <Grund>" bzw. "ERR EXIT <Grund>"
    std::string grund = z;
    size_t pos = z.find(' ', 4);
    if (pos != std::string::npos) grund = z.substr(pos + 1);
    return {KassenAntwort::ABGELEHNT, grund};
}

} // namespace protokoll
