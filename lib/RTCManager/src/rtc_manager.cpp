#include "rtc_manager.h"

RTCManager::RTCManager() {
    rtcPresent = false;
    lastRtcUpdate = 0;
}

bool RTCManager::begin() {
    Serial.println("Initializing RTC module...");
    // Configure I2C pins
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.printf("Using I2C pins: SDA=%d, SCL=%d\n", I2C_SDA, I2C_SCL);
    
    // Try to initialize RTC
    if (rtc.begin()) {
        Serial.println("✓ RTC found and initialized!");
        rtcPresent = true;
        
        // Check if RTC lost power and warn
        if (rtc.lostPower()) {
            Serial.println("! RTC lost power, will set time from NTP when connected");
        } else {
            // Print current RTC time
            DateTime now = rtc.now();
            Serial.printf("Current RTC time: %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
        }
        
        // Display temperature from RTC's internal sensor
        Serial.printf("RTC temperature: %.2f°C\n", rtc.getTemperature());
    } else {
        Serial.println("✗ Couldn't find RTC, will use NTP time only");
        rtcPresent = false;
    }
    
    return rtcPresent;
}

bool RTCManager::updateFromNTP() {
    if (!rtcPresent) {
        Serial.println("RTC not present, skipping update");
        return false;
    }
    
    Serial.println("Updating RTC from NTP...");
    
    struct tm timeInfo;
    if (getLocalTime(&timeInfo)) {
        Serial.printf("NTP time obtained: %04d-%02d-%02d %02d:%02d:%02d\n",
            timeInfo.tm_year + 1900, timeInfo.tm_mon + 1, timeInfo.tm_mday,
            timeInfo.tm_hour, timeInfo.tm_min, timeInfo.tm_sec);
            
        // Get RTC time before update for comparison
        DateTime beforeUpdate = rtc.now();
        
        // Update RTC with NTP time
        rtc.adjust(DateTime(
            timeInfo.tm_year + 1900, 
            timeInfo.tm_mon + 1, 
            timeInfo.tm_mday,
            timeInfo.tm_hour,
            timeInfo.tm_min,
            timeInfo.tm_sec
        ));
        
        // Get updated RTC time
        DateTime afterUpdate = rtc.now();
        
        // Calculate time difference
        int64_t timeDiff = afterUpdate.unixtime() - beforeUpdate.unixtime();
        
        Serial.printf("RTC updated! Before: %04d-%02d-%02d %02d:%02d:%02d\n",
            beforeUpdate.year(), beforeUpdate.month(), beforeUpdate.day(),
            beforeUpdate.hour(), beforeUpdate.minute(), beforeUpdate.second());
            
        Serial.printf("RTC after:  %04d-%02d-%02d %02d:%02d:%02d\n",
            afterUpdate.year(), afterUpdate.month(), afterUpdate.day(),
            afterUpdate.hour(), afterUpdate.minute(), afterUpdate.second());
            
        Serial.printf("Time drift was %lld seconds\n", timeDiff);
        
        lastRtcUpdate = millis();
        Serial.printf("Next RTC update scheduled in %0.2f hours\n", RTC_UPDATE_INTERVAL / 3600000.0);
        return true;
    } else {
        Serial.println("✗ Failed to obtain time from NTP for RTC update");
        return false;
    }
}

bool RTCManager::getCurrentTime(struct tm *timeInfo) {
    if (rtcPresent) {
        // Get time from RTC
        DateTime now = rtc.now();
        timeInfo->tm_year = now.year() - 1900;
        timeInfo->tm_mon = now.month() - 1;
        timeInfo->tm_mday = now.day();
        timeInfo->tm_hour = now.hour();
        timeInfo->tm_min = now.minute();
        timeInfo->tm_sec = now.second();
        
        static unsigned long lastTimeDebug = 0;
        // Only print time every 30 seconds to avoid flooding serial
        if (millis() - lastTimeDebug > 30000) {
            Serial.printf("[RTC Time] %04d-%02d-%02d %02d:%02d:%02d\n",
                now.year(), now.month(), now.day(),
                now.hour(), now.minute(), now.second());
            lastTimeDebug = millis();
        }
        return true;
    } else {
        // Try to get time from NTP (system time)
        bool success = getLocalTime(timeInfo);
        if (success) {
            static unsigned long lastTimeDebug = 0;
            // Only print time every 30 seconds
            if (millis() - lastTimeDebug > 30000) {
                Serial.printf("[NTP Time] %04d-%02d-%02d %02d:%02d:%02d\n",
                    timeInfo->tm_year + 1900, timeInfo->tm_mon + 1, timeInfo->tm_mday,
                    timeInfo->tm_hour, timeInfo->tm_min, timeInfo->tm_sec);
                lastTimeDebug = millis();
            }
        } else {
            static unsigned long lastTimeDebug = 0;
            // Only print errors every 60 seconds
            if (millis() - lastTimeDebug > 60000) {
                Serial.println("✗ Failed to obtain time from NTP");
                lastTimeDebug = millis();
            }
        }
        return success;
    }
}

void RTCManager::checkUpdateInterval() {
    // Check if we need to update RTC from NTP (daily)
    if (!rtcPresent || WiFi.status() != WL_CONNECTED) {
        return;
    }
    
    unsigned long currentMillis = millis();
    unsigned long timeSinceUpdate = currentMillis - lastRtcUpdate;
    
    if (lastRtcUpdate == 0) {
        Serial.println("Performing initial RTC update from NTP...");
        updateFromNTP();
    } 
    else if (timeSinceUpdate > RTC_UPDATE_INTERVAL) {
        Serial.printf("RTC update interval reached (%0.1f hours elapsed). Updating from NTP...\n", 
                    timeSinceUpdate / 3600000.0);
        updateFromNTP();
    }
}