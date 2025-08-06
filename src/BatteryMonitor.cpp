#include "BatteryMonitor.h"

BatteryMonitor::BatteryMonitor() : lastVoltage(0.0), vref(1100)
{
}

void BatteryMonitor::init()
{
    Serial.println("Initializing battery monitor...");

    // Correct the ADC reference voltage - code from LilyGO demo
    esp_adc_cal_value_t val_type = esp_adc_cal_characterize(
        ADC_UNIT_2,
        ADC_ATTEN_DB_11,
        ADC_WIDTH_BIT_12,
        1100,
        &adc_chars);

    if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        Serial.printf("eFuse Vref: %umV\r\n", adc_chars.vref);
        vref = adc_chars.vref;
    }

    Serial.println("Battery monitor initialized");
}

float BatteryMonitor::getBatteryVoltage()
{
    // Read ADC value from battery pin
    uint16_t adcValue = analogRead(BATT_PIN);

    // Convert to voltage using the formula from LilyGO demo
    // Battery voltage is divided by 2 through voltage divider
    float voltage = ((float)adcValue / 4095.0) * 2.0 * 3.3 * (vref / 1000.0);

    // Clamp to maximum expected voltage
    if (voltage >= MAX_VOLTAGE)
    {
        voltage = MAX_VOLTAGE;
    }

    lastVoltage = voltage;
    return voltage;
}

int BatteryMonitor::getBatteryPercentage()
{
    float voltage = getBatteryVoltage();

    // Calculate percentage based on voltage range
    if (voltage >= MAX_VOLTAGE)
    {
        return 100;
    }
    else if (voltage <= MIN_VOLTAGE)
    {
        return 0;
    }
    else
    {
        // Linear interpolation between min and max voltage
        float percentage = ((voltage - MIN_VOLTAGE) / (MAX_VOLTAGE - MIN_VOLTAGE)) * 100.0f;
        return (int)percentage;
    }
}

String BatteryMonitor::getBatteryStatus()
{
    float voltage = getBatteryVoltage();
    int percentage = getBatteryPercentage();

    String status = /*String(voltage, 2) +  "V ("*/ "battery:" + String(percentage) + "%";

    if (isLowBattery())
    {
        status += " LOW";
    }

    return status;
}

BatteryLevel BatteryMonitor::getBatteryLevel()
{
    int percentage = getBatteryPercentage();

    if (percentage <= 10)
    {
        return BatteryLevel::CRITICAL;
    }
    else if (percentage <= 25)
    {
        return BatteryLevel::BATTERY_LOW;
    }
    else if (percentage <= 75)
    {
        return BatteryLevel::MEDIUM;
    }
    else
    {
        return BatteryLevel::FULL;
    }
}
