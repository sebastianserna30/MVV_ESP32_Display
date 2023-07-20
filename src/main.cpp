#include <Arduino.h>

#include "WiFi.h"
#include "time.h"
// mvg api timestamp in ms needs long long
#define ARDUINOJSON_USE_LONG_LONG 1
#include <ArduinoJson.h>

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;

#include <iterator>
#include <list>
using namespace std;

// Wifi config struct
typedef struct {
    const char *ssid;
    const char *password;
} Secret;
#include "secrets.h"

bool setup_wifi();
void loop_wifi_connect();
const int size_of_secrets = sizeof(secrets) / sizeof(*secrets);

// Stations config struct
typedef struct {
    const char *bahnhof;
    const char *include_type;
    const char *time_offset;
} Config;
#include "config.h"
const int configs_size = sizeof(configs) / sizeof(*configs);

// MVG API
#include <HTTPClient.h>
#define BASE_URL "https://www.mvg.de/api/fib/v2/departure?globalId="
void call_mvg_api();
String constructUrl(String baseUrl, Config config);
String makeRequest(String url);

// Use arduinojson.org/v6/assistant to compute the capacity.
#define MAX_JSON_DOCUMENT 2000
StaticJsonDocument<MAX_JSON_DOCUMENT> doc;

typedef struct {
    String line;
    String destination;
    String time_to_departure;
} Departure;

typedef struct {
    String station_name;
    list<Departure> departure_list;
} Station;
list<Station> station_list;
void append_to_station_list(String station_name);

// Display
void display_departures(String line, String destination,
                        String time_to_departure);

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
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
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

    time_t start;
    time_t now;
    time(&start);
    time(&now);

    JsonArray responseArray = doc.createNestedArray("responseArray");

    for (int i = 0; i < configs_size; ++i) {
        // array of String responses
        String responses[configs_size];

        // construct the url based on the config
        String url = constructUrl(BASE_URL, configs[i]);

        // append the response to the array
        responses[i] = makeRequest(url);

        // deserialize the json response
        DeserializationError error = deserializeJson(doc, responses[i]);
        if (error) {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
        } else {
            // add response to the list of departures
            append_to_station_list(configs[i].bahnhof);
        }
    }

    list<Station>::iterator station;
    for (station = station_list.begin(); station != station_list.end();
         ++station) {
        list<Departure>::iterator departure;
        for (departure = station->departure_list.begin();
             departure != station->departure_list.end(); ++departure) {
            display_departures(departure->line, departure->destination,
                               departure->time_to_departure);
        }

        // loop for 60 seconds and show each screen 30 seconds.
        if (difftime(now, start) > 60) {
            break;
        } else {
            delay(3700);
        }
    }
}

String constructUrl(String baseUrl, Config config) {
    // call example:
    // https://www.mvg.de/api/fib/v2/departure?globalId=de:09162:662&offsetInMinutes=10&limit=10&transportTypes=UBAHN,BUS,TRAM
    return baseUrl + config.bahnhof + "&offsetInMinutes=" + config.time_offset +
           "&limit=2" + "&transportTypes=" + config.include_type;
}

String makeRequest(String url) {
    HTTPClient http;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode > 0) {
        // TODO: turn this to DEBUG logs.
        // Serial.print("HTTP Response code: ");
        // Serial.println(httpResponseCode);
        String payload = http.getString();
        // Serial.println(payload);
        return payload;
    } else {
        Serial.print("Error: failed GET: ");
        Serial.println(httpResponseCode);
        return "[]";
    }
}

void display_departures(String line, String destination,
                        String time_to_departure) {
    Serial.println(line + " to " + destination + " in " + time_to_departure +
                   " minutes");
}

void append_to_station_list(String station_name) {
    JsonArray departures = doc.as<JsonArray>();
    list<Departure> departure_list;

    // loop through the array
    for (JsonVariant departure : departures) {
        // get the line name
        String line = departure["label"];
        if (!strcmp("TRAM", departure["transportType"])) {
            line = "T" + line;
        }
        // get the destination
        String destination = departure["destination"];
        // get the departure time
        time_t now;
        time(&now);
        unsigned long departure_time =
            departure["realtimeDepartureTime"].as<long long>() /
            1000;  // ms to seconds
        unsigned long minutes_to_departure = 0;
        if (departure_time > now) {
            unsigned long wait = departure_time - now;
            minutes_to_departure = wait / 60;
        }

        Departure dep = {line, destination, String(minutes_to_departure)};
        departure_list.push_back(dep);
    }

    Station station = {station_name, departure_list};
    station_list.push_back(station);
}
