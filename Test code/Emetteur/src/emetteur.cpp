#include <SPI.h>
#include <LoRa.h>

int counter = 0;

void setup() {
  Serial.begin(115200);
  delay(6000);

  LoRa.setPins(10, 2, 19);

  Serial.println("LoRa Sender");

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
}

void packetLoRa(float_t data[], size_t taille) {
  Serial.print("Sending packet: ");
  Serial.println(counter);

  // send packet
  LoRa.beginPacket();
  for (size_t i = 0; i < taille; i++) {
      LoRa.print(data[i]);
      LoRa.print(";");
    }
    LoRa.println();
  
  LoRa.endPacket();

  delay(100);
}

void loop() {
  float_t data[3] = {1.23, 4.56, 7.89}; // Example data to send
  packetLoRa(data, 3);
  counter++;
}