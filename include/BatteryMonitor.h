#pragma once

#include <Arduino.h>
#include "utilities.h"
#include "esp_adc_cal.h"

enum class BatteryLevel
{
    CRITICAL,
    BATTERY_LOW,
    MEDIUM,
    FULL
};

class BatteryMonitor
{
public:
    BatteryMonitor();
    void init();
    float getBatteryVoltage();
    int getBatteryPercentage();
    String getBatteryStatus();
    BatteryLevel getBatteryLevel();
    bool isLowBattery() const { return lastVoltage < 3.3f; }

private:
    float lastVoltage;
    int vref;
    esp_adc_cal_characteristics_t adc_chars;
    static constexpr float MIN_VOLTAGE = 3.0f;
    static constexpr float MAX_VOLTAGE = 4.2f;
    static constexpr float LOW_BATTERY_THRESHOLD = 3.3f;
};
