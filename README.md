# FAO56 UART-MQTT Hub

An ESP32-S3 based bridge that receives sensor data via UART from ESP-NOW hubs and publishes it to MQTT brokers for cloud integration. This is part of the FAO56 IoT system for agricultural monitoring and irrigation management.

## Overview

The UART-MQTT Hub serves as the final bridge between the ESP-NOW wireless sensor network and cloud services. It receives processed sensor data from ESP-NOW UART Hubs via serial communication, adds timestamps using an RTC module, stores data locally on SD card, and publishes to configurable MQTT topics for remote monitoring and analysis.

## Architecture

```
[ESP-NOW UART Hub] ---> [UART-MQTT Hub] ---> [MQTT Broker] ---> [Cloud Services]
    (Serial)             (This Device)        (Internet)       (Dashboard/Analytics)
```

## Features

- **UART Communication**: Receives sensor data via serial from ESP-NOW hubs
- **MQTT Publishing**: Publishes data to cloud platforms or local brokers
- **Real-Time Clock**: Accurate timestamping with DS3231 RTC module
- **SD Card Storage**: Local data backup and configuration storage
- **Web Configuration**: Easy setup via captive portal interface
- **NTP Synchronization**: Automatic time synchronization
- **JSON Data Format**: Structured data for easy integration
- **WiFi Management**: Auto-reconnection and credential management
- **Flexible Storage**: Automatic fallback from SD card to SPIFFS
- **Adaptive Timekeeping**: Falls back to NTP when RTC unavailable
- **Manual Configuration**: GPIO button for portal access

## Hardware Requirements

- ESP32-S3 development board
- UART connection to ESP-NOW hub
- Power supply (5V recommended)
- Optional: DS3231 RTC module with battery backup
- Optional: MicroSD card module and card
- Optional: Status LEDs, external antenna

## Pin Configuration

```cpp
// UART Communication
#define RX_HUB 19     // UART RX pin for ESP-NOW hub
#define TX_HUB 20     // UART TX pin for ESP-NOW hub
#define BAUD_RATE 115200

// I2C for RTC (Optional)
#define I2C_SDA 8     // I2C data pin
#define I2C_SCL 9     // I2C clock pin

// SD Card (Optional)
#define SD_CS 10      // SD card chip select pin

// Configuration Button
#define CONFIG_BUTTON_PIN 0  // GPIO button for configuration portal
```

## Storage and Timing Flexibility

### Storage System Hierarchy
The hub automatically adapts to available storage:

1. **Primary: SD Card Storage**
   - Configuration: `/mqtt_config.json`
   - Data logs: `/data/YYYY-MM-DD.log`
   - Large capacity for extended logging

2. **Fallback: SPIFFS Storage**
   - Configuration: `/config.json`
   - Limited capacity but reliable
   - Automatic detection when SD unavailable

### Timing System Hierarchy
The hub provides accurate timestamps through multiple methods:

1. **Primary: RTC Module (DS3231)**
   - Battery-backed accurate timekeeping
   - Maintains time during power cycles
   - Independent of network connectivity

2. **Secondary: NTP Synchronization**
   - Automatic fallback when RTC unavailable
   - Periodic synchronization with internet time servers
   - Requires WiFi connectivity for updates

3. **Tertiary: System Uptime**
   - Basic timestamp based on boot time
   - Used when both RTC and NTP unavailable

## Configuration Portal Access

### Automatic Portal Activation
- Activates automatically when no valid configuration exists
- Creates AP: `MQTT-Hub-Config` with password: `admin@123`

### Manual Portal Activation
**GPIO Button Method:**
1. Hold down the configuration button (GPIO 0)
2. Watch for LED indicator on built-in LED
3. Continue holding until **3 consecutive LED blinks** occur
4. Release button - configuration portal will start
5. Connect to `MQTT-Hub-Config` WiFi network
6. Navigate to captive portal for configuration

This allows reconfiguration even when the device has valid settings.

## Data Structures

### Sensor Data Structure
```cpp
typedef struct dhtData {
    char nodeID[8];     // Sensor node identifier
    float temp;         // Temperature in °C
    float humidity;     // Humidity in %
    long moisture;      // Soil moisture in %
} dhtData;
```

### Configuration Structure
```cpp
struct HubConfig {
    String mqtt_server;     // MQTT broker address
    int mqtt_port;          // MQTT broker port (default: 1883)
    String mqtt_username;   // MQTT authentication username
    String mqtt_password;   // MQTT authentication password
    String wifi_ssid;       // WiFi network name
    String wifi_password;   // WiFi network password
    String hub_id;          // Hub identifier (default: "H-0")
};
```

## MQTT Topics

Data is published to structured topics:
Data is published as complete JSON objects to a single topic:

```
topic/sensor - Complete sensor data with metadata
```

### JSON Payload Structure
```json
{
	"sensor_id": "NODE01",
	"hub_id": "H-0",
	"temp": 25.4,
	"humidity": 60.8,
	"moisture": 45,
	"date": {
		"year": 2024,
		"month": 3,
		"day": 15,
		"hour": 14,
		"minute": 30,
		"second": 45
	}
}
```

### Fallback Timing
When RTC/NTP unavailable, uptime is used:
```json
{
	"sensor_id": "NODE01",
	"hub_id": "H-0",
	"temp": 25.4,
	"humidity": 60.8,
	"moisture": 45,
	"uptime_ms": 123456789
}
```

## Operation Flow

1. **Initialization**: Boot and detect available hardware (RTC, SD card)
2. **Storage Setup**: Initialize SD card or fallback to SPIFFS
3. **Timing Setup**: Initialize RTC or setup NTP synchronization
4. **Configuration Check**: Load config or start AP mode for setup
5. **WiFi Connection**: Connect to configured network
6. **MQTT Setup**: Establish connection to MQTT broker
7. **Time Sync**: Synchronize time source (RTC priority, NTP fallback)
8. **Data Processing**: 
   - Receive UART data from ESP-NOW hub
   - Add accurate timestamps from available time source
   - Store locally on available storage
   - Publish to MQTT topics
9. **Monitoring**: Continuous operation with periodic status updates

## Configuration

### Web-Based Setup
1. **Automatic**: If no configuration exists, device creates WiFi AP
2. **Manual**: Hold GPIO button until 3 LED blinks, then release
3. Connect to WiFi AP: `MQTT-Hub-Config` (password: `admin@123`)
4. Navigate to captive portal for configuration
5. Enter WiFi and MQTT broker details
6. Configuration saved to available storage (SD card preferred, SPIFFS fallback)

### Configuration File Locations
- **SD Card**: `/mqtt_config.json`
- **SPIFFS**: `/config.json`

### Configuration File Format
```json
{
    "mqtt_server": "your-broker.com",
    "mqtt_port": 1883,
    "mqtt_username": "username",
    "mqtt_password": "password",
    "wifi_ssid": "YourWiFi",
    "wifi_password": "YourPassword",
    "hub_id": "H-0"
}
```

### Default Settings
```cpp
#define TOPIC_SENSOR "topic/sensor"
#define NTP_OFFSET 6  // Timezone offset in hours
#define DEFAULT_AP_SSID "MQTT-Hub-Config"
#define DEFAULT_AP_PASSWORD "admin@123"
#define CONFIG_BUTTON_PIN 0  // Configuration button GPIO
```

## Building and Deployment

### Prerequisites
- PlatformIO IDE or Arduino IDE with ESP32-S3 support
- Required libraries:
  - PubSubClient (MQTT)
  - ArduinoJson (JSON handling)
  - RTClib (RTC support - optional)
  - SD (SD card support - optional)
  - SPIFFS (Built-in ESP32 storage)
  - WiFi (ESP32 WiFi)

### Build Configuration
Project uses PlatformIO with configuration in platformio.ini:
- Target: ESP32-S3
- Framework: Arduino
- Monitor speed: 115200 baud

### Upload Process
1. Connect ESP32-S3 to computer via USB
2. Select correct COM port and board
3. Build and upload firmware
4. **Optional**: Insert formatted SD card
5. **Optional**: Connect RTC module and set initial time
6. Connect to ESP-NOW UART Hub via UART

### Hardware Setup Options

#### Minimal Setup (No External Components)
- Uses SPIFFS for configuration storage
- Uses NTP for time synchronization
- Requires WiFi connectivity for time updates

#### Full Setup (All Components)
1. **RTC Module**: Connect DS3231 to I2C pins (SDA=8, SCL=9)
2. **SD Card**: Connect SD module to SPI with CS=10
3. **UART**: Connect to ESP-NOW hub (RX=19, TX=20)
4. **Button**: Connect momentary button to GPIO 0 and GND
5. **Power**: Ensure stable 5V supply for reliable operation

## Data Storage

### Storage Priority System
1. **SD Card** (if available):
   - Configuration file: `/mqtt_config.json`
   - Data logs: `/data/YYYY-MM-DD.log`
   - Backup files for offline periods
   - Automatic file rotation

2. **SPIFFS** (fallback):
   - Configuration file: `/config.json`
   - Limited logging capacity
   - More reliable than SD for configuration

### Cloud Storage (MQTT)
- Real-time data publishing
- Structured JSON payloads
- Persistent connections with auto-reconnect
- QoS level 1 for reliable delivery

## Debugging

Serial output provides detailed information:
- Hardware detection status (RTC, SD card)
- Storage system selection
- Time source configuration
- Configuration loading status
- WiFi connection attempts
- MQTT broker connections
- Data reception and processing
- Button press detection

Monitor at 115200 baud for complete debug information.

## Integration

This UART-MQTT Hub integrates with:
- **ESP-NOW UART Hubs**: Receives consolidated sensor data
- **MQTT Brokers**: AWS IoT, Google Cloud IoT, local Mosquitto
- **Cloud Platforms**: ThingSpeak, Blynk, custom dashboards
- **FAO56 System**: Provides data for irrigation calculations

## Troubleshooting

### Common Issues
1. **Configuration not loading**: Check storage system (SD/SPIFFS) and file format
2. **Time synchronization issues**: Verify RTC connections or WiFi for NTP
3. **MQTT connection failures**: Verify broker credentials and network
4. **Button not working**: Check GPIO 0 connection and pull-up resistor
5. **Storage full**: Monitor SPIFFS usage or SD card space

### Status Indicators
- Serial output shows detailed operational status
- LED blinks indicate button press detection
- MQTT status messages indicate system health
- Storage logs provide historical troubleshooting data

### Recovery Procedures
1. **Factory Reset**: Hold button for 3 blinks to access configuration portal
2. **Storage Issues**: System automatically falls back to SPIFFS
3. **Time Issues**: System automatically falls back to NTP
4. **Network Issues**: Device attempts auto-reconnection
5. **Complete Reset**: Erase SPIFFS and remove SD card for fresh start

### Hardware Compatibility
- **Minimum**: ESP32-S3 only (uses SPIFFS + NTP)
- **Standard**: ESP32-S3 + SD card (enhanced storage)
- **Full**: ESP32-S3 + SD card + RTC (maximum reliability)

## Performance Specifications

- **Data Throughput**: Up to 100 sensor readings per minute
- **Storage Capacity**: 
  - SPIFFS: ~1.5MB (thousands of readings)
  - SD Card: Limited by card size (32GB+ recommended)
- **Network Latency**: <500ms typical MQTT publish time
- **Power Consumption**: ~200mA average operation
- **Operating Range**: -10°C to +60°C ambient temperature
- **Time Accuracy**: 
  - RTC: ±2ppm (very accurate)
  - NTP: ±100ms (network dependent)

## License

Part of the FAO56 IoT agricultural monitoring system.