#include <Arduino.h>
#include <SPI.h>
#include <SD.h>


extern File logFile;  // utile lorsqu'on veut garder le fichier ouvert pour plusieurs écritures (écritures rapides), mais dans ce cas il faut aussi gérer la fermeture du fichier à la fin du programme ou en cas d'erreur
extern char filename[];   // ex: /LOG0001.TXT
extern float_t data[]; // tableau de données à écrire dans le fichier CSV, à adapter selon les besoins

#ifndef CARTE_SD_H
#define CARTE_SD_H

void halt(const char *msg);
bool initSD();
bool createNewLogFile(const char *prefix, const char *ext); 
void ecrireDonnees(float_t data[], size_t taille);
void dumpFile(const char *name);
void printDirectory(File dir, uint8_t level = 0);
void carte_SD_setup();
void carte_SD_loop( float_t data[], size_t taille, unsigned long temps_de_vol);

#endif