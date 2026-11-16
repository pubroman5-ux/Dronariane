#include "runcam.h"

RunCam::RunCam(HardwareSerial &serial, uint32_t baud)
    : _serial(serial), _baud(baud) {}

void RunCam::begin() {
    _serial.begin(_baud);
    delay(2000); // laisser la caméra démarrer
}

uint8_t RunCam::crc8_dvb_s2(uint8_t crc, uint8_t data) {
    crc ^= data;
    for (int i = 0; i < 8; i++) {
        crc = (crc & 0x80) ? (crc << 1) ^ 0xD5 : (crc << 1);
    }
    return crc;
}

uint8_t RunCam::calcCRC(uint8_t *data, uint8_t len) {
    uint8_t crc = 0;
    for (uint8_t i = 0; i < len; i++) {
        crc = crc8_dvb_s2(crc, data[i]);
    }
    return crc;
}

void RunCam::send(uint8_t action) {
    uint8_t packet[3];
    packet[0] = 0xCC;
    packet[1] = action;
    packet[2] = calcCRC(packet, 2);
    _serial.write(packet, 3);
}

void RunCam::startRecording() {
    if (!_recording) {
        send(RC_START_RECORDING);
        _recording = true;
    }
}

void RunCam::stopRecording() {
    if (_recording) {
        send(RC_START_RECORDING); // toggle
        _recording = false;
    }
}

void RunCam::capturePhoto() {
    send(RC_CAPTURE_PHOTO);
}