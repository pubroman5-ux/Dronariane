#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#ifndef GPS_H
#define GPS_H


void setup_GPS();
void mesureGPS(float_t mesures[12]);
void feedGPS(unsigned long ms); 

#endif