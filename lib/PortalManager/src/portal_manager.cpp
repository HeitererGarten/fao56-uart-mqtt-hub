#include "portal_manager.h"

PortalManager::PortalManager(HubConfig* config, ConfigManager* configManager) : server(80) {
    this->config = config;
    this->configManager = configManager;
    portalActive = false;
}

void PortalManager::begin() {
    // Setup the AP for the configuration portal
    WiFi.softAP(DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
    IPAddress IP = WiFi.softAPIP();
    Serial.print("Config Portal IP address: ");
    Serial.println(IP);
    portalActive = true;
    
    // Set up SPIFFS to serve the web interface files
    if(!SPIFFS.begin(true)){
        Serial.println("An Error has occurred while mounting SPIFFS");
        return;
    }
    
    setupRoutes();
    
    server.begin();
    
    Serial.println("Configuration portal started");
    Serial.printf("Connect to WiFi network '%s' with password '%s'\n", DEFAULT_AP_SSID, DEFAULT_AP_PASSWORD);
    Serial.printf("Then navigate to http://%s in your browser\n", IP.toString().c_str());
    // Flash the built-in LED 3 times to indicate portal activation
    pinMode(LED_BUILTIN, OUTPUT);
    for (int i = 0; i < 3; i++) {
        digitalWrite(LED_BUILTIN, HIGH);
        vTaskDelay(200 / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);
        vTaskDelay(200 / portTICK_PERIOD_MS);
    }
}

void PortalManager::setupRoutes() {
    // Setup the web server routes
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/index.html", "text/html");
    });
    
    server.on("/style.css", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/style.css", "text/css");
    });
    
    server.on("/script.js", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(SPIFFS, "/script.js", "application/javascript");
    });
    
    server.on("/config", HTTP_GET, [this](AsyncWebServerRequest *request){
        JsonDocument doc;
        doc["mqtt_server"] = config->mqtt_server;
        doc["mqtt_port"] = config->mqtt_port;
        doc["mqtt_username"] = config->mqtt_username;
        doc["mqtt_password"] = config->mqtt_password;
        doc["wifi_ssid"] = config->wifi_ssid;
        doc["wifi_password"] = "";  // Don't send the password
        doc["hub_id"] = config->hub_id;
        
        String response;
        serializeJson(doc, response);
        request->send(200, "application/json", response);
    });
    
    server.on("/save", HTTP_POST, [](AsyncWebServerRequest *request){
        // We'll handle the form in onBody
    }, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total){
        // Handle form data
        String dataStr = String((char*)data);
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, dataStr);
        
        if (error) {
            request->send(400, "text/plain", "Invalid JSON");
            return;
        }
        
        config->mqtt_server = doc["mqtt_server"].as<String>();
        config->mqtt_port = doc["mqtt_port"];
        config->mqtt_username = doc["mqtt_username"].as<String>();
        config->mqtt_password = doc["mqtt_password"].as<String>();
        config->wifi_ssid = doc["wifi_ssid"].as<String>();
        
        // Only update password if provided (not empty)
        if (doc["wifi_password"].as<String>() != "") {
            config->wifi_password = doc["wifi_password"].as<String>();
        }
        
        config->hub_id = doc["hub_id"].as<String>();
        
        if (configManager->saveConfig()) {
            request->send(200, "text/plain", "Configuration saved. The system will restart.");
            // Schedule a restart
            delay(1000);
            ESP.restart();
        } else {
            request->send(500, "text/plain", "Failed to save configuration");
        }
    });
    
    server.on("/restart", HTTP_POST, [](AsyncWebServerRequest *request){
        request->send(200, "text/plain", "Restarting...");
        delay(1000);
        ESP.restart();
    });
}

bool PortalManager::checkTrigger() {
    static unsigned long lastButtonPress = 0;
    const unsigned long debounceTime = 100;
    
    #define CONFIG_PIN 0 // GPIO0 is often the BOOT button on ESP32 boards
    
    pinMode(CONFIG_PIN, INPUT_PULLUP);
    
    // Check with debounce protection
    if (digitalRead(CONFIG_PIN) == LOW && (millis() - lastButtonPress > debounceTime)) {
        lastButtonPress = millis();
        Serial.println("Config button pressed, entering configuration mode");
        begin();
        return true;
    }
    return false;
}
