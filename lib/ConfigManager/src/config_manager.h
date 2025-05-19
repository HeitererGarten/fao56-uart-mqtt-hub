#pragma once

#include <Arduino.h>
#include <SD.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include "config.h"

class ConfigManager {
public:
    ConfigManager();
    bool initStorage();
    bool loadConfig();
    bool saveConfig();
    HubConfig* getConfig();
    bool isConfigLoaded();

private:
    HubConfig config;
    bool configLoaded;
    bool sdAvailable;
    bool spiffsAvailable;
    
    bool loadFromSD();
    bool loadFromSPIFFS();
    bool parseConfigFile(File &configFile);
    bool saveToSD();
    bool saveToSPIFFS();
    bool serializeJsonToFile(File &configFile);
};