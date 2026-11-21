#include <Arduino.h>
#include <BMI088.h>
#include "mesureBMI088.h"
#include <vector>

Bmi088 bmi(Wire2,0x19,0x69);
uint64_t time_ps;

void setup_mesureBMI088()
{   
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
}

void mesureBMI088(float_t mesures[6])
{
        bmi.readSensor();    
        mesures[0] = bmi.getAccelX_mss();
        mesures[1] = bmi.getAccelY_mss();
        mesures[2] = bmi.getAccelZ_mss();
        mesures[3] = bmi.getGyroX_rads();
        mesures[4] = bmi.getGyroY_rads();
        mesures[5] = bmi.getGyroZ_rads();        
}

