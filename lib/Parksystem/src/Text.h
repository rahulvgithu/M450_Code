#pragma once
// Kleine Textfunktionen ohne Arduino-String, damit die Logik auch
// auf dem PC (env:native) kompiliert.
#include <string>
#include <cctype>
#include <cstdio>

namespace text {

inline std::string trim(const std::string &s) {
    size_t a = 0, b = s.size();
    while (a < b && std::isspace((unsigned char)s[a])) a++;
    while (b > a && std::isspace((unsigned char)s[b - 1])) b--;
    return s.substr(a, b - a);
}

inline std::string gross(std::string s) {
    for (auto &c : s) c = (char)std::toupper((unsigned char)c);
    return s;
}

inline bool beginntMit(const std::string &s, const std::string &prefix) {
    return s.compare(0, prefix.size(), prefix) == 0;
}

inline std::string zahl(unsigned long n) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%lu", n);
    return buf;
}

inline std::string zweistellig(unsigned long n) {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%02lu", n);
    return buf;
}

inline std::string betrag(unsigned long rappen) {
    return "CHF " + zahl(rappen / 100UL) + "." + zweistellig(rappen % 100UL);
}

// Terminal-Eingabe "E LU12345" -> Buchstabe 'E', Argument " LU12345"
struct Befehl {
    char buchstabe;
    std::string argument;
};

// false bei leerer Eingabe
inline bool zerlegeBefehl(const std::string &eingabe, Befehl &befehl) {
    std::string zeile = trim(eingabe);
    if (zeile.empty()) return false;
    befehl.buchstabe = (char)std::toupper((unsigned char)zeile[0]);
    befehl.argument = zeile.substr(1);
    return true;
}

} // namespace text
