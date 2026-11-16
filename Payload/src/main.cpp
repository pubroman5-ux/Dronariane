#include <Arduino.h>
#include <SPI.h>
#include "mesureBMI088.h"
#include "carte_SD.h"

#define SD_SCK 0
#define SD_MISO 4
#define SD_MOSI 5
#define SD_CS 18
#define TAILLE_TABLEAU 7
float_t data [TAILLE_TABLEAU];

unsigned long debut = 0;

float_t *tableau_mesures(unsigned long temps_de_vol){
  data[0] = temps_de_vol;
    mesureBMI088(data + 1);
  return data;
}

void setup() {
  // Serial.begin(921600);
  // delay(6000);
  // Serial.println("--- DEMARRAGE REUSSI ---");

  SPI.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
  setup_mesureBMI088();
  carte_SD_setup();
  debut = millis();

}

void loop() {
  unsigned long temps_de_vol = millis() - debut;
  Serial.print(temps_de_vol);
  float_t *mesures = tableau_mesures(temps_de_vol);
  carte_SD_loop(mesures, TAILLE_TABLEAU, temps_de_vol);
  delay(10);
}

