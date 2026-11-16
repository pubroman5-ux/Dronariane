#include <Arduino.h>
#include <SD.h>
#include "carte_SD.h"

typedef float float_t;
#define SDCS 18   // SD interne Teensy 4.1

#define LOG_PREFIX  "/DATA"
#define LOG_EXT     ".CSV"
#define MAX_FILES   9999

File logFile;  // utile lorsqu'on veut garder le fichier ouvert pour plusieurs écritures (écritures rapides), mais dans ce cas il faut aussi gérer la fermeture du fichier à la fin du programme ou en cas d'erreur
char filename[20];   // ex: /LOG0001.TXT



// ======================= UTIL =======================

void halt(const char *msg)
{
  Serial.println("\n[FATAL ERROR]");
  Serial.println(msg);
  while (1) delay(1000);
}

// ======================= SD =======================

bool initSD()
{
  Serial.print("[SD] Init... ");
  if (!SD.begin(SDCS))
  {
    Serial.println("FAILED");
    return false;
  }
  Serial.println("OK");
  return true;
}

bool createNewLogFile(const char *name , const char* extension )
{
  for (uint16_t i = 1; i <= MAX_FILES; i++)
  {
    snprintf(filename, sizeof(filename), "%s%04d%s", name, i, extension);

    if (!SD.exists(filename))
    {
      File f = SD.open(filename, FILE_WRITE);
      if (!f) return false;
      f.close();

      Serial.print("[SD] New log file: ");
      Serial.println(filename);
      return true;
    }
  }
  return false;
}

// ======================= LOG =======================


void ecrireDonnees(float_t data[], size_t taille)
{
  if (logFile) 
  {
    logFile.print(data[0],0);
    logFile.print(";");
    for (size_t i = 1; i < taille; i++) {
      logFile.print(data[i]);
      logFile.print(";");
    }
    logFile.println(); // Ajoute un retour à la ligne à la fin de la ligne de données
  }
}

void dumpFile(const char *name)
{
  Serial.print("\n[SD] Dump file: ");
  Serial.println(name);

  File f = SD.open(name);
  if (!f)
  {
    Serial.println("[ERROR] Cannot open file");
    return;
  }

  while (f.available())
  {
    Serial.write(f.read());
  }
  f.close();
  Serial.println("\n[END FILE]");
}

void printDirectory(File dir, uint8_t level)
{
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry) break;

    for (uint8_t i = 0; i < level; i++) Serial.print("  ");

    Serial.print(entry.name());
    if (entry.isDirectory())
    {
      Serial.println("/");
      printDirectory(entry, level + 1);
    }
    else
    {
      Serial.print("  ");
      Serial.print(entry.size());
      Serial.println(" bytes");
    }
    entry.close();
  }
}


void carte_SD_setup()
{
  if (!initSD())
    halt("SD init failed");

  if (!createNewLogFile(LOG_PREFIX, LOG_EXT)) 
    halt("Cannot create log file");

  Serial.print("[SYSTEM] Active log file: ");
  Serial.println(filename);

  logFile = SD.open(filename, FILE_WRITE);
  if (!logFile)
  {
    halt("Cannot open log file");
  }
  if (logFile)  {
    logFile.println("Temps;Acceleration selon x;Acceleration selon y;Acceleration selon z;Vitesse angulaire selon x;Vitesse angulaire selon y;Vitesse angulaire selon z"); // En-tête du fichier CSV
  }
}

void carte_SD_loop(float_t data[], size_t taille, unsigned long temps_de_vol)
{

  // ajouter une mesure dans un fichier CSV
  ecrireDonnees(data, taille); 

  logFile.flush();   // <-- pousse réellement les données sur la carte SD
  Serial.println("Données écrites sur la SD");
  
}