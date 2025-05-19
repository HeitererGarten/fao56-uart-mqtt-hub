#include "mqtt_manager.h"

MQTTManager::MQTTManager(HubConfig* config) {
    this->config = config;
}

bool MQTTManager::begin() {
    client.setClient(espClient);
    client.setServer(config->mqtt_server.c_str(), config->mqtt_port);
    return connect();
}

bool MQTTManager::connect() {
    // Loop until MQTT is established
    int attempts = 0;
    const int MAX_ATTEMPTS = 2;
    
    while (!client.connected() && attempts < MAX_ATTEMPTS) {
        Serial.println("\nAttempting MQTT connection...");
        // Attempt to connect using config values
        if (client.connect(config->hub_id.c_str(), config->mqtt_username.c_str(), config->mqtt_password.c_str())) {
            Serial.println("MQTT connection established.");
            // Subscribe to your topics
            client.subscribe("sensor/data");  // Replace with your actual topic
            return true;
        } else {
            Serial.print("\nFailed, rc=");
            Serial.print(client.state());
            Serial.println(". Trying again...");
            // Wait before retrying
            vTaskDelay(2000 / portTICK_PERIOD_MS);
            attempts++;
        }
    }
    
    if (attempts >= MAX_ATTEMPTS) {
        Serial.println("MQTT connection failed repeatedly. Consider reconfiguring.");
        // Indicate connection issue with LED
        digitalWrite(LED_BUILTIN, HIGH);  // Turn on built-in LED to indicate MQTT connection failure
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        digitalWrite(LED_BUILTIN, LOW);   // Turn off LED after delay
        return false;
    }
    
    return false;
}

bool MQTTManager::publish(const char* topic, const char* payload) {
    return client.publish(topic, payload);
}

bool MQTTManager::isConnected() {
    return client.connected();
}

void MQTTManager::loop() {
    if (!client.connected()) {
        connect();
    }
    client.loop();
}