// VSCode Intellisense may erroneously underline certain macros. Ignore it
#include <Arduino.h>
#include <ArduinoJson.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "time.h"

#define BAUD_RATE 115200
#define RX_BUFFER_SIZE 2048
#define MQTT_MAX_PACKET_SIZE 256

// Struct to hold data from ESP_Serial_Hub
typedef struct dhtData {
    char nodeID[8];
    float temp;
    float humidity;
    long moisture;
} dhtData;

// Hold data to be sent to ESP_Serial_Hub 
typedef struct wifiData {
    // SSID max len is 63
    char wifiSSID[32];
    char wifiPass[64];
} wifiData;

// WiFi credentials to send to the other part of the Hub 
wifiData wifiProfile = {WIFI_SSID, WIFI_PASSWORD};

// Instance to hold data from ESP_Serial_Hub
dhtData dataInstance;
// String to be sent 
char dhtString[MQTT_MAX_PACKET_SIZE];

// Init the only JsonDocument to be used. Init here prevents mem leak
JsonDocument doc;

byte testByte;
byte readByte;
byte marker = 'W';

// Handles
static TaskHandle_t mqttHandle = NULL;
static TaskHandle_t serialHandle = NULL;
static TaskHandle_t wifiHandle = NULL;
static SemaphoreHandle_t binSem = NULL;
static SemaphoreHandle_t wifiBinSem = NULL;

// Number of bytes Read from Serial each time, must match amount sent
static uint16_t bytesRead = 0;

// NTP Server setup 
const char* ntpServer = "pool.ntp.org";
// Set time zone
const long gmtOffset_sec = (3600*NTP_OFFSET);
// Account for daylight saving
const int daylightOffset_sec = 3600;

// Init WiFi and MQTT 
WiFiClient espClient;
PubSubClient client(espClient);

// Create a dedicated Serial for comms between hub sections 
HardwareSerial interSerial(2);

void recvSerial(void *parameter) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    while (true) {
        if (interSerial.available()) {
            testByte = interSerial.peek();
            Serial.println(testByte);
            if (testByte == marker) {
                readByte = interSerial.read();
                xSemaphoreGiveFromISR(wifiBinSem, &xHigherPriorityTaskWoken);
            }
            else {
                // TODO: Find that one library to handle sending structs properly through Serial
                // This code reinterprets the struct ptr as ptr to a binary blob
                bytesRead = interSerial.readBytes(reinterpret_cast<char*>(&dataInstance), sizeof(dhtData));
                // Signal that data is ready to be taken
                xSemaphoreGive(binSem);
            }
        }
        vTaskDelay(5 / portTICK_PERIOD_MS);
    }
}

void sendMQTT(void *paramater) {
    while(true) {
        // Only send when MQTT is connected and data is available
        if (client.connected() && xSemaphoreTake(binSem, 0) == pdTRUE) {
            doc.clear();
            doc["sensor_id"] = dataInstance.nodeID;
            doc["hub_id"] = HUB_ID;
            doc["temp"] = dataInstance.temp;
            doc["humidity"] = dataInstance.humidity;
            doc["moisture"] = dataInstance.moisture;
            struct tm timeInfo;
            if (getLocalTime(&timeInfo)) {
                JsonObject date = doc["date"].to<JsonObject>();
                date["year"] = timeInfo.tm_year + 1900;
                date["month"] = timeInfo.tm_mon + 1;
                date["day"] = timeInfo.tm_mday;
                date["hour"] = timeInfo.tm_hour;
                date["minute"] = timeInfo.tm_min;
                date["second"] = timeInfo.tm_sec;
            } else {
                Serial.println("Failed to obtain time");
            }
            serializeJson(doc, dhtString);
            client.publish(TOPIC_SENSOR, dhtString);
        }
        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}   

// Transmit wifi credentials to ESP_Serial_Hub so that it can pair new sensor nodes 
void sendWifi(void *parameter) {
    while (true) {
        if (interSerial.availableForWrite()) {
            interSerial.write('`');
            interSerial.write((uint8_t*)&wifiProfile, sizeof(wifiProfile));
            Serial.print('\n');
        }
        if (xSemaphoreTake(wifiBinSem, 0) == pdTRUE) {
            vTaskSuspend(NULL);
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

void setupWifi() {
    // Connect to a WiFi network
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WIFI_SSID);
  
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  
    while (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(250 / portTICK_PERIOD_MS);
      Serial.print(".");
    }
  
    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
}

void reconnect() {
    // Loop until MQTT is established
    while (!client.connected()) {
        Serial.println("\nAttempting MQTT connection...");
        // Attempt to connect
        if (client.connect(HUB_ID, MQTT_USERNAME, MQTT_PASSWORD)) {
            Serial.println("MQTT connection established.");
            // Subscribe
            client.subscribe(TOPIC_SENSOR);
        } else {
            Serial.print("\nFailed, rc=");
            Serial.print(client.state());
            Serial.println(". Trying again...");
            // Wait before retrying
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
    }
}

void setup() {
    // In case data from Nodes scaled up. Also acts as buffer queue
    Serial.setRxBufferSize(RX_BUFFER_SIZE);
    // Connect to ESPNow-Serial Hub
    Serial.begin(BAUD_RATE);
    interSerial.begin(BAUD_RATE, SERIAL_8N1, RX_HUB, TX_HUB);

    // Create semaphores
    binSem = xSemaphoreCreateBinary();
    wifiBinSem = xSemaphoreCreateBinary();

    setupWifi();
    client.setServer(MQTT_SERVER, MQTT_PORT);

    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);

    xTaskCreatePinnedToCore (
        sendWifi,
        "sendWifi",
        1024,
        NULL,
        1,
        &wifiHandle,
        0
    );

    xTaskCreatePinnedToCore (
        recvSerial,
        "recvSerial",
        2048,
        NULL,
        1,
        &serialHandle,
        0
    );

    xTaskCreatePinnedToCore (
        sendMQTT,
        "sendMQTT",
        2048,
        NULL,
        1,
        &mqttHandle,
        1
    );
}

void loop() {
    // Ensure MQTT connection is established
    if (!client.connected()) {
        reconnect();
    }
    client.loop();
}
