// ============================================================
// Automatisierte Integrationstests: Pico 2 (Schranke) + Pico 1 (Kasse)
// ohne Hardware und ohne UART-Kabel.
// Ausfuehren: pio test -e native   (im Ordner Parkhaus_Schranke)
//
// KasseSpy umschliesst die ECHTE KasseLogik und das echte Protokoll und
// zeichnet jede Anfrage auf. Auch der produktive RamParkSpeicher wird
// verwendet. Ersetzt sind nur Servo (SchrankeMock), Zeit (UhrFake) und
// Bildschirm (AnzeigeMock / AnzeigeDummy).
// ============================================================
#include <unity.h>
#include <KasseLogik.h>
#include <RamParkSpeicher.h>
#include <SchrankenLogik.h>
#include <AnzeigeDoubles.h>
#include <KasseSpy.h>
#include <SchrankeMock.h>
#include <UhrFake.h>

struct Parkhaus {
    UhrFake uhr;                     // gemeinsame Zeit fuer beide Picos
    RamParkSpeicher<10> speicher;    // produktiv
    AnzeigeDummy kassenAnzeige;      // Kassenbildschirm wird hier nicht geprueft
    KasseLogik kasse{uhr, speicher, kassenAnzeige};
    KasseSpy spy{kasse};
    SchrankeMock ein;
    SchrankeMock aus;
    AnzeigeMock terminal;            // Meldungen an den Fahrer
    SchrankenLogik schranke{ein, aus, spy, uhr, terminal};
};

void setUp() {}
void tearDown() {}

void test_E2E_kernprozess_einfahrt_zahlung_ausfahrt() {
    Parkhaus p;
    p.schranke.befehl("E LU12345");
    TEST_ASSERT_EQUAL_INT(1, p.ein.anzahlOeffnen);

    p.uhr.vorspulenSekunden(130);
    p.kasse.befehl("B LU12345"); // Zahlung an Pico 1

    p.schranke.befehl("A LU12345");
    TEST_ASSERT_EQUAL_INT(1, p.aus.anzahlOeffnen);

    // Spy: genau diese Anfragen gingen ueber die (wegfallende) UART-Leitung
    TEST_ASSERT_EQUAL_INT(2, (int)p.spy.anfragen.size());
    TEST_ASSERT_EQUAL_STRING("REG LU12345", p.spy.anfragen[0].c_str());
    TEST_ASSERT_EQUAL_STRING("EXIT LU12345", p.spy.anfragen[1].c_str());
    TEST_ASSERT_EQUAL_STRING("OK EXIT LU12345", p.spy.antworten[1].c_str());

    // Kasse hat den Vorgang korrekt abgeschlossen (TC14)
    TEST_ASSERT_EQUAL_INT(0, p.speicher.anzahl());
    TEST_ASSERT_EQUAL_INT(1, (int)p.kasse.historie().size());
    TEST_ASSERT_EQUAL_UINT32(130, p.kasse.historie().front().betragRappen);
    TEST_ASSERT_EQUAL_UINT32(130000, p.kasse.historie().front().ausfahrtMs);
}

void test_TC13_integration_unbezahlt_bleibt_ausfahrt_zu() {
    Parkhaus p;
    p.schranke.befehl("E LU12345");
    p.uhr.vorspulenSekunden(20);
    p.schranke.befehl("A LU12345");
    TEST_ASSERT_EQUAL_INT(0, p.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(p.terminal.enthaelt("ABGELEHNT: Bitte zuerst an der Kasse bezahlen (CHF 1.10)."));
    TEST_ASSERT_EQUAL_INT(1, p.speicher.anzahl());
}

void test_TC11_integration_fehlgeschlagene_zahlung_gibt_nicht_frei() {
    Parkhaus p;
    p.schranke.befehl("E LU12345");
    p.uhr.vorspulenSekunden(75);
    p.kasse.befehl("X LU12345");
    p.schranke.befehl("A LU12345");
    TEST_ASSERT_EQUAL_INT(0, p.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(p.terminal.enthaelt("(CHF 1.20)"));
}

void test_TC05_integration_ungueltiges_kennzeichen() {
    Parkhaus p;
    p.schranke.befehl("E XX00000");
    TEST_ASSERT_EQUAL_INT(0, p.ein.anzahlOeffnen);
    TEST_ASSERT_EQUAL_STRING("ERR REG Ungueltiges Kennzeichen", p.spy.antworten[0].c_str());
    TEST_ASSERT_TRUE(p.kasse.protokoll().back().find("EINFAHRT ABGELEHNT XX00000") != std::string::npos);
}

void test_TC03_leeres_kennzeichen_sendet_keine_anfrage() {
    Parkhaus p;
    p.schranke.befehl("E");
    TEST_ASSERT_EQUAL_INT(0, (int)p.spy.anfragen.size()); // Spy: Pico 1 wurde nicht belaestigt
    TEST_ASSERT_EQUAL_INT(0, p.ein.anzahlOeffnen);
}

void test_TC17_integration_ausfahrt_ohne_einfahrt() {
    Parkhaus p;
    p.schranke.befehl("A ZH54321");
    TEST_ASSERT_EQUAL_INT(0, p.aus.anzahlOeffnen);
    TEST_ASSERT_TRUE(p.terminal.enthaelt("ABGELEHNT: Kein aktiver Parkvorgang."));
}

void test_TC18_integration_doppelte_einfahrt() {
    Parkhaus p;
    p.schranke.befehl("E LU12345");
    p.schranke.befehl("E lu 12345");
    TEST_ASSERT_EQUAL_INT(1, p.ein.anzahlOeffnen);
    TEST_ASSERT_TRUE(p.terminal.enthaelt("ABGELEHNT: Fahrzeug bereits im Parkhaus."));
}

void test_TC19_integration_volles_parkhaus_mit_produktivem_speicher() {
    Parkhaus p;
    for (int i = 0; i < 10; i++) p.schranke.befehl("E LU1000" + std::to_string(i) + "1");
    TEST_ASSERT_EQUAL_INT(10, p.ein.anzahlOeffnen);
    p.schranke.befehl("E LU10011");
    TEST_ASSERT_EQUAL_INT(10, p.ein.anzahlOeffnen);
    TEST_ASSERT_TRUE(p.terminal.enthaelt("ABGELEHNT: Parkhaus voll."));
}

void test_TC20_integration_hilferuf_erreicht_die_kasse() {
    Parkhaus p;
    p.schranke.befehl("R");
    TEST_ASSERT_EQUAL_STRING("HELP", p.spy.anfragen.back().c_str());
    TEST_ASSERT_TRUE(p.terminal.enthaelt("Hilferuf gesendet."));
    TEST_ASSERT_TRUE(p.kasse.protokoll().back().find("HILFERUF") != std::string::npos);
}

void test_status_meldet_verbindung_ok() {
    Parkhaus p;
    p.schranke.befehl("Z");
    TEST_ASSERT_EQUAL_STRING("PING", p.spy.anfragen.back().c_str());
    TEST_ASSERT_TRUE(p.terminal.enthaelt("Verbindung Kasse:  OK"));
}

void test_zuverlaessigkeit_kernprozess_fuenfmal_hintereinander() {
    Parkhaus p;
    const char *kz[] = {"LU12345", "ZH54321"};
    for (int i = 0; i < 5; i++) {
        std::string k = kz[i % 2];
        p.schranke.befehl("E " + k);
        p.uhr.vorspulenSekunden(200);
        p.kasse.bezahlen(k);
        p.schranke.befehl("A " + k);
        p.uhr.vorspulen(SchrankenLogik::OFFEN_DAUER_MS);
        p.schranke.aktualisieren();
    }
    TEST_ASSERT_EQUAL_INT(5, p.ein.anzahlOeffnen);
    TEST_ASSERT_EQUAL_INT(5, p.aus.anzahlOeffnen);
    TEST_ASSERT_EQUAL_INT(5, (int)p.kasse.historie().size());
    TEST_ASSERT_EQUAL_INT(0, p.speicher.anzahl());
    TEST_ASSERT_FALSE(p.ein.offen);
    TEST_ASSERT_FALSE(p.aus.offen);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_E2E_kernprozess_einfahrt_zahlung_ausfahrt);
    RUN_TEST(test_TC13_integration_unbezahlt_bleibt_ausfahrt_zu);
    RUN_TEST(test_TC11_integration_fehlgeschlagene_zahlung_gibt_nicht_frei);
    RUN_TEST(test_TC05_integration_ungueltiges_kennzeichen);
    RUN_TEST(test_TC03_leeres_kennzeichen_sendet_keine_anfrage);
    RUN_TEST(test_TC17_integration_ausfahrt_ohne_einfahrt);
    RUN_TEST(test_TC18_integration_doppelte_einfahrt);
    RUN_TEST(test_TC19_integration_volles_parkhaus_mit_produktivem_speicher);
    RUN_TEST(test_TC20_integration_hilferuf_erreicht_die_kasse);
    RUN_TEST(test_status_meldet_verbindung_ok);
    RUN_TEST(test_zuverlaessigkeit_kernprozess_fuenfmal_hintereinander);
    return UNITY_END();
}
