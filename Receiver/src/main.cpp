#include <SPI.h>
#include <LoRa.h>
#include <SD.h>
#include <Wire.h>
#include "ecranOLED.h"
// #include "carte_SD.h"
#include "packet.h"

bool evenement=0;
char cmd='s';
float_t data[11];
#define PIN_interrupteur 4
#define LoRa_CS 16
#define LoRa_MOSI 7
#define LoRa_MISO 2
#define LoRa_SCK 6
#define LoRa_RST 9
#define LoRa_DIO0 0



void ecriture_ecran(float_t data[11],float_t ecran[11]) {
  for (int i = 0; i < 11; i++) {
    ecran[i] = data[i];
  }
  ecran[7] = data[3];
  ecran[9] = data[4];
  ecran[8] = data[6];
  ecran[10] = data[7];
  ecran[3] = data[5];
  ecran[4] = data[8];
  ecran[6] = data[10];
  ecran[5] = data[9];

  float_t ecran1[7]={ecran[0], ecran[1], ecran[2], ecran[3], ecran[4], ecran[5], ecran[6]};
  float_t ecran2[2]={ecran[7], ecran[8]};
  float_t ecran3[2]={ecran[9], ecran[10]};
  affichage_ecran(ecran1, ecran2,  ecran3);
}

void setup() {
  Serial.begin(115200);
  delay(6000);

  Serial.println("--- Début du Setup ---");
  SPI.begin(LoRa_SCK, LoRa_MISO, LoRa_MOSI, LoRa_CS);  
  LoRa.setPins(LoRa_CS, LoRa_RST, LoRa_DIO0); 
  pinMode(PIN_interrupteur, INPUT);

  Serial.println("LoRa Receiver");

  if (!LoRa.begin(865788000)) { // Initialize LoRa with frequency 865.788 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("Starting LoRa SUCCESS!");
  LoRa.setTxPower(14,PA_OUTPUT_PA_BOOST_PIN);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(10);
  LoRa.setPreambleLength(8);

  setup_ecran();
  // carte_SD_setup();
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    if (packetSize == sizeof(TelemetryPacket)) {
      TelemetryPacket pkt;
      LoRa.readBytes((uint8_t*)&pkt, sizeof(TelemetryPacket));

      Serial.print("Packet #");
      Serial.print(pkt.counter);
      Serial.print(" data: ");
      Serial.print(pkt.data[0],0);
      Serial.print(";");
      for (size_t i = 1; i <= 8; i++) {
        Serial.print(pkt.data[i]);
        Serial.print(";");
      }
      for (size_t i = 9; i <= 10; i++) {
        Serial.print(pkt.data[i],6);
        Serial.print(";");
      }
      Serial.print(" RSSI ");
      Serial.println(LoRa.packetRssi());

      // carte_SD_loop(data, 11);
      float_t ecran[11] = {0.0};
      ecriture_ecran(pkt.data, ecran);
      
      if(pkt.data[0]>= 673000 && pkt.data[0]<= 674000){
        Serial.println("***********Nouvel video demarrer****************************************************");
      }
      if(pkt.data[0]>= 1346000 && pkt.data[0]<= 1347000){
        Serial.println("***********Nouvel video demarrer****************************************************");
      }
    } else {
      // paquet corrompu/tronqué -> on l'ignore proprement
      Serial.print("Taille de paquet inattendue: ");
      Serial.println(packetSize);
      while (LoRa.available()) LoRa.read(); // vider le buffer
    }
  }
}

