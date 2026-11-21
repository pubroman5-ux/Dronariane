#include <SPI.h>
#include <LoRa.h>
#include "packet.h"

int counter = 0;

void setup_emetteur() {

  LoRa.setPins(10, 2, 19);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(865788000)) { // initialize ratio at 865.788 MHz
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  LoRa.setTxPower(14);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSpreadingFactor(10);
  LoRa.setPreambleLength(8);
}

void packetLoRa(float_t data[], size_t taille) {
  TelemetryPacket pkt;
  pkt.counter = counter;
  for (size_t i = 0; i < taille && i < 11; i++) {
    pkt.data[i] = data[i];
  }

  Serial.print("Sending packet: ");
  Serial.println(counter);

  LoRa.beginPacket();
  LoRa.write((uint8_t*)&pkt, sizeof(TelemetryPacket));
  LoRa.endPacket();

  counter++;
}

