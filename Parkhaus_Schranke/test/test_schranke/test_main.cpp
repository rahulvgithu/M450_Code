// ============================================================
// Automatisierte Tests: Logik von Pico 2 (Schranke), isoliert
// Ausfuehren: pio test -e native   (im Ordner Parkhaus_Schranke)
//
// Test Doubles:  SchrankeMock (statt Servo), KasseStub (statt UART zu Pico 1),
//                UhrFake (statt millis), AnzeigeMock / AnzeigeDummy (statt Serial)
// ============================================================
#include <unity.h>
#include <SchrankenLogik.h>
#include <AnzeigeDoubles.h>
#include <KasseStub.h>
#include <SchrankeMock.h>
#include <UhrFake.h>

// Testaufbau: echte SchrankenLogik, alle Abhaengigkeiten durch Doubles ersetzt
struct Schranke {
    SchrankeMock ein;
    SchrankeMock aus;
    KasseStub kasse;
    UhrFake uhr;
    AnzeigeMock anzeige;
    SchrankenLogik logik;

    explicit Schranke(KasseStub stub) : kasse(stub), logik(ein, aus, kasse, uhr, anzeige) {
        logik.start();
        ein.zuruecksetzen();
        aus.zuruecksetzen();
        anzeige.leeren();
    }
};

void setUp() {}
void tearDown() {}

void test_start_schliesst_beide_schranken() {
    SchrankeMock ein, aus;
    KasseStub kasse = KasseStub::erlaubtAlles();
    UhrFake uhr;
    AnzeigeDummy anzeige; // Meldungen spielen hier keine Rolle
    SchrankenLogik logik(ein, aus, kasse, uhr, anzeige);

    logik.start();

    TEST_ASSERT_EQUAL_INT(1, ein.anzahlSchliessen);
    TEST_ASSERT_EQUAL_INT(1, aus.anzahlSchliessen);
    TEST_ASSERT_FALSE(ein.offen);
}

// ---------------- US02: Einfahrt ----------------

void test_TC04_gueltiges_kennzeichen_oeffnet_nur_die_einfahrt() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.befehl("E LU12345");
    TEST_ASSERT_EQUAL_INT(1, s.ein.anzahlOeffnen);
    TEST_ASSERT_EQUAL_INT(0, s.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Willkommen LU12345."));
}

void test_TC04_einfahrt_schliesst_genau_nach_5_sekunden() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.fahrzeugAnEinfahrt("LU12345");

    s.uhr.vorspulen(4999);
    s.logik.aktualisieren();
    TEST_ASSERT_TRUE(s.ein.offen);
    TEST_ASSERT_EQUAL_INT(0, s.ein.anzahlSchliessen);

    s.uhr.vorspulen(1);
    s.logik.aktualisieren();
    TEST_ASSERT_FALSE(s.ein.offen);
    TEST_ASSERT_EQUAL_INT(1, s.ein.anzahlSchliessen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Durchfahrt erfolgt."));
}

void test_TC03_ohne_kennzeichen_bleibt_schranke_zu_mit_hinweis_auf_hilfe() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.befehl("E   ");
    TEST_ASSERT_EQUAL_INT(0, s.ein.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Kennzeichen nicht erkannt"));
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("mit R Hilfe rufen"));
}

void test_TC05_abgelehntes_kennzeichen_oeffnet_nicht() {
    Schranke s(KasseStub::lehntAb("Ungueltiges Kennzeichen"));
    s.logik.fahrzeugAnEinfahrt("XX00000");
    TEST_ASSERT_EQUAL_INT(0, s.ein.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("ABGELEHNT: Ungueltiges Kennzeichen."));
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Einfahrtsschranke bleibt GESCHLOSSEN"));
}

// ---------------- US07 / US08: Ausfahrt ----------------

void test_TC12_bezahlte_ausfahrt_oeffnet_nur_die_ausfahrt() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.befehl("A LU12345");
    TEST_ASSERT_EQUAL_INT(1, s.aus.anzahlOeffnen);
    TEST_ASSERT_EQUAL_INT(0, s.ein.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Gute Fahrt LU12345."));

    s.uhr.vorspulen(SchrankenLogik::OFFEN_DAUER_MS);
    s.logik.aktualisieren();
    TEST_ASSERT_FALSE(s.aus.offen);
}

void test_TC13_unbezahlte_ausfahrt_bleibt_zu() {
    Schranke s(KasseStub::lehntAb("Bitte zuerst an der Kasse bezahlen (CHF 1.10)"));
    s.logik.fahrzeugAnAusfahrt("LU12345");
    TEST_ASSERT_EQUAL_INT(0, s.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Bitte zuerst an der Kasse bezahlen (CHF 1.10)."));
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Ausfahrtsschranke bleibt GESCHLOSSEN"));
}

// ---------------- US10 / NF Fehlerbehandlung ----------------

void test_TC15_ohne_verbindung_zur_kasse_bleibt_schranke_zu() {
    Schranke s(KasseStub::ohneVerbindung());
    s.logik.fahrzeugAnAusfahrt("LU12345");
    TEST_ASSERT_EQUAL_INT(0, s.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Keine Verbindung zur Kasse (Pico 1). Bitte Personal rufen."));

    s.logik.befehl("Z");
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Verbindung Kasse:  KEINE VERBINDUNG"));
}

void test_TC15_nach_wiederherstellung_funktioniert_die_ausfahrt() {
    Schranke s(KasseStub::ohneVerbindung());
    s.logik.fahrzeugAnAusfahrt("LU12345");
    TEST_ASSERT_EQUAL_INT(0, s.aus.anzahlOeffnen);

    s.kasse = KasseStub::erlaubtAlles(); // Kabel wieder eingesteckt
    s.logik.fahrzeugAnAusfahrt("LU12345");
    TEST_ASSERT_EQUAL_INT(1, s.aus.anzahlOeffnen);
}

void test_TC20_hilferuf_mit_und_ohne_verbindung() {
    Schranke mit(KasseStub::erlaubtAlles());
    mit.logik.befehl("R");
    TEST_ASSERT_TRUE(mit.anzeige.enthaelt("Hilferuf gesendet."));

    Schranke ohne(KasseStub::ohneVerbindung());
    ohne.logik.befehl("R");
    TEST_ASSERT_TRUE(ohne.anzeige.enthaelt("Bitte Notruftaste benutzen."));
}

void test_F006_befehl_O_oeffnet_keine_schranke() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.befehl("O");
    TEST_ASSERT_EQUAL_INT(0, s.ein.anzahlOeffnen);
    TEST_ASSERT_EQUAL_INT(0, s.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Unbekannter Befehl"));
}

void test_befehl_S_schliesst_beide_schranken() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.fahrzeugAnEinfahrt("LU12345");
    s.logik.fahrzeugAnAusfahrt("ZH54321");
    s.logik.befehl("S");
    TEST_ASSERT_FALSE(s.ein.offen);
    TEST_ASSERT_FALSE(s.aus.offen);
    TEST_ASSERT_TRUE(s.anzeige.enthaelt("Beide Schranken GESCHLOSSEN"));
}

void test_geschlossene_schranke_wird_nicht_ohne_grund_bewegt() {
    Schranke s(KasseStub::erlaubtAlles());
    s.uhr.vorspulen(60000);
    s.logik.aktualisieren();
    TEST_ASSERT_EQUAL_INT(0, s.ein.anzahlOeffnen + s.ein.anzahlSchliessen);
    TEST_ASSERT_EQUAL_INT(0, s.aus.anzahlOeffnen + s.aus.anzahlSchliessen);
}

// ---------------- Charakterisierungstests (vor dem Refactoring A04) ----------------
// Halten die heutigen Ausgaben fest, damit sich beim Refactoring nichts aendert.

static void pruefeZeilen(const AnzeigeMock &anzeige, const std::vector<std::string> &erwartet) {
    TEST_ASSERT_EQUAL_INT((int)erwartet.size(), (int)anzeige.zeilen.size());
    for (size_t i = 0; i < erwartet.size(); i++) {
        TEST_ASSERT_EQUAL_STRING(erwartet[i].c_str(), anzeige.zeilen[i].c_str());
    }
}

void test_char_startmeldung_und_hilfe() {
    SchrankeMock ein, aus;
    KasseStub kasse = KasseStub::erlaubtAlles();
    UhrFake uhr;
    AnzeigeMock anzeige;
    SchrankenLogik logik(ein, aus, kasse, uhr, anzeige);
    logik.start();
    pruefeZeilen(anzeige, {"PARKSYSTEM - PICO 2 SCHRANKE v1.2", "", "=== PARKSYSTEM: PICO 2 SCHRANKE v1.2 ===",
                           "E LU12345 - Einfahrt (Kennzeichen erfassen)", "A LU12345 - Ausfahrt",
                           "S         - Beide Schranken schliessen", "Z         - Status Schranken und Verbindung",
                           "R         - Hilfe vom Personal anfordern", "H         - Hilfe", ""});
}

void test_char_meldungen_einfahrt_bis_schliessen() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.befehl("E LU12345");
    s.uhr.vorspulen(5000);
    s.logik.aktualisieren();
    pruefeZeilen(s.anzeige, {"Willkommen LU12345.", "Einfahrtsschranke OFFEN", "Durchfahrt erfolgt.",
                             "Einfahrtsschranke GESCHLOSSEN"});
}

void test_char_meldungen_abgelehnt_ohne_verbindung_und_status() {
    Schranke abgelehnt(KasseStub::lehntAb("Parkhaus voll"));
    abgelehnt.logik.befehl("E LU12345");
    abgelehnt.logik.befehl("A");
    pruefeZeilen(abgelehnt.anzeige,
                 {"ABGELEHNT: Parkhaus voll.", "Einfahrtsschranke bleibt GESCHLOSSEN",
                  "FEHLER: Kennzeichen nicht erkannt. Bitte erneut eingeben oder mit R Hilfe rufen.",
                  "Ausfahrtsschranke bleibt GESCHLOSSEN"});

    Schranke ohne(KasseStub::ohneVerbindung());
    ohne.logik.befehl("A LU12345");
    ohne.logik.befehl("Z");
    pruefeZeilen(ohne.anzeige, {"FEHLER: Keine Verbindung zur Kasse (Pico 1). Bitte Personal rufen.",
                                "Ausfahrtsschranke bleibt GESCHLOSSEN", "Einfahrtsschranke: GESCHLOSSEN",
                                "Ausfahrtsschranke: GESCHLOSSEN", "Verbindung Kasse:  KEINE VERBINDUNG"});
}

void test_char_befehl_S_meldet_nur_offene_schranken() {
    Schranke s(KasseStub::erlaubtAlles());
    s.logik.befehl("A ZH54321");
    s.anzeige.leeren();
    s.logik.befehl("s");
    pruefeZeilen(s.anzeige, {"Ausfahrtsschranke GESCHLOSSEN", "Beide Schranken GESCHLOSSEN"});
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_char_startmeldung_und_hilfe);
    RUN_TEST(test_char_meldungen_einfahrt_bis_schliessen);
    RUN_TEST(test_char_meldungen_abgelehnt_ohne_verbindung_und_status);
    RUN_TEST(test_char_befehl_S_meldet_nur_offene_schranken);
    RUN_TEST(test_start_schliesst_beide_schranken);
    RUN_TEST(test_TC04_gueltiges_kennzeichen_oeffnet_nur_die_einfahrt);
    RUN_TEST(test_TC04_einfahrt_schliesst_genau_nach_5_sekunden);
    RUN_TEST(test_TC03_ohne_kennzeichen_bleibt_schranke_zu_mit_hinweis_auf_hilfe);
    RUN_TEST(test_TC05_abgelehntes_kennzeichen_oeffnet_nicht);
    RUN_TEST(test_TC12_bezahlte_ausfahrt_oeffnet_nur_die_ausfahrt);
    RUN_TEST(test_TC13_unbezahlte_ausfahrt_bleibt_zu);
    RUN_TEST(test_TC15_ohne_verbindung_zur_kasse_bleibt_schranke_zu);
    RUN_TEST(test_TC15_nach_wiederherstellung_funktioniert_die_ausfahrt);
    RUN_TEST(test_TC20_hilferuf_mit_und_ohne_verbindung);
    RUN_TEST(test_F006_befehl_O_oeffnet_keine_schranke);
    RUN_TEST(test_befehl_S_schliesst_beide_schranken);
    RUN_TEST(test_geschlossene_schranke_wird_nicht_ohne_grund_bewegt);
    return UNITY_END();
}
