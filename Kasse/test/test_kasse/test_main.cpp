// ============================================================
// Automatisierte Tests: Logik von Pico 1 (Kasse)
// Ausfuehren: pio test -e native   (im Ordner Kasse)
//
// Test Doubles:  UhrFake (statt millis), ParkSpeicherFake (statt RAM-Plaetze),
//                AnzeigeMock (statt Serial), AnzeigeDummy
// Die Testnamen beginnen mit der Testfall-ID aus dem Testkonzept.
// ============================================================
#include <unity.h>
#include <KasseLogik.h>
#include <AnzeigeDoubles.h>
#include <ParkSpeicherFake.h>
#include <UhrFake.h>

// Testaufbau: echte KasseLogik, alle Abhaengigkeiten durch Doubles ersetzt
struct Kasse {
    UhrFake uhr{1000};
    ParkSpeicherFake speicher{10};
    AnzeigeMock anzeige;
    KasseLogik logik{uhr, speicher, anzeige};
};

void setUp() {}
void tearDown() {}

// ---------------- US01 / US04: Einfahrt und Parkvorgang ----------------

void test_TC01_einfahrt_speichert_kennzeichen_mit_datum_und_uhrzeit() {
    Kasse k;
    k.logik.befehl("T 14.11.2026 13:36:00");
    k.uhr.vorspulenSekunden(12);

    TEST_ASSERT_EQUAL_STRING("", k.logik.einfahrt("LU12345").c_str());

    TEST_ASSERT_NOT_NULL(k.speicher.finde("LU12345"));
    k.logik.befehl("G LU12345");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("Einfahrt:       14.11.2026 13:36:12"));
}

void test_TC02_einfahrtszeit_entspricht_exakt_der_uhrzeit() {
    Kasse k;
    k.uhr.setze(123456);
    k.logik.einfahrt("LU12345");
    // Mit der Fake-Uhr ist die Abweichung 0 ms (Testkonzept: max. 2 s)
    TEST_ASSERT_EQUAL_UINT32(123456, k.speicher.finde("LU12345")->einfahrtMs);
}

void test_TC03_leeres_kennzeichen_wird_abgelehnt_ohne_parkvorgang() {
    Kasse k;
    TEST_ASSERT_EQUAL_STRING("Kennzeichen nicht erkannt", k.logik.einfahrt("   ").c_str());
    TEST_ASSERT_EQUAL_INT(0, k.speicher.anzahl());
}

void test_TC05_ungueltiges_kennzeichen_wird_abgelehnt_und_protokolliert() {
    Kasse k;
    TEST_ASSERT_EQUAL_STRING("Ungueltiges Kennzeichen", k.logik.einfahrt("XX00000").c_str());
    TEST_ASSERT_EQUAL_INT(0, k.speicher.anzahl());
    TEST_ASSERT_TRUE(k.logik.protokoll().back().find("EINFAHRT ABGELEHNT XX00000 (ungueltiges Kennzeichen)") !=
                     std::string::npos);
}

void test_TC06_parkvorgang_erscheint_in_der_liste_mit_status() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.logik.befehl("L");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("Aktive Parkvorgaenge (1/10):"));
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("Platz 1: LU12345"));
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("IM PARKHAUS | UNBEZAHLT"));
}

void test_TC07_parkvorgang_bleibt_nach_zwei_minuten_bestehen() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.uhr.vorspulenSekunden(120); // statt 2 Minuten warten
    TEST_ASSERT_NOT_NULL(k.speicher.finde("LU12345"));
    TEST_ASSERT_FALSE(k.speicher.finde("LU12345")->bezahlt);
}

void test_TC18_doppelte_einfahrt_wird_abgelehnt() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    TEST_ASSERT_EQUAL_STRING("Fahrzeug bereits im Parkhaus", k.logik.einfahrt("lu 12345").c_str());
    TEST_ASSERT_EQUAL_INT(1, k.speicher.anzahl());
}

void test_TC19_elftes_fahrzeug_wird_bei_vollem_parkhaus_abgelehnt() {
    Kasse k;
    for (int i = 1; i <= 10; i++) {
        TEST_ASSERT_EQUAL_STRING("", k.logik.einfahrt("LU1000" + std::to_string(i - 1) + "1").c_str());
    }
    TEST_ASSERT_EQUAL_STRING("Parkhaus voll", k.logik.einfahrt("LU10011").c_str());
}

void test_TC19_nach_einer_ausfahrt_ist_wieder_ein_platz_frei() {
    Kasse k;
    for (int i = 0; i < 10; i++) k.logik.einfahrt("LU2000" + std::to_string(i) + "1"); // LU200001 ...
    TEST_ASSERT_EQUAL_INT(10, k.speicher.anzahl());
    k.logik.bezahlen("LU200001");
    TEST_ASSERT_EQUAL_STRING("", k.logik.ausfahrt("LU200001").c_str());
    TEST_ASSERT_EQUAL_STRING("", k.logik.einfahrt("LU10011").c_str());
}

// ---------------- US05: Gebuehr ----------------

void test_TC08_gebuehr_nach_302_sekunden_ist_chf_1_60() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.uhr.vorspulenSekunden(302);
    TEST_ASSERT_EQUAL_UINT32(160, k.logik.gebuehrRappen("LU12345"));
    k.logik.befehl("G LU12345");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("Gebuehr:        CHF 1.60"));
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("Parkdauer:      302 Sekunden"));
}

void test_TC09_kurzer_und_langer_aufenthalt() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.uhr.vorspulenSekunden(30);
    TEST_ASSERT_EQUAL_UINT32(110, k.logik.gebuehrRappen("LU12345")); // kurz: 30 s
    k.uhr.vorspulenSekunden(570);
    TEST_ASSERT_EQUAL_UINT32(200, k.logik.gebuehrRappen("LU12345")); // lang: 10 min
}

void test_TC09_zwei_stunden_parkdauer_mit_vorbereitetem_fake() {
    // Der Fake erlaubt, den Zustand direkt herzustellen: Fahrzeug steht seit 2 h im Parkhaus
    Kasse k;
    k.speicher.vorbereiten("ZH54321", 0);
    k.uhr.setze(2UL * 60 * 60 * 1000);
    TEST_ASSERT_EQUAL_UINT32(1300, k.logik.gebuehrRappen("ZH54321")); // 1.00 + 120 x 0.10
}

// ---------------- US06: Zahlung ----------------

void test_TC10_erfolgreiche_zahlung_markiert_als_bezahlt() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.uhr.vorspulenSekunden(75);
    k.logik.befehl("B LU12345");
    TEST_ASSERT_TRUE(k.speicher.finde("LU12345")->bezahlt);
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("ZAHLUNG ERFOLGREICH: CHF 1.20"));
}

void test_TC10_F008_bezahlter_betrag_bleibt_fix() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.uhr.vorspulenSekunden(75);
    k.logik.bezahlen("LU12345");
    k.uhr.vorspulenSekunden(120);
    TEST_ASSERT_EQUAL_UINT32(120, k.logik.gebuehrRappen("LU12345"));
}

void test_TC11_fehlgeschlagene_zahlung_bleibt_unbezahlt() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.logik.befehl("X LU12345");
    TEST_ASSERT_FALSE(k.speicher.finde("LU12345")->bezahlt);
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("ZAHLUNG FEHLGESCHLAGEN"));
    TEST_ASSERT_TRUE(k.logik.ausfahrt("LU12345").find("Bitte zuerst an der Kasse bezahlen") == 0);
}

// ---------------- US07 / US08 / US09: Ausfahrt ----------------

void test_TC13_unbezahlte_ausfahrt_wird_mit_betrag_abgelehnt() {
    Kasse k;
    k.logik.einfahrt("LU12345");
    k.uhr.vorspulenSekunden(20);
    TEST_ASSERT_EQUAL_STRING("Bitte zuerst an der Kasse bezahlen (CHF 1.10)", k.logik.ausfahrt("LU12345").c_str());
    TEST_ASSERT_NOT_NULL(k.speicher.finde("LU12345"));
}

void test_TC14_ausfahrt_schliesst_parkvorgang_mit_ausfahrtszeit_ab() {
    Kasse k;
    k.logik.einfahrt("LU12345"); // bei 1000 ms
    k.uhr.vorspulenSekunden(130);
    k.logik.bezahlen("LU12345");
    k.uhr.vorspulenSekunden(32);
    TEST_ASSERT_EQUAL_STRING("", k.logik.ausfahrt("LU12345").c_str());

    TEST_ASSERT_NULL(k.speicher.finde("LU12345"));
    TEST_ASSERT_EQUAL_INT(1, (int)k.logik.historie().size());
    TEST_ASSERT_EQUAL_UINT32(163000, k.logik.historie().front().ausfahrtMs);
    TEST_ASSERT_EQUAL_UINT32(130, k.logik.historie().front().betragRappen);
    k.logik.befehl("P");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("LU12345 | Einfahrt"));
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("| CHF 1.30 | ABGESCHLOSSEN"));
}

void test_TC17_ausfahrt_ohne_einfahrt_wird_abgelehnt() {
    Kasse k;
    TEST_ASSERT_EQUAL_STRING("Kein aktiver Parkvorgang", k.logik.ausfahrt("ZH54321").c_str());
}

// ---------------- Protokoll zu Pico 2 / US10 ----------------

void test_protokoll_einfahrt_ok_und_abgelehnt() {
    Kasse k;
    TEST_ASSERT_EQUAL_STRING("OK REG LU12345", k.logik.verarbeiteAnfrage("REG lu 12345").c_str());
    TEST_ASSERT_EQUAL_STRING("ERR REG Ungueltiges Kennzeichen", k.logik.verarbeiteAnfrage("REG XX00000").c_str());
    TEST_ASSERT_EQUAL_STRING("PONG", k.logik.verarbeiteAnfrage("PING").c_str());
    TEST_ASSERT_EQUAL_STRING("ERR Unbekannte Anfrage", k.logik.verarbeiteAnfrage("OPEN").c_str());
}

void test_TC20_hilferuf_wird_angezeigt_und_protokolliert() {
    Kasse k;
    TEST_ASSERT_EQUAL_STRING("OK HELP", k.logik.verarbeiteAnfrage("HELP").c_str());
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("!!! HILFERUF AN DER SCHRANKE !!!"));
    TEST_ASSERT_TRUE(k.logik.protokoll().back().find("HILFERUF") != std::string::npos);
}

// ---------------- Bedienung ----------------

void test_uhr_stellen_lehnt_ungueltige_eingaben_ab() {
    Kasse k;
    k.logik.befehl("T 25:00:00");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("FEHLER: Ungueltige Uhrzeit."));
    k.logik.befehl("T ab:cd:ef");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("FEHLER: Format"));
}

void test_unbekannter_befehl_zeigt_hilfe_hinweis() {
    Kasse k;
    k.logik.befehl("Q");
    TEST_ASSERT_TRUE(k.anzeige.enthaelt("FEHLER: Unbekannter Befehl."));
}

void test_dummy_anzeige_wenn_meldungen_keine_rolle_spielen() {
    // Hier interessiert nur die Gebuehr. Die Anzeige wird nicht gebraucht -> Dummy.
    UhrFake uhr;
    ParkSpeicherFake speicher;
    AnzeigeDummy dummy;
    KasseLogik logik(uhr, speicher, dummy);
    logik.einfahrt("ZH54321");
    uhr.vorspulenSekunden(61);
    TEST_ASSERT_EQUAL_UINT32(120, logik.gebuehrRappen("ZH54321"));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_TC01_einfahrt_speichert_kennzeichen_mit_datum_und_uhrzeit);
    RUN_TEST(test_TC02_einfahrtszeit_entspricht_exakt_der_uhrzeit);
    RUN_TEST(test_TC03_leeres_kennzeichen_wird_abgelehnt_ohne_parkvorgang);
    RUN_TEST(test_TC05_ungueltiges_kennzeichen_wird_abgelehnt_und_protokolliert);
    RUN_TEST(test_TC06_parkvorgang_erscheint_in_der_liste_mit_status);
    RUN_TEST(test_TC07_parkvorgang_bleibt_nach_zwei_minuten_bestehen);
    RUN_TEST(test_TC18_doppelte_einfahrt_wird_abgelehnt);
    RUN_TEST(test_TC19_elftes_fahrzeug_wird_bei_vollem_parkhaus_abgelehnt);
    RUN_TEST(test_TC19_nach_einer_ausfahrt_ist_wieder_ein_platz_frei);
    RUN_TEST(test_TC08_gebuehr_nach_302_sekunden_ist_chf_1_60);
    RUN_TEST(test_TC09_kurzer_und_langer_aufenthalt);
    RUN_TEST(test_TC09_zwei_stunden_parkdauer_mit_vorbereitetem_fake);
    RUN_TEST(test_TC10_erfolgreiche_zahlung_markiert_als_bezahlt);
    RUN_TEST(test_TC10_F008_bezahlter_betrag_bleibt_fix);
    RUN_TEST(test_TC11_fehlgeschlagene_zahlung_bleibt_unbezahlt);
    RUN_TEST(test_TC13_unbezahlte_ausfahrt_wird_mit_betrag_abgelehnt);
    RUN_TEST(test_TC14_ausfahrt_schliesst_parkvorgang_mit_ausfahrtszeit_ab);
    RUN_TEST(test_TC17_ausfahrt_ohne_einfahrt_wird_abgelehnt);
    RUN_TEST(test_protokoll_einfahrt_ok_und_abgelehnt);
    RUN_TEST(test_TC20_hilferuf_wird_angezeigt_und_protokolliert);
    RUN_TEST(test_uhr_stellen_lehnt_ungueltige_eingaben_ab);
    RUN_TEST(test_unbekannter_befehl_zeigt_hilfe_hinweis);
    RUN_TEST(test_dummy_anzeige_wenn_meldungen_keine_rolle_spielen);
    return UNITY_END();
}
