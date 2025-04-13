#pragma once

#include <Arduino.h>
#include <WiFi.h>

struct Secret
{
    const char *ssid;
    const char *password;
};

class WiFiManager
{
public:
    WiFiManager();
    bool connect();
    void ensureConnection();
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

private:
    bool setupWiFi();
    void reconnect();
};