#include "serial_manager.h"

SerialManager::SerialManager(HardwareSerial* serial) {
    this->interSerial = serial;
    bytesRead = 0;
}

void SerialManager::begin(long baud, uint8_t rxPin, uint8_t txPin) {
    interSerial->begin(baud, SERIAL_8N1, rxPin, txPin);
}

bool SerialManager::readData(dhtData* data) {
    if (interSerial->available()) {
        // This code reinterprets the struct ptr as ptr to a binary blob
        bytesRead = interSerial->readBytes(reinterpret_cast<char*>(data), sizeof(dhtData));
        
        if (bytesRead == sizeof(dhtData)) {
            return true;
        }
    }
    return false;
}