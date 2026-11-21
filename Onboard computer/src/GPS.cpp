
#include <TinyGPSPlus.h>
#include <SoftwareSerial.h>

#define GPS_SERIAL Serial2
#define GPS_BAUD 115200

// The TinyGPSPlus object
TinyGPSPlus gps;

static void smartDelay(unsigned long ms);

void setup_GPS()
{
  GPS_SERIAL.begin(GPS_BAUD);
}

void mesureGPS(float_t mesures[12])
{
  static const double LONDON_LAT = 51.508131, LONDON_LON = -0.128002;

  mesures[0]=gps.altitude.meters();
  mesures[1]=gps.location.lat();
  mesures[2]=gps.location.lng();
  mesures[3]=gps.hdop.hdop();
  mesures[4]=gps.location.age();
  mesures[5]= gps.satellites.value();
  mesures[6]=gps.date.day();
  mesures[7]=gps.date.month();
  mesures[8]=gps.date.year();
  mesures[9]=gps.time.hour();
  mesures[10]=gps.time.minute();
  mesures[11]=gps.time.second();
  
  smartDelay(1);
}

// This custom version of delay() ensures that the gps object
// is being "fed".
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  do 
  {
    while (GPS_SERIAL.available())
      gps.encode(GPS_SERIAL.read());
  } while (millis() - start < ms);
}

void feedGPS(unsigned long ms)
{
  smartDelay(ms);
}