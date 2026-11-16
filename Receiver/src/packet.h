#ifndef PACKET_H
#define PACKET_H

#include <stdint.h>

#pragma pack(push, 1)  // pas de padding, taille garantie identique sur les deux MCU
struct TelemetryPacket {
  uint16_t counter;
  float data[11];
};
#pragma pack(pop)

#endif