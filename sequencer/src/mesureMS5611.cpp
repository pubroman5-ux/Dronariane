
#include <Arduino.h>
#include <MS5611.h>
#include "mesureMS5611.h"

#define SPIKE_THRESHOLD  5.0     // seuil anti-spike (mètres)
#define WINDOW_SIZE     10       // nombre d'échantillons pour la moyenne

MS5611 ms5611(0x76);

float altitudeBuffer[WINDOW_SIZE];
int bufferIndex = 0;
bool bufferFull = false;

float lastValidAltitude = 0;
bool firstMeasure = true;

// put function declarations here:
float getMoyenneGlissante();
float filtreAltitude(float);

float filtreAltitude(float nouvelleAltitude)
{
    // ===== 1. Anti-spike =====
    if (!firstMeasure)
    {
        if (abs(nouvelleAltitude - lastValidAltitude) > SPIKE_THRESHOLD)
        {
            // Spike détecté → on ignore la valeur
            return getMoyenneGlissante();
        }
    }
    else
    {
        firstMeasure = false;
    }

    // Valeur valide
    lastValidAltitude = nouvelleAltitude;

    // ===== 2. Mise à jour du buffer =====
    altitudeBuffer[bufferIndex] = nouvelleAltitude;
    bufferIndex++;

    if (bufferIndex >= WINDOW_SIZE)
    {
        bufferIndex = 0;
        bufferFull = true;
    }

    // ===== 3. Calcul moyenne glissante =====
    return getMoyenneGlissante();
}

// ----------------------------------------------------
float getMoyenneGlissante()
{
    float somme = 0;
    int n = bufferFull ? WINDOW_SIZE : bufferIndex;

    if (n == 0) return lastValidAltitude;

    for (int i = 0; i < n; i++)
    {
        somme += altitudeBuffer[i];
    }

    return somme / n;
}

void setup_mesureMS5611()
{
    // Serial.begin(115200);

	// delay(6000); // délai pour laisser le temps d'ouvrir le moniteur série après le reset   
    // Serial.println("--- DEMARRAGE REUSSI ---");

    Serial.println();
    Serial.println(__FILE__);
    Serial.print("MS5611_LIB_VERSION: ");
    Serial.println(MS5611_LIB_VERSION);
    Serial.println();

    //Wire.begin(1, 2);  //  adjust ESP32 pins if needed
    if (ms5611.begin() == true)
    {
        Serial.print("MS5611 found: ");
        Serial.println(ms5611.getAddress());
    }
    else
    {
        Serial.println("MS5611 not found. halt.");
        while (1);
    }
    Serial.println();

	ms5611.setOversampling(OSR_STANDARD); // 2.1 ms définit la vitesse de mesure

    Serial.println("System Ready");
}
  

void mesureMS5611(float AltitudeFiltree)
{
   float_t AltitudeBrute = 0;
    // float AltitudeFiltree = 0;
    if (ms5611.read() == 0)
    { 
        AltitudeBrute = ms5611.getAltitude();

        if(!isnan(AltitudeBrute))
        { 
            AltitudeFiltree = filtreAltitude(AltitudeBrute);
        }
    }
    
	
    Serial.print("\tAltitude: ");
    Serial.print(AltitudeFiltree);
    Serial.println(" m");
    

    /*Serial.println("Test de connexion OK !");
    delay(1000);*/
}


//  -- END OF FILE --