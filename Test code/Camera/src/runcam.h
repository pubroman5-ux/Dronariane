#pragma once
#include <Arduino.h>

#define RC_START_RECORDING  0x01
#define RC_CAPTURE_PHOTO    0x02
#define RC_WIFI_TOGGLE      0x03
#define RC_POWER_TOGGLE     0x04
#define RC_CHANGE_MODE      0x05

class RunCam {
public:
    RunCam(HardwareSerial &serial, uint32_t baud = 115200);
    void begin();
    void send(uint8_t action);
    void startRecording();
    void stopRecording();
    void capturePhoto();
    bool isRecording() { return _recording; }

private:
    HardwareSerial &_serial;
    uint32_t _baud;
    bool _recording = false;

    uint8_t crc8_dvb_s2(uint8_t crc, uint8_t data);
    uint8_t calcCRC(uint8_t *data, uint8_t len);
};