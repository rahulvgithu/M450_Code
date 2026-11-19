#pragma once
// ============================================================
// Schnittstellen zu Hardware und Umgebung.
// Die Logik (KasseLogik, SchrankenLogik) kennt nur diese
// Schnittstellen. Die echten Implementierungen (millis(), Servo,
// Serial, UART) stehen in src/main.cpp der beiden Projekte,
// die Test Doubles im Ordner test/doubles.
// ============================================================
#include <string>

// Zeit: ersetzt millis()
class IUhr {
public:
    virtual ~IUhr() {}
    virtual unsigned long jetztMs() const = 0;
};

// Ausgabe fuer Fahrer bzw. Kassenpersonal: ersetzt Serial.println()
class IAnzeige {
public:
    virtual ~IAnzeige() {}
    virtual void zeige(const std::string &text) = 0;
};

// Schranke: ersetzt Servo.write()
class ISchranke {
public:
    virtual ~ISchranke() {}
    virtual void oeffnen() = 0;
    virtual void schliessen() = 0;
};

// Antwort der Kasse an die Schranke
struct KassenAntwort {
    enum Status { OK, ABGELEHNT, KEINE_VERBINDUNG };
    Status status;
    std::string grund; // nur bei ABGELEHNT
};

// Kasse aus Sicht der Schranke: ersetzt die UART-Verbindung zu Pico 1
class IKasse {
public:
    virtual ~IKasse() {}
    virtual KassenAntwort einfahrt(const std::string &kennzeichen) = 0;
    virtual KassenAntwort ausfahrt(const std::string &kennzeichen) = 0;
    virtual bool verbunden() = 0;
    virtual bool hilfeRufen() = 0;
};

// Ein Parkvorgang
struct Parkvorgang {
    std::string kennzeichen;
    unsigned long einfahrtMs = 0;
    unsigned long ausfahrtMs = 0;
    unsigned long betragRappen = 0;
    bool bezahlt = false;
};

// Speicherung der Fahrzeuge im Parkhaus
class IParkSpeicher {
public:
    virtual ~IParkSpeicher() {}
    // false, wenn kein Platz mehr frei ist. platz = 1-basierte Platznummer.
    virtual bool hinzufuegen(const Parkvorgang &vorgang, int &platz) = 0;
    virtual Parkvorgang *finde(const std::string &kennzeichen) = 0;
    virtual bool entfernen(const std::string &kennzeichen) = 0;
    virtual int anzahl() const = 0;
    virtual int kapazitaet() const = 0;
    // Zugriff auf Platz 1..kapazitaet(), nullptr wenn frei
    virtual const Parkvorgang *platz(int nummer) const = 0;
};
