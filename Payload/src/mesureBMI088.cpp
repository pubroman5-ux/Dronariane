#include <Arduino.h>
#include <BMI088.h>
#include "mesureBMI088.h"
#include <vector>

Bmi088 bmi(Wire,0x19,0x69);
uint64_t time_ps;


// #define PIN_DRDY 31

// volatile bool dataReady = false;

// // La fonction ISR — appelée automatiquement sur front montant de GPIO10
// void FASTRUN onDrdy() {
//   dataReady = true;
// }


void setup_mesureBMI088()
{   
    // Serial.begin(921600);
    // delay(6000);
    // Serial.println("--- DEMARRAGE REUSSI ---");

    Wire.begin(6, 7); //  Wire.begin(SDA, SCL)
    if (bmi.begin() == true)
    {
        Serial.print("BMI found");
    }
    else
    {
        Serial.println("BMI not found. halt.");
        while (1);
    }
    Serial.println();

    bmi.setOdr(Bmi088::ODR_400HZ);
    bmi.setRange(Bmi088::ACCEL_RANGE_24G,Bmi088::GYRO_RANGE_500DPS);

    bmi.pinModeDrdy(Bmi088::BMI_PUSH_PULL,Bmi088::ACTIVE_HIGH);
    bmi.mapDrdy(Bmi088::PIN_1);

    // pinMode(PIN_DRDY, INPUT);
    // attachInterrupt(digitalPinToInterrupt(PIN_DRDY), onDrdy, RISING);
    
}

void mesureBMI088(float_t mesures[6])
{
        bmi.readSensor();
        // time_ps = bmi.getTime_ps();
        // double time_sec = (double)micros() / 1e6;


        // // Affichage avec 12 décimales pour voir les picosecondes
        // Serial.print("Tps : ");
        // Serial.print(time_sec, 12);

        // Serial.print("INT_STAT=0x"); Serial.print(intStatus, HEX);
        Serial.print(" \tAccel X: ");
        Serial.print(-bmi.getAccelX_mss());
        Serial.print(" m/s^2, Y: ");
        Serial.print(bmi.getAccelY_mss());
        Serial.print(" m/s^2, Z: ");
        Serial.print(bmi.getAccelZ_mss());
        Serial.print(" m/s^2,   ");
        Serial.print("Gyro X: ");
        Serial.print(-bmi.getGyroX_rads());
        Serial.print(" deg/s, Y: ");
        Serial.print(bmi.getGyroY_rads());
        Serial.print(" deg/s, Z: ");
        Serial.print(bmi.getGyroZ_rads());
        Serial.println(" deg/s ");

        
        mesures[0] = -bmi.getAccelX_mss();
        mesures[1] = bmi.getAccelY_mss();
        mesures[2] = bmi.getAccelZ_mss();
        mesures[3] = -bmi.getGyroX_rads();
        mesures[4] = bmi.getGyroY_rads();
        mesures[5] = bmi.getGyroZ_rads();
        
}

