#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <LoRa.h>
#include "carte_SD.h"
#include "mesureMS5611.h"
#include "mesureBMI088.h"
#include "emetteur.h"
#include "RunCam.h"
#include "GPS.h"

#define FIN_DE_VOL 900000
#define PACKET_WINDOW_SIZE 5
#define SERIAL_BAUD 115200
#define TAILLE_TABLEAU 20 // Définir la taille du tableau de données
#define TAILLE_TABLEAU_Lora 11
elapsedMillis temps_de_vol;
float_t data[TAILLE_TABLEAU]; // tableau de données à écrire dans le fichier CSV
// bool depart = 0;
bool video=0; // variable pour stocker l'état de la caméra (0 = arrêtée, 1 = enregistrement)
int compteur = 0; // variable pour compter le nombre de mesures prises
bool gpsFixSignale = false;

RunCam camera(Serial5);  // camera branché sur RX5, TX5 
const int RED_1 = 35; // modifiable selon le câblage
const int BLUE_1 = 36; // modifiable selon le câblage
const int GREEN_1 = 34; // modifiable selon le câblage


// ======================= SETUP =======================
void setup_mesureBMI088(); 
void setup_mesureMS5611();
void setup_emetteur();
void setup_camera(){
  bool ok = camera.begin();
  if (ok) {
    Serial.println("RunCam prête (device info OK)");
    Serial.print("RunCam start/stop explicites: ");
    Serial.println(camera.usesExplicitStartStop() ? "oui" : "non (repli sur toggle)");
  } else {
    Serial.println("RunCam prête (pas de reponse device info -> mode degrade, toggle uniquement)");
  }
}
void setup_GPS();

// ======================= FUNCTIONS =======================
void led1(bool r, bool g, bool b) {
  digitalWrite(RED_1, r); digitalWrite(GREEN_1, g); digitalWrite(BLUE_1, b);
}
void mesureBMI088(float_t mesures[6]); 
void mesureGPS(float_t mesures[12]);
void packetLoRa(float_t data[], size_t taille);
float_t *tableau_mesures(uint32_t temps_de_vol)
{
   data[0] = temps_de_vol;
    mesureMS5611(data[1]);
    mesureBMI088(data + 2);
    mesureGPS(data + 8);
   return data;
}
void enregistrementterminé() {
  LoRa.sleep();
  logFile.close();
  camera.stopRecording();   

  led1(0, 0, 1); // bleu
  while(1);
  
}



// ======================= void setup() =======================
void setup()
{
  Serial.begin(SERIAL_BAUD);
  delay(6000);// while (!Serial) delay(10); 
  Serial.println("\n[STARTUP] Initializing...");
  
  Wire2.begin(); // SDA, SCL : 24, 25
  setup_mesureMS5611();
  setup_mesureBMI088();
  carte_SD_setup();
  setup_emetteur();
  setup_camera(); 
  setup_GPS();

  pinMode(RED_1, OUTPUT);pinMode(BLUE_1, OUTPUT);pinMode(GREEN_1, OUTPUT);
  led1(1, 0, 0); // rouge


  temps_de_vol=0;

  // ---- Démarrage caméra + check pré-vol (fait ici plutôt qu'en vol pour
  //      absorber le delay() bloquant de checkRecordingActive() AVANT le
  //      décollage, pas pendant) ----
  camera.startRecording(); 
  int8_t recOk = camera.checkRecordingActive(2000);
  if (recOk == 1) {
    Serial.println("[PREFLIGHT] Enregistrement confirme actif");
  } else if (recOk == 0) {
    Serial.println("[PREFLIGHT] Enregistrement NON confirme -> nouvelle tentative");
    camera.startRecording();
  } else {
    Serial.println("[PREFLIGHT] Verification impossible (feature non supportee ou pas de reponse)");
  }

  
}

// ======================= void loop() =======================

void loop()
{
  // Serial.print("Temps: ");
  // Serial.print(temps_de_vol);
  // Serial.println(" ms");

  if(video == 0) led1(0, 1, 0); // vert
  camera.startRecording(); 
  camera.keepAlive(); // réassertion périodique (non bloquant, s'auto-limite à 1x/3s)
  float_t *mesures = tableau_mesures(temps_de_vol); // remplir le tableau de mesures avec les données actuelles 
  if(!gpsFixSignale && mesures[8]!=0 && mesures[9]!=0 && mesures[10]!=0){
    led1(0, 1, 0); // vert
    gpsFixSignale = true;
  } 
  carte_SD_loop(mesures, TAILLE_TABLEAU, temps_de_vol); // écrire les données dans le fichier CSV
  if (compteur >= PACKET_WINDOW_SIZE) {
    packetLoRa(mesures, TAILLE_TABLEAU_Lora); // envoyer les données via LoRa
    compteur = 0; // réinitialiser le compteur après l'envoi
  } else {
    delay(1);
    compteur++; // incrémenter le compteur si on n'a pas encore atteint la taille de la fenêtre
  }
  if (logFile) {
     logFile.flush();
  }
  if(temps_de_vol>=FIN_DE_VOL) enregistrementterminé();    
    
}

  
 

  


