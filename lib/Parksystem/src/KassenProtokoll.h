#pragma once
// Textprotokoll zwischen Pico 2 (Schranke) und Pico 1 (Kasse) ueber UART.
//   Anfrage:  REG <KZ> | EXIT <KZ> | PING | HELP
//   Antwort:  OK REG <KZ> | OK EXIT <KZ> | ERR REG <Grund> | ERR EXIT <Grund> | PONG | OK HELP
#include <cstring>
#include "Schnittstellen.h"
#include "Text.h"

namespace protokoll {

// Befehle (Pico 2 -> Pico 1)
const char *const EINFAHRT = "REG";
const char *const AUSFAHRT = "EXIT";
const char *const PING = "PING";
const char *const HILFE = "HELP";

// Antworten (Pico 1 -> Pico 2)
const char *const PONG = "PONG";
const char *const OK = "OK";
const char *const FEHLER = "ERR";

// "REG LU12345"
inline std::string anfrage(const char *befehl, const std::string &kennzeichen) {
    return std::string(befehl) + " " + kennzeichen;
}

// "OK REG LU12345" bzw. "OK HELP"
inline std::string okAntwort(const char *befehl, const std::string &kennzeichen = "") {
    std::string antwort = std::string(OK) + " " + befehl;
    return kennzeichen.empty() ? antwort : antwort + " " + kennzeichen;
}

// "ERR REG Ungueltiges Kennzeichen"
inline std::string fehlerAntwort(const std::string &befehl, const std::string &grund) {
    return std::string(FEHLER) + " " + befehl + " " + grund;
}

inline bool istAnfrage(const std::string &zeile, const char *befehl) { return text::beginntMit(zeile, befehl); }

// Teil der Anfrage nach dem Befehl, z. B. " LU12345" bei "REG LU12345"
inline std::string argument(const std::string &zeile, const char *befehl) { return zeile.substr(std::strlen(befehl)); }

// Wandelt die Antwortzeile von Pico 1 in eine KassenAntwort um.
// Leere Antwort (Timeout) = KEINE_VERBINDUNG.
inline KassenAntwort auswerten(const std::string &zeile) {
    std::string z = text::trim(zeile);
    if (z.empty()) return {KassenAntwort::KEINE_VERBINDUNG, ""};
    if (text::beginntMit(z, OK)) return {KassenAntwort::OK, ""};
    // "ERR REG <Grund>": Grund beginnt nach dem zweiten Leerzeichen
    size_t nachFehlerKennung = std::strlen(FEHLER) + 1;
    size_t pos = z.find(' ', nachFehlerKennung);
    std::string grund = pos == std::string::npos ? z : z.substr(pos + 1);
    return {KassenAntwort::ABGELEHNT, grund};
}

} // namespace protokoll
