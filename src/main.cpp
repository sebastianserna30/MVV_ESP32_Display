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
    String bahnhof;      // Changed from const char* to String
    String include_type; // Changed from const char* to String
    String time_offset;  // Changed from const char* to String
} Config;
#include "config.h"
const int configs_size = sizeof(configs) / sizeof(*configs);

// MVG API v3
#include <HTTPClient.h>
#define MVG_BASE_URL "https://www.mvg.de/api/bgw-pt/v3"
#define MVG_DEPARTURE_ENDPOINT "/departures"

// Increase JSON document size for larger responses
#define MAX_JSON_DOCUMENT 16384 // 16KB

void call_mvg_api();
String constructUrl(const Config &config);
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
void append_to_station_list(const char *station_name);

// Display includes
#include <epd_driver.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <firasans.h>

// Display configuration
const int STATION_Y = 20;
const int TOP_MARGIN = 80;
const int LINE_HEIGHT = 52;
const int LINE_X = 50;
const int DEST_X = 180;
const int TIME_X = 800;

uint8_t *framebuffer = NULL;
int current_y = TOP_MARGIN;

// Font configuration
const GFXfont *FONT_LARGE = &FiraSans;
const GFXfont *FONT_SMALL = &FiraSans;

FontProperties font_props = {
    .fg_color = 0,
    .bg_color = 255,
    .fallback_glyph = 0,
    .flags = 0};

bool display_initialized = false;

void init_display()
{
    epd_init();
    framebuffer = (uint8_t *)ps_calloc(EPD_WIDTH * EPD_HEIGHT / 2, 1);
    if (!framebuffer)
    {
        Serial.println("Error: Could not allocate display buffer!");
        return;
    }
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    epd_poweron();
    epd_clear();
    display_initialized = true;
}

void clear_display()
{
    if (!display_initialized)
        return;

    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    epd_clear();
    current_y = TOP_MARGIN;
}

void start_station_display(const char *station_name)
{
    clear_display();

    // Draw station name at the top
    int32_t cursor_x = 50;
    int32_t cursor_y = STATION_Y;
    write_mode(FONT_LARGE, station_name, &cursor_x, &cursor_y, framebuffer, BLACK_ON_WHITE, &font_props);

    // Draw a line under the station name
    epd_draw_hline(40, STATION_Y + 40, EPD_WIDTH - 80, 0, framebuffer);

    current_y = TOP_MARGIN;

    // Update display
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

void display_departure(const String &line, const String &destination, const String &time_to_departure)
{
    if (!display_initialized)
        return;

    int32_t cursor_x, cursor_y;

    // Draw line number (left)
    cursor_x = LINE_X;
    cursor_y = current_y;
    write_mode(FONT_LARGE, line.c_str(), &cursor_x, &cursor_y, framebuffer, BLACK_ON_WHITE, &font_props);

    // Draw destination (center)
    cursor_x = DEST_X;
    cursor_y = current_y;
    write_mode(FONT_LARGE, destination.c_str(), &cursor_x, &cursor_y, framebuffer, BLACK_ON_WHITE, &font_props);

    // Draw time (right)
    String mins = time_to_departure + " min";
    cursor_x = TIME_X;
    cursor_y = current_y;
    write_mode(FONT_LARGE, mins.c_str(), &cursor_x, &cursor_y, framebuffer, BLACK_ON_WHITE, &font_props);

    current_y += LINE_HEIGHT;

    // If we've filled the screen, update it
    if (current_y > EPD_HEIGHT - LINE_HEIGHT)
    {
        epd_draw_grayscale_image(epd_full_screen(), framebuffer);
        delay(2000); // Give time to read
        clear_display();
    }
}

// Update the original display_departures to use the new functions
void display_departures(String line, String destination, String time_to_departure)
{
    Serial.println(line + " to " + destination + " in " + time_to_departure + " minutes");
    display_departure(line, destination, time_to_departure);
}

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
    init_display();
    clear_display();
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
    if (!display_initialized)
    {
        Serial.println("Display not initialized, skipping update");
        return;
    }

    Serial.println("\n=== Fetching MVG departures ===");

    time_t start;
    time_t now;
    time(&start);
    time(&now);

    // Clear previous station list
    station_list.clear();
    Serial.printf("Processing %d stations\n", configs_size);

    for (int i = 0; i < configs_size; ++i)
    {
        Serial.print("Raw station ID from config: ");
        Serial.println(configs[i].bahnhof);

        Serial.printf("\nStation %d: %s\n", i + 1, configs[i].bahnhof.c_str());

        // construct the url based on the config
        String url = constructUrl(configs[i]);

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
            append_to_station_list(configs[i].bahnhof.c_str());
        }

        // Small delay between requests to avoid overwhelming the API
        delay(1000);
    }

    Serial.println("\n=== Processing departures for display ===");

    // Process and display the departures
    list<Station>::iterator station;
    for (station = station_list.begin(); station != station_list.end(); ++station)
    {
        Serial.printf("\nDisplaying departures for station: %s\n",
                      station->station_name.c_str());

        start_station_display(station->station_name.c_str());

        list<Departure>::iterator departure;
        for (departure = station->departure_list.begin();
             departure != station->departure_list.end(); ++departure)
        {
            display_departures(departure->line, departure->destination,
                               departure->time_to_departure);
        }

        // Update display with any remaining content
        if (current_y > TOP_MARGIN)
        {
            epd_draw_grayscale_image(epd_full_screen(), framebuffer);
        }

        // Check if we've been running for more than 60 seconds
        time(&now);
        if (difftime(now, start) > 60)
        {
            break;
        }
        else
        {
            delay(3700); // Display each screen for about 3.7 seconds
        }
    }

    Serial.println("\n=== Finished processing departures ===\n");
}

String constructUrl(const Config &config)
{
    // Correct MVG API v3 format:
    // https://www.mvg.de/api/bgw-pt/v3/departures?globalId=de:09162:170
    String url = MVG_BASE_URL;
    url += MVG_DEPARTURE_ENDPOINT;
    url += "?globalId=" + config.bahnhof;

    // Add limit parameter
    url += "&limit=10";

    // Add transport types if specified
    if (!config.include_type.isEmpty())
    {
        url += "&transportTypes=" + config.include_type;
    }

    // Add time offset if specified
    if (!config.time_offset.isEmpty() && config.time_offset != "0")
    {
        url += "&offsetInMinutes=" + config.time_offset;
    }

    return url;
}

String makeRequest(String url)
{
    HTTPClient http;
    http.begin(url);
    http.addHeader("Accept", "application/json");
    http.addHeader("User-Agent", "MVG_ESP32_Display/1.0");

    Serial.println("Making request to MVG API: " + url);
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

void append_to_station_list(const char *station_name)
{
    Serial.println("\nProcessing departures for station: " + String(station_name));

    // The MVG API returns departures in the root array
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
        String line = departure["label"].as<const char *>();
        const char *transportType = departure["transportType"].as<const char *>();

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
            line = "" + line;
        }
        else if (strcmp(transportType, "BUS") == 0)
        {
            line = "B" + line;
        }

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

        if (minutes_to_departure > 0)
        {
            Departure dep = {line, destination, String(minutes_to_departure)};
            departure_list.push_back(dep);
        }
    }

    if (!departure_list.empty())
    {
        Station station;
        station.station_name = String(station_name); // Create String from char*
        station.departure_list = departure_list;
        station_list.push_back(station);
        Serial.printf("Added %d departures for station %s\n",
                      departure_list.size(),
                      station_name);
    }
    else
    {
        Serial.println("No valid departures found for station");
    }
}
