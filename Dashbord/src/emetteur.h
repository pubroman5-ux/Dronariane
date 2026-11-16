#include <LoRa.h>
typedef float float_t;
typedef size_t size_t;

extern char cmd; // variable pour stocker la commande reçue via LoRa

#ifndef emetteur_H
#define emetteur_H


void setup_emetteur();
void packetLoRa(float_t data[], size_t taille);

#endif