#pragma once

#include <WString.h>
#include <epd_driver.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <firasans.h>
#include <firasans_small.h>

class DisplayManager
{
public:
    DisplayManager();
    void init();
    void clear();
    void startStationDisplay(const char *station_name);
    bool displayDeparture(const String &line, const String &destination, const String &time_to_departure);
    void displaySleepMode();
    void displayConnecting();
    void displayBatteryStatus(const String &batteryStatus);
    void powerOn();
    void powerOff();
    bool isInitialized() const { return display_initialized; }

private:
    static const int STATION_Y = 100;
    static const int TOP_MARGIN = 200;
    static const int LINE_HEIGHT = 52;
    static const int LINE_X = 50;
    static const int DEST_X = 180;
    static const int TIME_X = 800;

    uint8_t *framebuffer;
    int current_y;
    bool display_initialized;

    const GFXfont *FONT_LARGE;
    const GFXfont *FONT_SMALL;
    FontProperties font_props;
};