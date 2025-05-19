#include "oled_manager.h"

OLEDManager::OLEDManager() : display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET) {
    // Constructor
}

bool OLEDManager::begin() {
    // Initialize the OLED display
    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        Serial.println(F("SSD1306 allocation failed"));
        return false;
    }
    
    initialized = true;
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    showWelcomeScreen();
    
    return true;
}

void OLEDManager::showWelcomeScreen() {
    if (!initialized) return;
    
    display.clearDisplay();
    
    // Display title
    display.setTextSize(2);
    drawCenteredText("FAO56", 10);
    
    // Display subtitle
    display.setTextSize(1);
    drawCenteredText("UART-MQTT Hub", 30);
    drawCenteredText("Starting...", 45);
    
    display.display();
    delay(2000);
}

void OLEDManager::showSensorData(const char* nodeID, float temp, float humidity, long moisture) {
    if (!initialized) return;
    
    // Store the data
    strncpy(lastNodeID, nodeID, sizeof(lastNodeID) - 1);
    lastNodeID[sizeof(lastNodeID) - 1] = '\0';
    lastTemp = temp;
    lastHumidity = humidity;
    lastMoisture = moisture;
    hasData = true;
    
    // Display the data immediately
    display.clearDisplay();
    display.setTextSize(1);
    
    // Header with node info
    String header = "Node: ";
    header += nodeID;
    drawCenteredText(header, 0);
    
    display.drawLine(0, 10, SCREEN_WIDTH, 10, SSD1306_WHITE);
    
    // Show sensor values
    display.setCursor(5, 15);
    display.print("Temp: ");
    display.print(temp);
    display.println(" C");
    
    display.setCursor(5, 25);
    display.print("Humidity: ");
    display.print(humidity);
    display.println(" %");
    
    display.setCursor(5, 35);
    display.print("Moisture: ");
    display.print(moisture);
    display.println(" %");
    
    display.display();
}

void OLEDManager::showTime(struct tm *timeinfo) {
    if (!initialized) return;
    
    display.clearDisplay();
    
    // Show date at the top
    char dateStr[20];
    snprintf(dateStr, sizeof(dateStr), "%04d-%02d-%02d", 
             timeinfo->tm_year + 1900,
             timeinfo->tm_mon + 1,
             timeinfo->tm_mday);
    
    display.setTextSize(1);
    drawCenteredText(dateStr, 5);
    
    // Show time in larger font
    char timeStr[20];
    snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", 
             timeinfo->tm_hour,
             timeinfo->tm_min,
             timeinfo->tm_sec);
    
    display.setTextSize(2);
    drawCenteredText(timeStr, 25);
    
    display.display();
}

void OLEDManager::showStatus(const char* status) {
    if (!initialized) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    drawCenteredText("Status", 10);
    drawCenteredText(status, 30);
    display.display();
}

void OLEDManager::showWiFiStatus(bool connected, const char* ssid) {
    if (!initialized) return;
    
    display.clearDisplay();
    display.setTextSize(1);
    drawCenteredText("WiFi Status", 5);
    
    if (connected && ssid) {
        drawCenteredText("Connected to:", 20);
        drawCenteredText(ssid, 35);
    } else {
        drawCenteredText("Disconnected", 25);
    }
    
    display.display();
}

void OLEDManager::showMQTTStatus(bool connected) {
    if (!initialized) return;
    
    // Save current content to draw at the bottom
    display.fillRect(0, 50, SCREEN_WIDTH, 10, SSD1306_BLACK);
    
    display.setTextSize(1);  // Ensure text is small
    display.setCursor(5, 52);  // Moved cursor up from 55 to 52
    display.print("MQTT: ");
    display.print(connected ? "Connected" : "Disconnected");
    
    display.display();
}

void OLEDManager::clear() {
    if (!initialized) return;
    
    display.clearDisplay();
    display.display();
}

void OLEDManager::update() {
    if (!initialized) return;
    
    // Toggle between showing data and time every 5 seconds
    unsigned long now = millis();
    if (now - lastToggle > 5000) {
        showingData = !showingData;
        lastToggle = now;
    }
}

void OLEDManager::drawCenteredText(const String &text, int16_t y) {
    int16_t x1, y1;
    uint16_t w, h;
    
    display.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
    display.setCursor((SCREEN_WIDTH - w) / 2, y);
    display.print(text);
}