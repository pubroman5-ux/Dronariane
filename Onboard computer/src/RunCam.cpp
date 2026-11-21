#include "RunCam.h"

RunCam::RunCam(HardwareSerial &serial, uint32_t baud)
    : _serial(serial), _baud(baud) {}

uint8_t RunCam::crc8_dvb_s2(uint8_t crc, uint8_t data) {
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
    }
    return crc;
}

uint8_t RunCam::calcCRC(uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) crc = crc8_dvb_s2(crc, data[i]);
    return crc;
}

void RunCam::flushRx() {
    while (_serial.available()) _serial.read();
}

bool RunCam::begin() {
    _serial.begin(_baud);
    delay(2000); // laisser la caméra terminer son boot firmware

    bool ok = queryDeviceInfo(5, 300); // 5 tentatives, 300 ms de timeout chacune

    if (ok) {
        _explicitStartStop = (_features & RC_FEATURE_START_RECORD) &&
                              (_features & RC_FEATURE_STOP_RECORD);
        Serial.print("[RunCam] Reponse OK, features = 0x");
        Serial.println(_features, HEX);
        Serial.println(_explicitStartStop
            ? "[RunCam] Start/Stop explicites supportes -> utilises"
            : "[RunCam] Start/Stop explicites NON signales -> repli sur toggle bouton power");
    } else {
        Serial.println("[RunCam] Aucune reponse au GET_DEVICE_INFO apres 5 tentatives.");
        Serial.println("[RunCam] -> repli sur toggle bouton power (comportement d'origine).");
        _explicitStartStop = false;
    }
    return ok;
}

bool RunCam::queryDeviceInfo(uint8_t retries, uint16_t timeoutMs) {
    uint8_t req[3];
    req[0] = 0xCC;
    req[1] = RC_CMD_GET_DEVICE_INFO;
    req[2] = calcCRC(req, 2);

    for (uint8_t attempt = 0; attempt < retries; attempt++) {
        flushRx();
        _serial.write(req, 3);

        uint8_t resp[5];
        uint8_t got = 0;
        uint32_t t0 = millis();
        while (millis() - t0 < timeoutMs && got < 5) {
            if (_serial.available()) {
                resp[got++] = _serial.read();
            }
        }

        if (got == 5 && resp[0] == 0xCC) {
            uint8_t crcCalc = calcCRC(resp, 4);
            if (crcCalc == resp[4]) {
                // ATTENTION: la doc RunCam précise juste "uint16_t" pour le champ
                // Feature, sans donner l'ordre des octets sur le fil. J'ai mis
                // big-endian (MSB d'abord) par défaut ci-dessous. Si le flag
                // affiché au boot ne correspond à aucune combinaison plausible
                // (ex: > 0x3FF), inverse en (resp[3] << 8) | resp[2].
                _features = ((uint16_t)resp[2] << 8) | resp[3];
                return true;
            }
        }
        delay(100);
    }
    return false;
}

void RunCam::sendControl(uint8_t action, uint8_t repeats) {
    uint8_t packet[4];
    packet[0] = 0xCC;
    packet[1] = RC_CMD_CAMERA_CONTROL;
    packet[2] = action;
    packet[3] = calcCRC(packet, 3);

    // Pas d'accusé de réception pour cette commande dans le protocole RunCam :
    // on compense en la renvoyant plusieurs fois, espacées, plutôt qu'un seul
    // tir "à l'aveugle".
    for (uint8_t i = 0; i < repeats; i++) {
        _serial.write(packet, 4);
        if (i + 1 < repeats) delay(60);
    }
}

void RunCam::startRecording() {
    if (_explicitStartStop) {
        sendControl(RC_ACTION_START_RECORD);
    } else {
        // toggle : on ne l'envoie que si on pense être à l'arrêt, sinon
        // on inverserait l'état par erreur
        if (!_recording) sendControl(RC_ACTION_POWER_BTN);
    }
    _recording = true;
    _lastKeepAlive = millis();
}

void RunCam::stopRecording() {
    if (_explicitStartStop) {
        sendControl(RC_ACTION_STOP_RECORD);
    } else {
        if (_recording) sendControl(RC_ACTION_POWER_BTN);
    }
    _recording = false;
}

void RunCam::capturePhoto() {
    // NB: la doc officielle RunCam Device Protocol ne liste aucune action
    // "capture photo" dédiée pour la commande CAMERA_CONTROL (seulement
    // wifi/power/change_mode/start/stop). Le 0x02 utilisé dans la version
    // précédente correspond en réalité à CHANGE_MODE. A vérifier/adapter
    // selon le comportement réel observé sur ta Split 4 si tu utilises
    // cette fonction.
    sendControl(RC_ACTION_CHANGE_MODE);
}

void RunCam::keepAlive() {
    if (!_explicitStartStop) return; // dangereux de spammer un toggle
    if (!_recording) return;
    if (millis() - _lastKeepAlive < 3000) return;
    _lastKeepAlive = millis();
    sendControl(RC_ACTION_START_RECORD, 1); // simple réassertion, un seul envoi suffit ici
}

// Lit un réglage caméra via READ_SETTING_DETAIL (0x11).
// Format réponse (non garanti à 100% par la doc, à valider au banc) :
//   [0xCC][remaining_chunk][data_length][setting_type][valeur...][crc]
// data_length couvre tout entre setting_type et le dernier octet avant le crc.
bool RunCam::readSettingDetail(uint8_t settingId, uint8_t *outValue, uint8_t maxLen,
                                uint8_t &outLen, uint16_t timeoutMs) {
    if (!(_features & RC_FEATURE_SETTINGS)) return false;

    uint8_t req[5];
    req[0] = 0xCC;
    req[1] = RC_CMD_READ_SETTING_DETAIL;
    req[2] = settingId;
    req[3] = 0; // chunk index 0
    req[4] = calcCRC(req, 4);

    flushRx();
    _serial.write(req, 5);

    // header + remaining_chunk + data_length
    uint8_t head[3];
    uint8_t got = 0;
    uint32_t t0 = millis();
    while (millis() - t0 < timeoutMs && got < 3) {
        if (_serial.available()) head[got++] = _serial.read();
    }
    if (got < 3 || head[0] != 0xCC) return false;

    uint8_t dataLen = head[2];
    if (dataLen == 0 || dataLen > 60) return false;

    uint8_t payload[61];
    got = 0;
    t0 = millis();
    while (millis() - t0 < timeoutMs && got < (uint16_t)(dataLen + 1)) { // +1 = crc
        if (_serial.available()) payload[got++] = _serial.read();
    }
    if (got < (uint8_t)(dataLen + 1)) return false;

    uint8_t crcReceived = payload[dataLen];
    uint8_t full[64];
    memcpy(full, head, 3);
    memcpy(full + 3, payload, dataLen);
    if (calcCRC(full, 3 + dataLen) != crcReceived) return false;

    // payload[0] = setting type, payload[1..] = valeur (STRING: null-terminated)
    uint8_t copyLen = (dataLen - 1 < maxLen) ? (dataLen - 1) : maxLen;
    memcpy(outValue, payload + 1, copyLen);
    outLen = copyLen;
    return true;
}

int8_t RunCam::checkRecordingActive(uint32_t sampleWindowMs) {
    if (!(_features & RC_FEATURE_SETTINGS)) {
        Serial.println("[RunCam] Feature 'settings access' non signalee -> verif impossible");
        return -1;
    }

    char t1[32] = {0}, t2[32] = {0};
    uint8_t len1 = 0, len2 = 0;

    if (!readSettingDetail(RC_SETTING_REMAIN_RECORDING_TIME, (uint8_t *)t1, sizeof(t1) - 1, len1)) {
        Serial.println("[RunCam] Pas de reponse a la 1ere lecture du temps restant");
        return -1;
    }
    delay(sampleWindowMs); // bloquant : voir avertissement dans le .h

    if (!readSettingDetail(RC_SETTING_REMAIN_RECORDING_TIME, (uint8_t *)t2, sizeof(t2) - 1, len2)) {
        Serial.println("[RunCam] Pas de reponse a la 2eme lecture du temps restant");
        return -1;
    }

    t1[len1] = '\0';
    t2[len2] = '\0';

    Serial.print("[RunCam] Temps restant: t0=");
    Serial.print(t1);
    Serial.print("  t0+");
    Serial.print(sampleWindowMs);
    Serial.print("ms=");
    Serial.println(t2);

    return (strcmp(t1, t2) != 0) ? 1 : 0;
}
