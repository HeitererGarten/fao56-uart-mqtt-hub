#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "config.h"
#include <time.h>

// Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET    -1  // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C // I2C address for most common OLED displays

class OLEDManager {
public:
    OLEDManager();
    bool begin();
    void showWelcomeScreen();
    void showSensorData(const char* nodeID, float temp, float humidity, long moisture);
    void showTime(struct tm *timeinfo);
    void showStatus(const char* status);
    void showWiFiStatus(bool connected, const char* ssid = nullptr);
    void showMQTTStatus(bool connected);
    void clear();
    void update(); // Call this periodically to refresh dynamic content
    
private:
    Adafruit_SSD1306 display;
    bool initialized = false;
    unsigned long lastUpdate = 0;
    
    // Display data
    char lastNodeID[8] = "";
    float lastTemp = 0;
    float lastHumidity = 0;
    long lastMoisture = 0;
    bool hasData = false;
    bool showingData = true; // Toggle between data and time
    
    // Time and update vars
    unsigned long lastToggle = 0;
    
    void drawCenteredText(const String &text, int16_t y);
};