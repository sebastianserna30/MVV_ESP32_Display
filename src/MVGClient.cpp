#include "MVGClient.h"
#include "config.h"
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <time.h>

MVGClient::MVGClient() {}

void MVGClient::fetchDepartures()
{
    Serial.println("\n=== Fetching MVG departures ===");
    station_list.clear();

    // Iterate through configured stations
    for (const Config &config : configs)
    {
        Serial.printf("\nStation: %s\n", config.pretty_name.c_str());

        String url = constructUrl(config);

        delay(50);
        String response = makeRequest(url);

        if (!response.isEmpty())
        {
            doc.clear();
            DeserializationError error = deserializeJson(doc, response);
            if (error)
            {
                Serial.print(F("deserializeJson() failed: "));
                Serial.println(error.c_str());
            }
            else
            {
                appendToStationList(config.pretty_name.c_str());
            }
        }
    }
}

String MVGClient::constructUrl(const Config &config)
{
    String url = MVG_BASE_URL;
    url += MVG_DEPARTURE_ENDPOINT;
    url += "?globalId=" + config.bahnhof;
    url += "&limit=5";

    if (!config.include_type.isEmpty())
    {
        url += "&transportTypes=" + config.include_type;
    }

    if (!config.time_offset.isEmpty() && config.time_offset != "0")
    {
        url += "&offsetInMinutes=" + config.time_offset;
    }

    return url;
}

String MVGClient::makeRequest(const String &url)
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
    }
    else
    {
        Serial.print("Error: failed GET: ");
        Serial.println(httpResponseCode);
    }

    http.end();
    return payload;
}

void MVGClient::appendToStationList(const char *station_name)
{
    JsonArray departures = doc.as<JsonArray>();
    if (departures.isNull())
    {
        Serial.println("No departures array found in response!");
        return;
    }

    Station station;
    station.station_name = String(station_name);

    for (JsonVariant departure : departures)
    {
        String line = departure["label"].as<const char *>();
        const char *transportType = departure["transportType"].as<const char *>();

        if (strcmp(transportType, "TRAM") == 0)
            line = "T" + line;
        else if (strcmp(transportType, "SBAHN") == 0)
            line = "S" + line;
        else if (strcmp(transportType, "UBAHN") == 0)
            line = "" + line;
        else if (strcmp(transportType, "BUS") == 0)
            line = "B" + line;

        String destination = departure["destination"].as<const char *>();

        time_t now;
        time(&now);
        unsigned long departure_time = departure["departureTime"].as<long long>() / 1000;

        if (departure.containsKey("realtimeDepartureTime"))
        {
            departure_time = departure["realtimeDepartureTime"].as<long long>() / 1000;
        }

        unsigned long minutes_to_departure = 0;
        if (departure_time > now)
        {
            minutes_to_departure = (departure_time - now) / 60;

            Departure dep = {
                line,
                destination,
                String(minutes_to_departure)};
            station.departure_list.push_back(dep);

            Serial.print(line);
            Serial.print(" to ");
            Serial.print(destination);
            Serial.print(" in ");
            Serial.print(minutes_to_departure);
            Serial.println(" minutes \n");
        }
    }

    if (!station.departure_list.empty())
    {
        station_list.push_back(station);
        Serial.printf("Added %d departures for station %s\n",
                      station.departure_list.size(),
                      station_name);
    }
}