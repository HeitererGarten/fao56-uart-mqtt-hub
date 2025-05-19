#include "config_manager.h"

ConfigManager::ConfigManager() {
    configLoaded = false;
    sdAvailable = false;
    spiffsAvailable = false;
}

bool ConfigManager::initStorage() {
    // Try SD card first
    Serial.println("Initializing SD card...");
    sdAvailable = SD.begin(SD_CS);
    if (sdAvailable) {
        Serial.println("SD card initialized.");
    } else {
        Serial.println("SD card initialization failed, falling back to internal storage.");
    }
    
    // Initialize SPIFFS as backup
    spiffsAvailable = SPIFFS.begin(true);
    if (spiffsAvailable) {
        Serial.println("SPIFFS initialized as backup storage.");
    } else {
        Serial.println("SPIFFS initialization failed!");
    }
    
    return (sdAvailable || spiffsAvailable);
}

bool ConfigManager::loadConfig() {
    bool loaded = false;
    
    // Try SD card first if available
    if (sdAvailable) {
        loaded = loadFromSD();
    }
    
    // If SD failed or unavailable, try SPIFFS
    if (!loaded && spiffsAvailable) {
        loaded = loadFromSPIFFS();
    }
    
    if (loaded) {
        Serial.println("Configuration loaded successfully");
        Serial.printf("MQTT: %s:%d\n", config.mqtt_server.c_str(), config.mqtt_port);
        Serial.printf("WiFi: %s\n", config.wifi_ssid.c_str());
        Serial.printf("Hub ID: %s\n", config.hub_id.c_str());
        configLoaded = true;
    }
    
    return loaded;
}

bool ConfigManager::loadFromSD() {
    if (!SD.exists(CONFIG_FILE)) {
        Serial.println("Config file not found on SD card");
        return false;
    }

    File configFile = SD.open(CONFIG_FILE, FILE_READ);
    if (!configFile) {
        Serial.println("Failed to open config file for reading from SD");
        return false;
    }

    return parseConfigFile(configFile);
}

bool ConfigManager::loadFromSPIFFS() {
    if (!SPIFFS.exists(CONFIG_FILE)) {
        Serial.println("Config file not found in SPIFFS");
        return false;
    }

    File configFile = SPIFFS.open(CONFIG_FILE, "r");
    if (!configFile) {
        Serial.println("Failed to open config file for reading from SPIFFS");
        return false;
    }

    bool result = parseConfigFile(configFile);
    if (result) {
        Serial.println("Configuration loaded from SPIFFS");
    }
    return result;
}

bool ConfigManager::parseConfigFile(File &configFile) {
    size_t size = configFile.size();
    std::unique_ptr<char[]> buf(new char[size]);
    configFile.readBytes(buf.get(), size);
    configFile.close();

    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, buf.get());
    
    if (error) {
        Serial.println("Failed to parse config file");
        return false;
    }

    config.mqtt_server = doc["mqtt_server"].as<String>();
    config.mqtt_port = doc["mqtt_port"];
    config.mqtt_username = doc["mqtt_username"].as<String>();
    config.mqtt_password = doc["mqtt_password"].as<String>();
    config.wifi_ssid = doc["wifi_ssid"].as<String>();
    config.wifi_password = doc["wifi_password"].as<String>();
    config.hub_id = doc["hub_id"].as<String>();
    
    return true;
}

bool ConfigManager::saveConfig() {
    bool savedToSD = false;
    bool savedToSPIFFS = false;
    
    // Try to save to SD if available
    if (sdAvailable) {
        savedToSD = saveToSD();
    }
    
    // Always try to save to SPIFFS as backup
    if (spiffsAvailable) {
        savedToSPIFFS = saveToSPIFFS();
    }
    
    // Success if saved to at least one storage
    return (savedToSD || savedToSPIFFS);
}

bool ConfigManager::saveToSD() {
    File configFile = SD.open(CONFIG_FILE, FILE_WRITE);
    if (!configFile) {
        Serial.println("Failed to open config file for writing to SD");
        return false;
    }

    if (serializeJsonToFile(configFile) == false) {
        return false;
    }

    Serial.println("Configuration saved successfully to SD card");
    return true;
}

bool ConfigManager::saveToSPIFFS() {
    File configFile = SPIFFS.open(CONFIG_FILE, "w");
    if (!configFile) {
        Serial.println("Failed to open config file for writing to SPIFFS");
        return false;
    }

    if (serializeJsonToFile(configFile) == false) {
        return false;
    }

    Serial.println("Configuration saved successfully to SPIFFS");
    return true;
}

bool ConfigManager::serializeJsonToFile(File &configFile) {
    JsonDocument doc;

    doc["mqtt_server"] = config.mqtt_server;
    doc["mqtt_port"] = config.mqtt_port;
    doc["mqtt_username"] = config.mqtt_username;
    doc["mqtt_password"] = config.mqtt_password;
    doc["wifi_ssid"] = config.wifi_ssid;
    doc["wifi_password"] = config.wifi_password;
    doc["hub_id"] = config.hub_id;

    if (serializeJson(doc, configFile) == 0) {
        Serial.println("Failed to write config to file");
        configFile.close();
        return false;
    }

    configFile.close();
    return true;
}

HubConfig* ConfigManager::getConfig() {
    return &config;
}

bool ConfigManager::isConfigLoaded() {
    return configLoaded;
}