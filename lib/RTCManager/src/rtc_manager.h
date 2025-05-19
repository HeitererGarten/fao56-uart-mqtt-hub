#pragma once

#include <Arduino.h>
#include <RTClib.h>
#include <Wire.h>
#include <WiFi.h>
#include "config.h"
#include "time.h"

class RTCManager {
public:
    RTCManager();
    bool begin();
    bool updateFromNTP();
    bool getCurrentTime(struct tm* timeInfo);
    bool isPresent() { return rtcPresent; }
    void checkUpdateInterval();

private:
    RTC_DS3231 rtc;
    bool rtcPresent;
    unsigned long lastRtcUpdate;
};