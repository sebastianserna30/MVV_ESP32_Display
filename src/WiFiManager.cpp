#include "WiFiManager.h"
#include "secrets.h"

WiFiManager::WiFiManager() {}

bool WiFiManager::connect()
{
    return setupWiFi();
}

void WiFiManager::ensureConnection()
{
    if (!isConnected())
    {
        reconnect();
    }
}

bool WiFiManager::setupWiFi()
{
    Serial.println("\n=== Setting up WiFi ===");
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_config.ssid, wifi_config.password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println("");

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Connected to WiFi, IP: ");
        Serial.println(WiFi.localIP());
        return true;
    }
    else
    {
        Serial.println("Failed to connect to WiFi");
        return false;
    }
}

void WiFiManager::reconnect()
{
    Serial.println("Reconnecting to WiFi...");
    WiFi.disconnect();
    delay(1000);
    setupWiFi();
}