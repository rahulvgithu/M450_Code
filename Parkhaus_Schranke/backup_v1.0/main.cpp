
#include <Arduino.h>
#include <Servo.h>

const int SERVO_PIN = 8;

Servo schrankenServo;

const int WINKEL_GESCHLOSSEN = 0;
const int WINKEL_OFFEN = 90;

bool schrankeOffen = false;

void setup() {
    Serial.begin(115200);
    delay(2000);

    schrankenServo.attach(SERVO_PIN);
    schrankenServo.write(WINKEL_GESCHLOSSEN);
    delay(1000);

    Serial.println("PARKSYSTEM - PICO 2 SCHRANKE");
    Serial.println("O = Schranke oeffnen");
    Serial.println("S = Schranke schliessen");
    Serial.println("H = Hilfe");
}

void loop() {
    if (Serial.available() > 0) {
        String befehl = Serial.readStringUntil('\n');
        befehl.trim();
        befehl.toUpperCase();

        if (befehl == "O") {
            schrankenServo.write(WINKEL_OFFEN);
            schrankeOffen = true;
            Serial.println("Schranke OFFEN");
        } else if (befehl == "S") {
            schrankenServo.write(WINKEL_GESCHLOSSEN);
            schrankeOffen = false;
            Serial.println("Schranke GESCHLOSSEN");
        } else if (befehl == "H") {
            Serial.println("O = Oeffnen, S = Schliessen, H = Hilfe");
        } else {
            Serial.println("Unbekannter Befehl");
        }
    } 
}
