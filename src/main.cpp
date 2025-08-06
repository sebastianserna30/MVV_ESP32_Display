#include <Arduino.h>
#include "WiFiManager.h"
#include "MVGClient.h"
#include "DisplayManager.h"
#include "ModeManager.h"
#include "BatteryMonitor.h"
#include "config.h"
#include <time.h>
#include <esp_sleep.h>

WiFiManager wifiManager;
MVGClient mvgClient;
DisplayManager displayManager;
ModeManager modeManager;
BatteryMonitor batteryMonitor;

unsigned long lastUpdateTime = 0;
unsigned long lastBatteryCheck = 0;
const unsigned long UPDATE_INTERVAL = 60000;         // 1 minute in milliseconds
const unsigned long BATTERY_CHECK_INTERVAL = 300000; // 5 minutes in milliseconds

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting MVG Display...");

    // Initialize mode manager (handles buttons)
    modeManager.init();

    // Initialize battery monitor
    batteryMonitor.init();

    // Small delay to let system stabilize
    delay(1000);

    // Initialize display
    displayManager.init();
    if (!displayManager.isInitialized())
    {
        Serial.println("Failed to initialize display!");
        return;
    }

    // Start in sleep mode - display static information
    displayManager.displaySleepMode();
    displayManager.powerOff();

    Serial.println("Setup complete - entering sleep mode");
}

void loop()
{
    // Update mode manager (handles button presses and timeouts)
    modeManager.update();

    unsigned long currentTime = millis();

    // Check battery status periodically
    if (currentTime - lastBatteryCheck >= BATTERY_CHECK_INTERVAL || lastBatteryCheck == 0)
    {
        displayManager.powerOn(); // Need power to read battery
        delay(10);                // Stabilize ADC
        String batteryStatus = batteryMonitor.getBatteryStatus();
        Serial.println("Battery: " + batteryStatus);

        // Display battery status text
        displayManager.displayBatteryStatus(batteryStatus);

        lastBatteryCheck = currentTime;

        // Check for low battery
        if (batteryMonitor.isLowBattery())
        {
            Serial.println("Warning: Low battery detected!");
        }
    }

    if (modeManager.getCurrentMode() == DisplayMode::SLEEP)
    {
        // In sleep mode, just wait for button press
        if (modeManager.shouldEnterSleep())
        {
            Serial.println("Entering deep sleep...");
            displayManager.powerOff();
            esp_deep_sleep_start();
        }
        delay(100);
        return;
    }

    // Live mode - handle data updates
    if (modeManager.getCurrentMode() == DisplayMode::LIVE)
    {
        unsigned long currentTime = millis();

        // Check if it's time to update or if we just switched to live mode
        if (currentTime - lastUpdateTime >= UPDATE_INTERVAL || lastUpdateTime == 0)
        {
            Serial.println("Live mode - powering on display and connecting to WiFi...");
            if (!wifiManager.isConnected())
            {
                Serial.println("Powering on display and connecting to WiFi...");
                wifiManager.connect();
                displayManager.powerOn();
                displayManager.displayConnecting();
                delay(1000);
            }

            wifiManager.ensureConnection();

            if (wifiManager.isConnected())
            {
                Serial.println("Fetching live departures...");
                mvgClient.fetchDepartures();

                // Display departures for each station
                const auto &stations = mvgClient.getStationList();
                for (const auto &station : stations)
                {
                    displayManager.startStationDisplay(station.station_name.c_str());

                    for (const auto &departure : station.departure_list)
                    {
                        if (!displayManager.displayDeparture(
                                departure.line,
                                departure.destination,
                                departure.time_to_departure))
                        {
                            break; // Stop if display is full
                        }
                    }

                    delay(5000); // Show each station for 5 seconds
                }
            }
            else
            {
                Serial.println("WiFi connection failed in live mode");
                // Show error message on display
                displayManager.clear();
                int32_t cursor_x = 50;
                int32_t cursor_y = 300;
                // Note: You might need to add an error display method to DisplayManager
            }

            lastUpdateTime = currentTime;
        }

        // Check if we should go back to sleep mode
        if (modeManager.shouldEnterSleep())
        {
            Serial.println("Returning to sleep mode...");
            modeManager.enterSleepMode();
            displayManager.displaySleepMode();
            displayManager.powerOff();
            lastUpdateTime = 0; // Reset update timer
        }
    }

    delay(1000); // Small delay to prevent busy waiting
}
