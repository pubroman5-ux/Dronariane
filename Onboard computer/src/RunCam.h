#pragma once
#include <Arduino.h>

// ---- Command IDs (RunCam Device Protocol officiel) ----
#define RC_CMD_GET_DEVICE_INFO      0x00
#define RC_CMD_CAMERA_CONTROL       0x01
#define RC_CMD_READ_SETTING_DETAIL  0x11

// ---- Setting ID utile pour vérifier que l'enregistrement progresse ----
#define RC_SETTING_REMAIN_RECORDING_TIME 4

// ---- Actions pour la commande CAMERA_CONTROL (0x01) ----
#define RC_ACTION_WIFI_BTN       0x00
#define RC_ACTION_POWER_BTN      0x01
#define RC_ACTION_CHANGE_MODE    0x02
#define RC_ACTION_START_RECORD   0x03
#define RC_ACTION_STOP_RECORD    0x04

// ---- Bits de "Feature" retournés par GET_DEVICE_INFO ----
#define RC_FEATURE_POWER_BTN      (1 << 0)
#define RC_FEATURE_WIFI_BTN       (1 << 1)
#define RC_FEATURE_CHANGE_MODE    (1 << 2)
#define RC_FEATURE_5KEY_OSD       (1 << 3)
#define RC_FEATURE_SETTINGS       (1 << 4)
#define RC_FEATURE_DISPLAYPORT    (1 << 5)
#define RC_FEATURE_START_RECORD   (1 << 6)
#define RC_FEATURE_STOP_RECORD    (1 << 7)

class RunCam {
public:
    RunCam(HardwareSerial &serial, uint32_t baud = 115200);

    // Initialise le port série et interroge la caméra (GET_DEVICE_INFO, avec retries).
    // Retourne true si la caméra a répondu correctement (CRC valide).
    // Détecte au passage si les actions START/STOP explicites sont supportées ;
    // sinon on retombe sur le toggle "bouton power" (comportement d'origine).
    bool begin();

    void startRecording();
    void stopRecording();
    void capturePhoto(); // NB: pas d'action "photo" documentée officiellement, voir commentaire dans le .cpp

    bool isRecording() const { return _recording; }
    bool usesExplicitStartStop() const { return _explicitStartStop; }
    uint16_t features() const { return _features; }

    // A appeler régulièrement dans loop() (ex: à chaque itération, elle se
    // charge elle-même de ne réémettre que toutes les ~3s).
    // Comme la commande de contrôle caméra n'a AUCUN accusé de réception,
    // c'est ce qui protège contre un paquet perdu/corrompu par du bruit EMI
    // (LoRa, moteur) ou un reboot caméra suite à un creux d'alimentation.
    // Ne fait rien si la caméra ne supporte pas le start/stop explicite
    // (renvoyer un toggle en boucle serait dangereux : ça arrêterait
    // l'enregistrement un coup sur deux).
    void keepAlive();

    // Vérifie que l'enregistrement PROGRESSE réellement, en lisant deux fois
    // le "temps d'enregistrement restant" côté caméra, espacées de
    // sampleWindowMs. Nécessite RC_FEATURE_SETTINGS (vérifié en interne).
    // ATTENTION: fonction BLOQUANTE (delay(sampleWindowMs) inclus) -> à
    // réserver à un check pré-vol dans setup(), pas à la boucle de vol.
    // Retour : 1 = progression détectée (ça enregistre vraiment),
    //          0 = valeur inchangée (probablement arrêté),
    //         -1 = pas de réponse caméra / feature non supportée.
    int8_t checkRecordingActive(uint32_t sampleWindowMs = 3000);

private:
    HardwareSerial &_serial;
    uint32_t _baud;
    bool _recording = false;
    bool _explicitStartStop = false;
    uint16_t _features = 0;
    uint32_t _lastKeepAlive = 0;

    uint8_t crc8_dvb_s2(uint8_t crc, uint8_t data);
    uint8_t calcCRC(uint8_t *data, uint8_t len);

    void sendControl(uint8_t action, uint8_t repeats = 2);
    bool queryDeviceInfo(uint8_t retries, uint16_t timeoutMs);
    bool readSettingDetail(uint8_t settingId, uint8_t *outValue, uint8_t maxLen,
                            uint8_t &outLen, uint16_t timeoutMs = 300);
    void flushRx();
};
