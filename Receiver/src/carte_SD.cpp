#include <Arduino.h>
#define SPI_DRIVER_SELECT 2   // active le driver SPI logiciel SoftSpiDriver
#include <SdFat.h>
#include "carte_SD.h"

#define SD_SCK   18
#define SD_MISO  19
#define SD_MOSI  20
#define SD_CS    21

#define LOG_PREFIX  "/DATA"
#define LOG_EXT     ".CSV"
#define MAX_FILES   9999

SdFat32 SD;   // remplace l'objet SD standard
File32 logFile;  // utile lorsqu'on veut garder le fichier ouvert pour plusieurs écritures (écritures rapides), mais dans ce cas il faut aussi gérer la fermeture du fichier à la fin du programme ou en cas d'erreur
char filename[20];   // ex: /LOG0001.TXT
typedef float float_t;
SoftSpiDriver<SD_MISO, SD_MOSI, SD_SCK> softSpi;
#define SD_CONFIG SdSpiConfig(SD_CS, DEDICATED_SPI, SD_SCK_MHZ(4), &softSpi)


// ======================= UTIL =======================

void halt(const char *msg)
{
  Serial.println("\n[FATAL ERROR]");
  Serial.println(msg);
  while (1) delay(1000);
}

// ======================= SD =======================

bool initSD() {
  Serial.print("[SD] Init (soft SPI)... ");
  if (!SD.begin(SD_CONFIG)) {
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
      File32 f = SD.open(filename, FILE_WRITE);
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
    for (size_t i = 0; i < taille; i++) {
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

  File32 f = SD.open(name);
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

// void printDirectory(File32 dir, uint8_t level)
// {
//   while (true)
//   {
//     File32 entry = dir.openNextFile();
//     if (!entry) break;

//     for (uint8_t i = 0; i < level; i++) Serial.print("  ");

//     Serial.print(entry.name());
//     if (entry.isDirectory())
//     {
//       Serial.println("/");
//       printDirectory(entry, level + 1);
//     }
//     else
//     {
//       Serial.print("  ");
//       Serial.print(entry.size());
//       Serial.println(" bytes");
//     }
//     entry.close();
//   }
// }


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
    logFile.println("Temps;Altitude;Acceleration selon x;Acceleration selon y;Acceleration selon z;Vitesse angulaire selon x;Vitesse angulaire selon y;Vitesse angulaire selon z"); // En-tête du fichier CSV
  }
}

void carte_SD_loop(float_t data[], size_t taille)
{

  // ajouter une mesure dans un fichier CSV
  ecrireDonnees(data, taille); 
  Serial.println("Données écrites sur la SD");
  logFile.flush();
  
}