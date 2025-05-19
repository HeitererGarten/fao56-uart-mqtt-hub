#include <Arduino.h>
#include <ArduinoJson.h>
#include "config.h"
#include "config_manager.h"
#include "rtc_manager.h"
#include "wifi_manager.h"
#include "mqtt_manager.h"
#include "portal_manager.h"
#include "serial_manager.h"
#include "oled_manager.h"

// Global instances
ConfigManager configManager;
RTCManager rtcManager;
WiFiManager wifiManager(configManager.getConfig());
MQTTManager mqttManager(configManager.getConfig());
PortalManager portalManager(configManager.getConfig(), &configManager);
OLEDManager oledManager;

// Serial handling
HardwareSerial interSerial(2);
SerialManager serialManager(&interSerial);

// Task management
TaskHandle_t serialTaskHandle = NULL;
TaskHandle_t mqttTaskHandle = NULL;
TaskHandle_t displayTaskHandle = NULL;
SemaphoreHandle_t binSem = NULL;

// Data instance for sensor data
dhtData dataInstance;
char jsonBuffer[MQTT_MAX_PACKET_SIZE];
JsonDocument doc;

// Structure for WiFi credentials to send to ESP-NOW hub
typedef struct wifiCredentials {
    char wifiSSID[32];
    char wifiPass[64];
} wifiCredentials;

// NTP Server setup 
const char* ntpServer = "pool.ntp.org";
// Timezone settings
const long gmtOffset_sec = 3600 * NTP_OFFSET;
const int daylightOffset_sec = 3600;

// Send WiFi credentials to ESP-NOW hub
bool sendWiFiCredentials() {
    wifiCredentials creds;
    HubConfig* config = configManager.getConfig();
    
    // Copy WiFi credentials from config
    strncpy(creds.wifiSSID, config->wifi_ssid.c_str(), sizeof(creds.wifiSSID) - 1);
    strncpy(creds.wifiPass, config->wifi_password.c_str(), sizeof(creds.wifiPass) - 1);
    
    // Ensure null termination
    creds.wifiSSID[sizeof(creds.wifiSSID) - 1] = '\0';
    creds.wifiPass[sizeof(creds.wifiPass) - 1] = '\0';
    
    Serial.println("Sending WiFi credentials to ESP-NOW hub");
    Serial.print("SSID: ");
    Serial.println(creds.wifiSSID);
    
    // Send start marker
    interSerial.write('`');
    
    // Send credentials struct
    size_t written = interSerial.write((uint8_t*)&creds, sizeof(creds));
    
    // Wait for confirmation (with timeout)
    unsigned long startTime = millis();
    while (millis() - startTime < 5000) { // 5 second timeout
        if (interSerial.available()) {
            byte confirmation = interSerial.read();
            if (confirmation == 'W') {
                Serial.println("WiFi credentials confirmed by ESP-NOW hub");
                return true;
            }
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    if (written == sizeof(creds)) {
        Serial.println("WiFi credentials sent but no confirmation received");
        return true;
    } else {
        Serial.println("Failed to send WiFi credentials");
        return false;
    }
}

// Task to receive sensor data via Serial
void serialTask(void *parameter) {
    while (true) {
        if (serialManager.readData(&dataInstance)) {
            xSemaphoreGive(binSem);
            Serial.println("Received data from ESP-NOW Hub");
            
            // Update display with new sensor data
            oledManager.showSensorData(dataInstance.nodeID, dataInstance.temp, 
                                    dataInstance.humidity, dataInstance.moisture);
        }
        
        // Check for serial commands
        if (Serial.available()) {
            String command = Serial.readStringUntil('\n');
            command.trim();
            
            if (command == "sendwifi") {
                sendWiFiCredentials();
            }
        }
        
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

// Task to process and send data via MQTT
void mqttTask(void *parameter) {
    while (true) {
        // Only send when MQTT is connected and data is available
        if (mqttManager.isConnected() && xSemaphoreTake(binSem, 0) == pdTRUE) {
            doc.clear();
            doc["sensor_id"] = dataInstance.nodeID;
            doc["hub_id"] = configManager.getConfig()->hub_id;
            doc["temp"] = dataInstance.temp;
            doc["humidity"] = dataInstance.humidity;
            doc["moisture"] = dataInstance.moisture;
            
            // Get time from RTC or NTP
            struct tm timeInfo;
            if (rtcManager.getCurrentTime(&timeInfo)) {
                JsonObject date = doc["date"].to<JsonObject>();
                date["year"] = timeInfo.tm_year + 1900;
                date["month"] = timeInfo.tm_mon + 1;
                date["day"] = timeInfo.tm_mday;
                date["hour"] = timeInfo.tm_hour;
                date["minute"] = timeInfo.tm_min;
                date["second"] = timeInfo.tm_sec;
            } else {
                Serial.println("Failed to obtain time");
                // Add timestamp using uptime instead
                doc["uptime_ms"] = millis();
            }
            
            serializeJson(doc, jsonBuffer);
            mqttManager.publish(TOPIC_SENSOR, jsonBuffer);
            Serial.println("Published to MQTT");
        }
        
        // Check if we need to update RTC from NTP
        rtcManager.checkUpdateInterval();
        
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
// New task to update the display
void displayTask(void *parameter) {
    struct tm timeinfo;
    static bool showingData = true;
    static unsigned long lastToggle = 0;
    
    while (true) {
        // Toggle between sensor data and time display every 5 seconds
        unsigned long now = millis();
        if (now - lastToggle > 5000) {
            showingData = !showingData;
            lastToggle = now;
            
            if (showingData) {
                // Only show data if we've received some
                if (dataInstance.nodeID[0] != '\0') {
                    oledManager.showSensorData(dataInstance.nodeID, dataInstance.temp, 
                                          dataInstance.humidity, dataInstance.moisture);
                }
            } else {
                // Show current time
                if (rtcManager.getCurrentTime(&timeinfo)) {
                    oledManager.showTime(&timeinfo);
                }
            }
        }
        
        // Update MQTT status indicator
        oledManager.showMQTTStatus(mqttManager.isConnected());
        
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}
bool testUartConnection() {
    Serial.println("Testing UART connection to ESP-NOW hub...");
    
    // Clear any data in the buffer
    while (interSerial.available()) {
        interSerial.read();
    }
    
    // Send test character
    interSerial.write('T');
    interSerial.flush();
    
    // Wait for response
    unsigned long startTime = millis();
    while (millis() - startTime < 1000) {
        if (interSerial.available()) {
            char c = interSerial.read();
            Serial.printf("Received: %c\n", c);
            return true;
        }
        delay(10);
    }
    
    Serial.println("No response from ESP-NOW hub");
    return false;
}

void setup() {
    
    // Initialize Serial
    Serial.begin(BAUD_RATE);
    Serial.setRxBufferSize(RX_BUFFER_SIZE);
    Serial.println("UART-MQTT Hub starting...");

    // Initialize OLED
    if (!oledManager.begin()) {
        Serial.println("Warning: OLED display initialization failed");
    }
    
    oledManager.showWelcomeScreen();

    // Initialize RTC
    rtcManager.begin();

    // Initialize SD card and load config
    if (!configManager.initStorage()) {
        Serial.println("WARNING: All storage systems failed. Using default configuration.");
        oledManager.showStatus("No config found");
        portalManager.begin();
        return;
    }

    if (!configManager.loadConfig()) {
        Serial.println("No configuration found or unable to load configuration!");
        oledManager.showStatus("Config required");
        portalManager.begin();
        return;
    }
    
    // Check if portal should be triggered
    portalManager.checkTrigger();

    // If portal is active, don't continue with normal setup
    if (portalManager.isActive()) {
        Serial.println("Configuration portal is active. Normal operation paused.");
        oledManager.showStatus("Config portal active");
        return;
    }
    
    // Initialize serial communication with ESP-NOW hub
    serialManager.begin(BAUD_RATE, RX_HUB, TX_HUB);
    
    // Test UART connection to ESP-NOW hub
    oledManager.showStatus("Testing UART...");
    if (testUartConnection()) {
        Serial.println("UART connection to ESP-NOW hub successful");
        oledManager.showStatus("UART connected");
        delay(1000);  // Show the message for a moment
    } else {
        Serial.println("UART connection to ESP-NOW hub failed");
        oledManager.showStatus("UART failed");
        delay(2000);  // Show the error message for a moment
    }
    
    // Create binary semaphore for task synchronization
    binSem = xSemaphoreCreateBinary();
    
    oledManager.showStatus("Connecting WiFi...");
    
    // Set up WiFi and MQTT
    if (wifiManager.connect()) {
        // Once WiFi is connected, send credentials to ESP-NOW hub
        // Add a small delay to ensure serial is ready
         oledManager.showWiFiStatus(true, configManager.getConfig()->wifi_ssid.c_str());
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        // Once WiFi is connected, send credentials to ESP-NOW hub
        oledManager.showStatus("Sending WiFi to ESP-NOW");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        if (!sendWiFiCredentials()) {
            Serial.println("Retrying WiFi credential send...");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            if (!sendWiFiCredentials()) {
                Serial.println("Second attempt to send WiFi credentials failed");
                oledManager.showStatus("WiFi send failed");
                vTaskDelay(2000 / portTICK_PERIOD_MS);
            }
        } else {
            Serial.println("WiFi credentials sent successfully");
            oledManager.showStatus("WiFi info sent");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
        }
        
        // Configure MQTT
        oledManager.showStatus("Connecting MQTT...");
        mqttManager.begin();
        
        // Configure NTP and update RTC
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
        Serial.println("NTP configured");
        
        // Set RTC from NTP if present
        if (rtcManager.isPresent()) {
            oledManager.showStatus("Updating time...");
            rtcManager.updateFromNTP();
            vTaskDelay(100 / portTICK_PERIOD_MS);
        }
        
        // Create the tasks for normal operation
        oledManager.showStatus("Creating tasks...");
        vTaskDelay(100 / portTICK_PERIOD_MS);
        
        // Create the tasks for normal operation
        xTaskCreatePinnedToCore(
            serialTask,
            "serialTask",
            2048,
            NULL,
            1,
            &serialTaskHandle,
            0
        );
        
        xTaskCreatePinnedToCore(
            mqttTask,
            "mqttTask",
            4096,
            NULL,
            1,
            &mqttTaskHandle,
            1
        );
        
        xTaskCreatePinnedToCore(
            displayTask,
            "displayTask",
            2048,
            NULL,
            1,
            &displayTaskHandle,
            0
        );
        
        oledManager.showStatus("System ready");
        delay(1000);
        Serial.println("Tasks created, system operational");
    } else {
        oledManager.showWiFiStatus(false);
    }
}

void loop() {
    // Always check for config button press first
    if (portalManager.checkTrigger() || portalManager.isActive()) {
        vTaskDelay(100 / portTICK_PERIOD_MS);
        return;
    }
    
    // Check WiFi status and reconnect if needed
    if (!wifiManager.isConnected()) {
        oledManager.showStatus("WiFi reconnecting...");
        Serial.println("WiFi connection lost, attempting to reconnect...");
        if (!wifiManager.connect()) {
            Serial.println("WiFi reconnection failed. Press config button to enter setup mode.");
            oledManager.showWiFiStatus(false);
            vTaskDelay(5000 / portTICK_PERIOD_MS);
            return;
        }
        // If reconnected, re-send WiFi credentials to ESP-NOW hub
        oledManager.showWiFiStatus(true, configManager.getConfig()->wifi_ssid.c_str());
        //sendWiFiCredentials(); No need to resend credentials
        Serial.println("WiFi reconnected");
        oledManager.showStatus("WiFi reconnected");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
    
    // Keep MQTT processing
    mqttManager.loop();
    // Add a small delay to prevent excessive CPU usage
    vTaskDelay(10 / portTICK_PERIOD_MS);
}