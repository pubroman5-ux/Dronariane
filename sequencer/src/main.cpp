// #include "KalmanFilter.h"
// #include "mesureBMI088sequencer.h"
#include "mesureMS5611.h"
#include <Arduino.h>
#include <Wire.h> 

// KalmanFilter kf(0.01);   Δt = 10 ms 
#define delai_sens_armement 750
#define vitesse_armement 255

// Broches L298N
const int ARMER = 0;  // PWM
const int ENA = 15;  // PWM
const int IN1 = 23;
const int IN2 = 22;
const int JACK = 6;
const int RED_1 = 21; 
const int RED_2 = 18;
const int BLUE_1 = 19;
const int BLUE_2 = 13;
const int GREEN_1 = 20;
const int GREEN_2 = 9;
//Led front panel
const int RED_3 = 7;  
const int BLUE_3 = 3;
const int GREEN_3 = 11;

// Paramètres PWM
const int freq = 5000;
const int resolution = 8;

const int Te = 10; // Temps d'échantillonnage en ms
const int duree_de_vol = 5700; // 7500 ms = 7.5 secondes
int temps_de_vol = 0;
float lastAltitude = 0;
float NouvelleAltitude = 0;
int consecutive_drop = 0;
const int tempsavantdeploiment = 5500;

// Fonctions
void led1(bool r, bool g, bool b) {
  digitalWrite(RED_1, r); digitalWrite(GREEN_1, g); digitalWrite(BLUE_1, b);
}
void led2(bool r, bool g, bool b) {
  digitalWrite(RED_2, r); digitalWrite(GREEN_2, g); digitalWrite(BLUE_2, b);
}
void led3(bool r, bool g, bool b) {
  digitalWrite(RED_3, r); digitalWrite(GREEN_3, g); digitalWrite(BLUE_3, b);
}
void deploiment() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  analogWrite(ENA, vitesse_armement);
  delay(delai_sens_armement);
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
}

void armer() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  analogWrite(ENA, vitesse_armement);
  delay(delai_sens_armement);
  analogWrite(ENA, 0);
}



void setup()
{
    // Serial.begin(115200);
    // delay(2000);
    // Serial.println("--- DEMARRAGE REUSSI ---");

    // Wire.begin(1, 2);
    // setup_mesureMS5611();

    pinMode(IN1, OUTPUT);pinMode(IN2, OUTPUT); pinMode(ENA, OUTPUT);
    pinMode(JACK, INPUT_PULLUP);
    pinMode(ARMER, INPUT_PULLUP);

    pinMode(RED_1, OUTPUT);pinMode(BLUE_1, OUTPUT);pinMode(GREEN_1, OUTPUT);
    pinMode(RED_2, OUTPUT);pinMode(BLUE_2, OUTPUT);pinMode(GREEN_2, OUTPUT);
    pinMode(RED_3, OUTPUT);pinMode(BLUE_3, OUTPUT);pinMode(GREEN_3, OUTPUT);

}


void loop()

{
  analogWrite(ENA, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  led1(1, 0, 0);  // rouge
  led2(0, 0, 1);  // rouge
  led3(1, 0, 0);  // rouge

  while(digitalRead(JACK) == HIGH){
    if (digitalRead(ARMER) == HIGH) {
      armer();
    }
  }
  

    led1(0, 1, 0);  // vert
    led3(0, 0, 1);  // bleu (fusée en vol)
  
  unsigned long debut = millis();
  bool annule = false;

  while (millis() - debut < duree_de_vol) {
    if (digitalRead(JACK) == HIGH) {  // jack rebranché
      annule = true;
      break;
    }
    delay(50);
  }

  if (annule) return;  // repart au début du loop()
  
  led2(0, 1, 0);  // vert (parachute déployé par le timer)
  
        
  // Déploiement du parachute      
  deploiment();

  led2(0, 1, 0);  // vert (parachute déployé par le timer)
  led3(0, 1, 0);  // vert (parachute déployé)
  while(true);
          
}


//  -- END OF FILE --