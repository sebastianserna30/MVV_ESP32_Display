#include <Arduino.h>
#include "WiFiManager.h"
#include "MVGClient.h"
#include "DisplayManager.h"
#include "config.h"
#include <time.h>

WiFiManager wifiManager;
MVGClient mvgClient;
DisplayManager displayManager;

unsigned long lastUpdateTime = 0;
const unsigned long UPDATE_INTERVAL = 60000; // 1 minute in milliseconds

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting MVG Display...");

    // Initialize display
    displayManager.init();
    if (!displayManager.isInitialized())
    {
        Serial.println("Failed to initialize display!");
        return;
    }

    // Connect to WiFi
    if (!wifiManager.connect())
    {
        Serial.println("Failed to connect to WiFi!");
        return;
    }

    // Configure time
    configTime(3600, 3600, "pool.ntp.org");
}

void loop()
{
    unsigned long currentTime = millis();

    // Check if it's time to update
    if (currentTime - lastUpdateTime >= UPDATE_INTERVAL || lastUpdateTime == 0)
    {
        wifiManager.ensureConnection();

        if (wifiManager.isConnected())
        {
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

        lastUpdateTime = currentTime;
    }

    delay(1000); // Small delay to prevent busy waiting
}
