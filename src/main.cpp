#include <Arduino.h>

#include "WiFi.h"

// Wifi config struct
typedef struct {
    const char *ssid;
    const char *password;
} Secret;
#include "secrets.h"

// Stations config struct
typedef struct {
    const char *bahnhof;
    const char *include_type;
    const char *time_offset;
} Config;
#include "config.h"

bool setup_wifi();
void loop_wifi_connect();
const int size_of_secrets = sizeof(secrets) / sizeof(*secrets);

// MVG API
#include <HTTPClient.h>
#define BASE_URL "https://www.mvg.de/api/fib/v2/departure?globalId="
void call_mvg_api();
String constructUrl(String baseUrl, Config config);
void makeRequest(String url);

void loop_wifi_connect() {
    bool connected = false;
    while (connected == false) {
        connected = setup_wifi();
    }
}

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
    loop_wifi_connect();
}

void loop() {
    if (WiFi.status() == WL_CONNECTED) {
        call_mvg_api();
    } else {
        Serial.println("Error in WiFi connection");
        Serial.println("Try to Reconnect WiFi");
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);
        WiFi.mode(WIFI_STA);
        loop_wifi_connect();
    }
}

/* The goal is to have a config array that contains the stations to monitor.
 * The array should be in the config.h file.
 * The array should be a struct with the following fields:
 * - station id
 * - included transportation types
 * - included lines
 * - excluded destinations
 *
 * In my use case I need to monitor the following stations:
 * - Leuchtenbergring (s-bahn)
 * - Prinzregentenplatz (ubahn)
 * - Einsteinstraße (tram+bus)
 * - Grillparzerstraße (tram+bus)
 *
 * Ideally every minute we call the mvg api. (Avoid too much calls and avoid
 * waisting battery)
 */
void call_mvg_api() {
    // create multple http clients for each station in the config array

    for (const auto &config : configs) {
        String url = constructUrl(BASE_URL, config);
        makeRequest(url);
    }
}

String constructUrl(String baseUrl, Config config) {
    // call example:
    // https://www.mvg.de/api/fib/v2/departure?globalId=de:09162:662&offsetInMinutes=10&limit=10&transportTypes=UBAHN,BUS,TRAM
    return baseUrl + config.bahnhof + "&offsetInMinutes=" + config.time_offset +
           "&limit=2" + "&transportTypes=" + config.include_type;
}

void makeRequest(String url) {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        Serial.print("HTTP Response code: ");
        Serial.println(httpResponseCode);
        String payload = http.getString();
        Serial.println(payload);
    } else {
        Serial.print("Error: failed GET: ");
        Serial.println(httpResponseCode);
    }
}
