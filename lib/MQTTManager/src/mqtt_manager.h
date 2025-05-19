#pragma once

#include <Arduino.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include "config.h"

class MQTTManager {
public:
    MQTTManager(HubConfig* config);
    bool begin();
    bool connect();
    bool publish(const char* topic, const char* payload);
    bool isConnected();
    void loop();

private:
    WiFiClient espClient;
    PubSubClient client;
    HubConfig* config;
};