# include <Arduino.h>
#include <Wire.h> 
# include "mesureMS5611.h"
# include "mesureBMI088.h"

// // Bmi088Accel accel(SPI,10);
void setup_mesureBMI088(); 
void loop_mesureBMI088();

void setup() {
    Serial.begin(115200);
    delay(6000);
    Serial.println("--- DEMARRAGE REUSSI ---");
    
    Wire.begin(1, 2); // Wire.begin(SDA, SCL) - adjust ESP32 pins if needed
    
    setup_mesureMS5611();
    setup_mesureBMI088();
}

void loop() {
    loop_mesureMS5611();
    loop_mesureBMI088();

    delay(100); // Ajuste ce délai selon tes besoins
}

// #include <Wire.h>
// #include <Arduino.h>

// void setup() {
//   Serial.begin(115200);
//   delay(2000);
//   // Remplace 1 et 2 par les pins que tu utilises actuellement
//   Wire.begin(1, 2); 
//   Serial.println("\nScan I2C en cours...");
// }

// void loop() {
//   byte error, address;
//   int nDevices = 0;
//   for(address = 1; address < 127; address++ ) {
//     Wire.beginTransmission(address);
//     error = Wire.endTransmission();
//     if (error == 0) {
//       Serial.print("Appareil trouve a l'adresse 0x");
//       if (address<16) Serial.print("0");
//       Serial.println(address,HEX);
//       nDevices++;
//     }
//   }
//   if (nDevices == 0) Serial.println("Aucun appareil I2C trouve\n");
//   delay(5000);
// }

// #include <Arduino.h>

// void setup() {
//   Serial.begin(115200);
//   delay(6000);
// //   Serial.println("\n\nBoot ESP32");

// //   pinMode(8, OUTPUT);
// //   digitalWrite(8, LOW);

//   Serial.println("ESP32 demarre");
// }

// void loop() {
//   Serial.println("Hello");
//   delay(1000);
// //   digitalWrite(8, LOW);
  
// }