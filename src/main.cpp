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
typedef struct
{
    const char *ssid;
    const char *password;
} Secret;
#include "secrets.h"

bool setup_wifi();
void loop_wifi_connect();
const int size_of_secrets = sizeof(secrets) / sizeof(*secrets);

// Stations config struct
typedef struct
{
    const char *bahnhof;
    const char *include_type;
    const char *time_offset;
} Config;
#include "config.h"
const int configs_size = sizeof(configs) / sizeof(*configs);

// MVG API v3
#include <HTTPClient.h>
// Local Python server endpoint
#define SERVER_IP "192.168.1.XXX" // Replace with your computer's IP address
#define SERVER_PORT "8000"
#define BASE_URL "http://" SERVER_IP ":" SERVER_PORT
#define API_ENDPOINT "/departures/"

// Increase JSON document size for larger responses
#define MAX_JSON_DOCUMENT 16384 // 16KB

void call_mvg_api();
String constructUrl(String baseUrl, Config config);
String makeRequest(String url);

// Use arduinojson.org/v6/assistant to compute the capacity.
StaticJsonDocument<MAX_JSON_DOCUMENT> doc;

typedef struct
{
    String line;
    String destination;
    String time_to_departure;
} Departure;

typedef struct
{
    String station_name;
    list<Departure> departure_list;
} Station;
list<Station> station_list;
void append_to_station_list(String station_name);

// Display
void display_departures(String line, String destination,
                        String time_to_departure);

void loop_wifi_connect()
{
    bool connected = false;
    while (connected == false)
    {
        connected = setup_wifi();
    }
}

bool setup_wifi()
{
    int number_of_networks = WiFi.scanNetworks();
    if (number_of_networks == -1)
    {
        Serial.println("No networks available");
    }
    int wait = 0;
    int wifi_retry = 0;
    for (int i = 0; i < number_of_networks; ++i)
    {
        String ssid = WiFi.SSID(i);
        // Is this network in the secrets.h file?
        for (int j = 0; j < size_of_secrets; ++j)
        {
            if (strcmp(ssid.c_str(), secrets[j].ssid) == 0)
            {
                // ... yes it is
                WiFi.begin(secrets[j].ssid, secrets[j].password);
                while (WiFi.status() != WL_CONNECTED && wait < 5)
                {
                    delay(1000);
                    ++wait;
                }
                while (WiFi.status() != WL_CONNECTED && wifi_retry < 5)
                {
                    ++wifi_retry;
                    Serial.println("WiFi not connected. Try to reconnect");
                    WiFi.disconnect();
                    WiFi.mode(WIFI_OFF);
                    WiFi.mode(WIFI_STA);
                    WiFi.begin(secrets[j].ssid, secrets[j].password);
                    Serial.println("Connecting to WiFi...");
                    delay(5000);
                }
                if (wifi_retry >= 5)
                {
                    Serial.println("\nReboot");
                    ESP.restart();
                }
                else
                {
                    Serial.printf("Connected to the WiFi network: %s\n",
                                  ssid.c_str());
                    return true;
                }
            }
        }
    }
    return false;
}

void setup()
{
    Serial.begin(115200);
    loop_wifi_connect();
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
}

void loop()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        call_mvg_api();
    }
    else
    {
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
void call_mvg_api()
{
    Serial.println("\n=== Fetching MVG departures ===");

    // create multple http clients for each station in the config array
    time_t start;
    time_t now;
    time(&start);
    time(&now);

    // Clear previous station list
    station_list.clear();
    Serial.printf("Processing %d stations\n", configs_size);

    for (int i = 0; i < configs_size; ++i)
    {
        Serial.printf("\nStation %d: %s\n", i + 1, configs[i].bahnhof);

        // construct the url based on the config
        String url = constructUrl(BASE_URL, configs[i]);

        // Get the response
        String response = makeRequest(url);

        // Clear the JSON document before parsing new data
        doc.clear();

        // deserialize the json response
        DeserializationError error = deserializeJson(doc, response);
        if (error)
        {
            Serial.print(F("deserializeJson() failed: "));
            Serial.println(error.c_str());
        }
        else
        {
            // add response to the list of departures
            append_to_station_list(configs[i].bahnhof);
        }

        // Small delay between requests to avoid overwhelming the API
        delay(1000);
    }

    Serial.println("\n=== Processing departures for display ===");

    list<Station>::iterator station;
    for (station = station_list.begin(); station != station_list.end();
         ++station)
    {
        Serial.printf("\nDisplaying departures for station: %s\n",
                      station->station_name.c_str());

        list<Departure>::iterator departure;
        for (departure = station->departure_list.begin();
             departure != station->departure_list.end(); ++departure)
        {
            display_departures(departure->line, departure->destination,
                               departure->time_to_departure);
        }

        // loop for 60 seconds and show each screen 30 seconds.
        if (difftime(now, start) > 60)
        {
            break;
        }
        else
        {
            delay(3700);
        }
    }

    Serial.println("\n=== Finished processing departures ===\n");
}

String constructUrl(String baseUrl, Config config)
{
    // Format: http://192.168.1.XXX:8000/departures/de:09162:170?transport_types=TRAM,UBAHN
    String url = String(BASE_URL) + API_ENDPOINT + String(config.bahnhof);

    // Add transport types if specified
    if (config.include_type && strlen(config.include_type) > 0)
    {
        url += "?transport_types=" + String(config.include_type);
    }

    url += url.indexOf('?') == -1 ? "?" : "&";
    url += "limit=10";

    return url;
}

String makeRequest(String url)
{
    HTTPClient http;
    http.begin(url);
    http.addHeader("Accept", "application/json");

    Serial.println("Making request to local server: " + url);
    int httpResponseCode = http.GET();
    String payload = "[]";

    if (httpResponseCode > 0)
    {
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
        payload = http.getString();
        Serial.println("Response payload: " + payload);
    }
    else
    {
        Serial.print("Error: failed GET: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return payload;
}

void display_departures(String line, String destination,
                        String time_to_departure)
{
    Serial.println(line + " to " + destination + " in " + time_to_departure +
                   " minutes");
}

void append_to_station_list(String station_name)
{
    Serial.println("\nProcessing departures for station: " + station_name);

    // The new API structure is different, departures are in the root array
    JsonArray departures = doc.as<JsonArray>();
    if (departures.isNull())
    {
        Serial.println("No departures array found in response!");
        Serial.println("Raw JSON content:");
        serializeJsonPretty(doc, Serial);
        return;
    }

    Serial.printf("Found %d departures\n", departures.size());

    list<Departure> departure_list;

    for (JsonVariant departure : departures)
    {
        // Get the line name and type
        String line = departure["product"]["name"].as<const char *>();
        const char *transportType = departure["product"]["type"].as<const char *>();

        Serial.printf("\nProcessing departure: Line %s (%s)\n", line.c_str(), transportType);

        // Add prefix for transport type
        if (strcmp(transportType, "TRAM") == 0)
        {
            line = "T" + line;
        }
        else if (strcmp(transportType, "SBAHN") == 0)
        {
            line = "S" + line;
        }
        else if (strcmp(transportType, "UBAHN") == 0)
        {
            line = "U" + line;
        }
        else if (strcmp(transportType, "BUS") == 0)
        {
            line = "B" + line;
        }

        // Get the destination
        String destination = departure["destination"].as<const char *>();

        // Calculate time to departure
        time_t now;
        time(&now);
        unsigned long departure_time = departure["departureTime"].as<long long>() / 1000; // ms to seconds

        // Use realtime departure if available
        if (departure.containsKey("realtimeDepartureTime"))
        {
            departure_time = departure["realtimeDepartureTime"].as<long long>() / 1000;
        }

        unsigned long minutes_to_departure = 0;
        if (departure_time > now)
        {
            unsigned long wait = departure_time - now;
            minutes_to_departure = wait / 60;
        }

        Serial.printf("Line: %s to %s in %lu minutes\n",
                      line.c_str(),
                      destination.c_str(),
                      minutes_to_departure);

        // Only add departures that haven't left yet
        if (minutes_to_departure > 0)
        {
            Departure dep = {line, destination, String(minutes_to_departure)};
            departure_list.push_back(dep);
        }
    }

    // Only add station if we have departures
    if (!departure_list.empty())
    {
        Station station = {station_name, departure_list};
        station_list.push_back(station);
        Serial.printf("Added %d departures for station %s\n",
                      departure_list.size(),
                      station_name.c_str());
    }
    else
    {
        Serial.println("No valid departures found for station");
    }
}
