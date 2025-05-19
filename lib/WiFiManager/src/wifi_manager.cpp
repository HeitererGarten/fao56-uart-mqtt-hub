#include "wifi_manager.h"
int connectionFailures = 0;
const int MAX_FAILURES_BEFORE_CONFIG = 5;
WiFiManager::WiFiManager(HubConfig* config) {
    this->config = config;
}

bool WiFiManager::connect() {
    // Connect to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(config->wifi_ssid);
  
    WiFi.begin(config->wifi_ssid.c_str(), config->wifi_password.c_str());
    
    // Add a timeout (30 seconds)
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(1000);
        Serial.print(".");
        attempts++;
    }
  
    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        return true;  // Return success
    } else {
        Serial.println("");
        Serial.println("WiFi connection failed!");
        return false;  // Return failure
    }
    if (WiFi.status() == WL_CONNECTED) {
        connectionFailures = 0;
        return true;
    } else {
        connectionFailures++;
        if (connectionFailures >= MAX_FAILURES_BEFORE_CONFIG) {
            Serial.println("Too many connection failures, entering config mode");
            // Call portal manager's begin method here or set a flag
            return false;
        }
        return false;
    }
}

bool WiFiManager::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::setupAP(const char* ssid, const char* password) {
    WiFi.softAP(ssid, password);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);
}