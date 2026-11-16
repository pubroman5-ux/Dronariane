#include <Adafruit_SSD1306.h>

#define nombreDePixelsEnLargeur 128         // Taille de l'écran OLED, en pixel, au niveau de sa largeur
#define nombreDePixelsEnHauteur 64          // Taille de l'écran OLED, en pixel, au niveau de sa hauteur
#define brocheResetOLED         -1          // Reset de l'OLED partagé avec l'Arduino (d'où la valeur à -1, et non un numéro de pin)
#define adresseI2CecranOLED     0x3C        // Adresse de "mon" écran OLED sur le bus i2c (généralement égal à 0x3C ou 0x3D)
#define tailleDeCaractere       1           // Taille des caractères affichés sur l'écran OLED (1:1, puis 2:1, puis 3:1)
Adafruit_SSD1306 ecranOLED(128, 64, &Wire, -1);
#define SDA_PIN 11
#define SCL_PIN 10

// ========================
// Initialisation programme
// ========================
void setup_ecran() {
    
    Wire.begin(SDA_PIN, SCL_PIN);  // SDA=GPIO11, SCL=GPIO10

  // Initialisation de l'écran OLED
  if (!ecranOLED.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
        Serial.println("ECHEC init OLED");
        while(1);
    }
    Serial.println("OLED OK");

  
    ecranOLED.clearDisplay();                                   // Effaçage de l'intégralité du buffer
    ecranOLED.setTextSize(tailleDeCaractere);                   // Taille des caractères (1:1, puis 2:1, puis 3:1)
    ecranOLED.setCursor(0, 0);                                  // Déplacement du curseur en position (0,0), c'est à dire dans l'angle supérieur gauche

  
    ecranOLED.setTextColor(SSD1306_WHITE);                  // Affichage du texte en "blanc" (avec la couleur principale, en fait, car l'écran monochrome peut être coloré)
    ecranOLED.println("Temps");
    ecranOLED.println("Altitude");
    ecranOLED.println("ax,ay,az");
    ecranOLED.println("wx,wy,wz");
    ecranOLED.println("GPS Alt");
    ecranOLED.println("GPS Lat");
    ecranOLED.println("GPS Long");
    ecranOLED.display();                            // Transfert le buffer à l'écran
    delay(2000);
  
}

// =================
// Boucle principale
// =================
void affichage_ecran(float_t data1[7], float_t data2[3], float_t data3[3]) {
  
  for(byte numeroDeLigne=0; numeroDeLigne<=6; numeroDeLigne++) {    
    ecranOLED.setCursor(50, numeroDeLigne*8.2);
      ecranOLED.setTextColor(SSD1306_WHITE, SSD1306_BLACK);  
      int decimales=1;
      if(numeroDeLigne==5 || numeroDeLigne==6) decimales=6;           
      ecranOLED.print(data1[numeroDeLigne],decimales);
      
    }
  for(byte numeroDeLigne=0; numeroDeLigne<=1; numeroDeLigne++) {    
    ecranOLED.setCursor(75, (numeroDeLigne+2)*8.2);
      ecranOLED.setTextColor(SSD1306_BLACK, SSD1306_WHITE);                  
      ecranOLED.print(data2[numeroDeLigne],1);
    }
  for(byte numeroDeLigne=0; numeroDeLigne<=1; numeroDeLigne++) {    
    ecranOLED.setCursor(102, (numeroDeLigne+2)*8.2);
      ecranOLED.setTextColor(SSD1306_WHITE, SSD1306_BLACK);                 
      ecranOLED.print(data3[numeroDeLigne],1);
    }
  ecranOLED.display(); 
}
