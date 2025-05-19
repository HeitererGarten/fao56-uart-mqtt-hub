#pragma once

#include <Arduino.h>

// General settings
#define BAUD_RATE 115200
#define RX_BUFFER_SIZE 2048
#define MQTT_MAX_PACKET_SIZE 256

// UART pins for ESP-NOW hub communication
#define RX_HUB 19  // Define your actual RX pin here
#define TX_HUB 20  // Define your actual TX pin here

// MQTT topics
#define TOPIC_SENSOR "topic/sensor"  // Define your actual topic here
#define NTP_OFFSET 6  // Define your actual time zone offset here

// RTC settings
#define I2C_SDA 8  // Default I2C SDA pin on ESP32-S3
#define I2C_SCL 9  // Default I2C SCL pin on ESP32-S3
#define RTC_UPDATE_INTERVAL 86400000  // Update RTC from NTP once a day (in ms)

// SD Card settings
#define SD_CS 10  // SD card chip select pin
#define CONFIG_FILE "/mqtt_config.json"
#define DEFAULT_AP_SSID "MQTT-Hub-Config"
#define DEFAULT_AP_PASSWORD "admin@123"

// Configuration structure
struct HubConfig {
    String mqtt_server = "";
    int mqtt_port = 1883;
    String mqtt_username = "";
    String mqtt_password = "";
    String wifi_ssid = "";
    String wifi_password = "";
    String hub_id = "H-0";
};

// Sensor data structure
typedef struct dhtData {
    char nodeID[8];
    float temp;
    float humidity;
    long moisture;
} dhtData;