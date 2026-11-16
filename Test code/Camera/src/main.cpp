#include <Arduino.h>
#include "runcam.h"

RunCam camera(Serial1); // Serial1 = pins 0/1 sur Teensy 4.1

void setup() {
    Serial.begin(115200);   // debug USB
    camera.begin();         // RunCam sur Serial1
    Serial.println("RunCam prête");
}

void loop() {
    // Démarrer l'enregistrement sur appui touche série
    if (Serial.available()) {
        char cmd = Serial.read();

        switch (cmd) {
            case 's':
                camera.startRecording();
                Serial.println("Enregistrement démarré");
                break;
            case 'x':
                camera.stopRecording();
                Serial.println("Enregistrement arrêté");
                break;
            case 'p':
                camera.capturePhoto();
                Serial.println("Photo capturée");
                break;
            default:
                Serial.println("Commandes: s=start x=stop p=photo");
                break;
        }
    }
}