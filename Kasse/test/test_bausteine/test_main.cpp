// ============================================================
// Automatisierte Tests: Bausteine ohne Abhaengigkeiten
// (Kennzeichenpruefung, Tarif, Protokoll) und Vertragstest
// Speicher: Der ParkSpeicherFake muss sich gleich verhalten wie der
// produktive RamParkSpeicher, sonst waeren die anderen Tests wertlos.
// Ausfuehren: pio test -e native   (im Ordner Kasse)
// ============================================================
#include <unity.h>
#include <Kennzeichen.h>
#include <KassenProtokoll.h>
#include <RamParkSpeicher.h>
#include <Tarif.h>
#include <ParkSpeicherFake.h>

void setUp() {}
void tearDown() {}

// ---------------- Kennzeichen (US03) ----------------

void test_kennzeichen_wird_normalisiert() {
    TEST_ASSERT_EQUAL_STRING("LU12345", kennzeichen::normalisiere("lu 12345").c_str());
    TEST_ASSERT_EQUAL_STRING("ZH54321", kennzeichen::normalisiere("  zh-54321 ").c_str());
}

void test_TC05_gueltige_und_ungueltige_kennzeichen() {
    TEST_ASSERT_TRUE(kennzeichen::istGueltig("LU12345"));
    TEST_ASSERT_TRUE(kennzeichen::istGueltig("ZH54321"));
    TEST_ASSERT_TRUE(kennzeichen::istGueltig("BE1"));        // Grenzwert: 1 Ziffer
    TEST_ASSERT_TRUE(kennzeichen::istGueltig("GE999999"));   // Grenzwert: 6 Ziffern
    TEST_ASSERT_FALSE(kennzeichen::istGueltig("GE9999999")); // 7 Ziffern
    TEST_ASSERT_FALSE(kennzeichen::istGueltig("XX00000"));   // kein Kanton
    TEST_ASSERT_FALSE(kennzeichen::istGueltig("LU01234"));   // beginnt mit 0
    TEST_ASSERT_FALSE(kennzeichen::istGueltig("LU12A45"));   // Buchstabe
    TEST_ASSERT_FALSE(kennzeichen::istGueltig("LU"));        // keine Nummer
    TEST_ASSERT_FALSE(kennzeichen::istGueltig(""));
}

// ---------------- Tarif (TC08, TC09, TC16) ----------------

void test_TC16_tarif_grenzwerte() {
    TEST_ASSERT_EQUAL_UINT32(100, tarif::gebuehrRappen(0));
    TEST_ASSERT_EQUAL_UINT32(110, tarif::gebuehrRappen(1));
    TEST_ASSERT_EQUAL_UINT32(110, tarif::gebuehrRappen(59000));
    TEST_ASSERT_EQUAL_UINT32(110, tarif::gebuehrRappen(60000));
    TEST_ASSERT_EQUAL_UINT32(120, tarif::gebuehrRappen(60001));
    TEST_ASSERT_EQUAL_UINT32(120, tarif::gebuehrRappen(61000));
    TEST_ASSERT_EQUAL_UINT32(120, tarif::gebuehrRappen(119000));
    TEST_ASSERT_EQUAL_UINT32(130, tarif::gebuehrRappen(121000));
}

void test_TC08_TC09_tarif_standardwerte() {
    TEST_ASSERT_EQUAL_UINT32(110, tarif::gebuehrRappen(31000));  // kurz
    TEST_ASSERT_EQUAL_UINT32(160, tarif::gebuehrRappen(302000)); // TC08
    TEST_ASSERT_EQUAL_UINT32(210, tarif::gebuehrRappen(604000)); // lang
}

// ---------------- Protokoll Pico 2 <-> Pico 1 ----------------

void test_protokoll_antworten_auswerten() {
    TEST_ASSERT_EQUAL_INT(KassenAntwort::OK, protokoll::auswerten("OK REG LU12345").status);

    KassenAntwort abgelehnt = protokoll::auswerten("ERR EXIT Kein aktiver Parkvorgang");
    TEST_ASSERT_EQUAL_INT(KassenAntwort::ABGELEHNT, abgelehnt.status);
    TEST_ASSERT_EQUAL_STRING("Kein aktiver Parkvorgang", abgelehnt.grund.c_str());

    TEST_ASSERT_EQUAL_INT(KassenAntwort::KEINE_VERBINDUNG, protokoll::auswerten("").status);
    TEST_ASSERT_EQUAL_INT(KassenAntwort::KEINE_VERBINDUNG, protokoll::auswerten("  \r").status);
}

// ---------------- Vertragstest Speicher: Fake == Produktiv ----------------

static Parkvorgang fahrzeug(const char *kz) {
    Parkvorgang p;
    p.kennzeichen = kz;
    return p;
}

static void pruefeSpeicherVertrag(IParkSpeicher &s) {
    int platz = 0;
    TEST_ASSERT_EQUAL_INT(2, s.kapazitaet());
    TEST_ASSERT_TRUE(s.hinzufuegen(fahrzeug("LU1"), platz));
    TEST_ASSERT_EQUAL_INT(1, platz);
    TEST_ASSERT_TRUE(s.hinzufuegen(fahrzeug("LU2"), platz));
    TEST_ASSERT_EQUAL_INT(2, platz);
    TEST_ASSERT_FALSE(s.hinzufuegen(fahrzeug("LU3"), platz)); // voll
    TEST_ASSERT_EQUAL_INT(2, s.anzahl());

    TEST_ASSERT_NOT_NULL(s.finde("LU2"));
    TEST_ASSERT_NULL(s.finde("LU3"));

    TEST_ASSERT_TRUE(s.entfernen("LU1"));
    TEST_ASSERT_FALSE(s.entfernen("LU1"));
    TEST_ASSERT_NULL(s.platz(1));
    TEST_ASSERT_TRUE(s.hinzufuegen(fahrzeug("LU3"), platz));
    TEST_ASSERT_EQUAL_INT(1, platz); // freier Platz wird wieder verwendet
}

void test_speicher_vertrag_ram_speicher_produktiv() {
    RamParkSpeicher<2> produktiv;
    pruefeSpeicherVertrag(produktiv);
}

void test_speicher_vertrag_park_speicher_fake() {
    ParkSpeicherFake fake(2);
    pruefeSpeicherVertrag(fake);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_kennzeichen_wird_normalisiert);
    RUN_TEST(test_TC05_gueltige_und_ungueltige_kennzeichen);
    RUN_TEST(test_TC16_tarif_grenzwerte);
    RUN_TEST(test_TC08_TC09_tarif_standardwerte);
    RUN_TEST(test_protokoll_antworten_auswerten);
    RUN_TEST(test_speicher_vertrag_ram_speicher_produktiv);
    RUN_TEST(test_speicher_vertrag_park_speicher_fake);
    return UNITY_END();
}
