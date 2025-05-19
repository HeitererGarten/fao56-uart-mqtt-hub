#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>
#include "config.h"

class SerialManager {
public:
    SerialManager(HardwareSerial* serial);
    void begin(long baud, uint8_t rxPin, uint8_t txPin);
    bool readData(dhtData* data);

private:
    HardwareSerial* interSerial;
    uint16_t bytesRead;
};