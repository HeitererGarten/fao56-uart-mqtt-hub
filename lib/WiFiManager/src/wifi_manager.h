#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "config.h"

class WiFiManager {
public:
    WiFiManager(HubConfig* config);
    bool connect();
    bool isConnected();
    void setupAP(const char* ssid = "MQTT-Hub-Config", const char* password = "admin@123");

private:
    HubConfig* config;
};