#pragma once
#include <Schnittstellen.h>

// MOCK fuer ISchranke (ersetzt den Servo an GP8/GP9)
// Merkt sich, wie oft geoeffnet und geschlossen wurde und in welcher Stellung
// die Schranke zuletzt war. Der Test prueft damit, OB und WIE OFT die Logik
// den Servo angesteuert haette - ohne Hardware und ohne hinschauen.
class SchrankeMock : public ISchranke {
public:
    void oeffnen() override {
        anzahlOeffnen++;
        offen = true;
    }
    void schliessen() override {
        anzahlSchliessen++;
        offen = false;
    }
    void zuruecksetzen() {
        anzahlOeffnen = anzahlSchliessen = 0;
    }

    int anzahlOeffnen = 0;
    int anzahlSchliessen = 0;
    bool offen = false;
};
