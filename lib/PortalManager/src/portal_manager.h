#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "config.h"
#include "config_manager.h"

class PortalManager {
public:
    PortalManager(HubConfig* config, ConfigManager* configManager);
    void begin();
    void handleClient();
    bool isActive() { return portalActive; }
    bool checkTrigger();

private:
    AsyncWebServer server;
    HubConfig* config;
    ConfigManager* configManager;
    bool portalActive = false;
    void setupRoutes();
};