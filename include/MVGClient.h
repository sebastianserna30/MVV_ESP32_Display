#pragma once

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <list>

struct Departure
{
    String line;
    String destination;
    String time_to_departure;
};

struct Station
{
    String station_name;
    std::list<Departure> departure_list;
};

struct Config
{
    String pretty_name;
    String bahnhof;
    String include_type;
    String time_offset;
};

class MVGClient
{
public:
    MVGClient();
    void fetchDepartures();
    const std::list<Station> &getStationList() const { return station_list; }

private:
    static constexpr const char *MVG_BASE_URL = "https://www.mvg.de/api/bgw-pt/v3";
    static constexpr const char *MVG_DEPARTURE_ENDPOINT = "/departures";
    static constexpr size_t MAX_JSON_DOCUMENT = 16384;

    String constructUrl(const Config &config);
    String makeRequest(const String &url);
    void appendToStationList(const char *station_name);

    StaticJsonDocument<MAX_JSON_DOCUMENT> doc;
    std::list<Station> station_list;
};