#include <Arduino.h>

#include "WiFi.h"

typedef struct {
    const char *ssid;
    const char *password;
} Secret;
#include "secrets.h"

bool setup_wifi();
const int size_of_secrets = sizeof(secrets) / sizeof(*secrets);

bool setup_wifi() {
    int number_of_networks = WiFi.scanNetworks();
    if (number_of_networks == -1) {
        Serial.println("No networks available");
    }
    int wait = 0;
    int wifi_retry = 0;
    for (int i = 0; i < number_of_networks; ++i) {
        String ssid = WiFi.SSID(i);
        // Is this network in the secrets.h file?
        for (int j = 0; j < size_of_secrets; ++j) {
            if (strcmp(ssid.c_str(), secrets[j].ssid) == 0) {
                // ... yes it is
                WiFi.begin(secrets[j].ssid, secrets[j].password);
                while (WiFi.status() != WL_CONNECTED && wait < 5) {
                    delay(1000);
                    ++wait;
                }
                while (WiFi.status() != WL_CONNECTED && wifi_retry < 5) {
                    ++wifi_retry;
                    Serial.println("WiFi not connected. Try to reconnect");
                    WiFi.disconnect();
                    WiFi.mode(WIFI_OFF);
                    WiFi.mode(WIFI_STA);
                    WiFi.begin(secrets[j].ssid, secrets[j].password);
                    Serial.println("Connecting to WiFi...");
                    delay(5000);
                }
                if (wifi_retry >= 5) {
                    Serial.println("\nReboot");
                    ESP.restart();
                } else {
                    Serial.printf("Connected to the WiFi network: %s\n",
                                  ssid.c_str());
                    return true;
                }
            }
        }
    }
    return false;
}

void setup() {
    Serial.begin(115200);
    bool connected = false;
    while (connected == false) {
        connected = setup_wifi();
    }
}

void loop() {}
