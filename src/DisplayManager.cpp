#include "DisplayManager.h"
#include <cstdlib>
#include <cstring>
#include <Arduino.h>

DisplayManager::DisplayManager()
    : framebuffer(nullptr), current_y(TOP_MARGIN), display_initialized(false), FONT_LARGE(&FiraSans), FONT_SMALL(&FiraSans)
{
    font_props = {
        .fg_color = 0,
        .bg_color = 255,
        .fallback_glyph = 0,
        .flags = 0x0F // Using proper flag value for 4-bit field
    };
}

void DisplayManager::init()
{
    Serial.println("Initializing display...");

    epd_init();

    framebuffer = (uint8_t *)ps_calloc(EPD_WIDTH * EPD_HEIGHT / 2, 1);
    if (!framebuffer)
    {
        Serial.println("Error: Could not allocate display buffer!");
        display_initialized = false;
        return;
    }

    Serial.println("Framebuffer allocated successfully");
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);

    epd_poweron();
    epd_clear();
    display_initialized = true;

    Serial.println("Display initialized successfully");
}

void DisplayManager::clear()
{
    if (!display_initialized)
        return;
    memset(framebuffer, 0xFF, EPD_WIDTH * EPD_HEIGHT / 2);
    epd_clear();
    current_y = TOP_MARGIN;
}

void DisplayManager::startStationDisplay(const char *station_name)
{
    clear();

    // Draw station name at the top
    int32_t cursor_x = 50;
    int32_t cursor_y = STATION_Y;
    write_string(FONT_LARGE, station_name, &cursor_x, &cursor_y, framebuffer);

    // Draw a line under the station name
    epd_draw_hline(40, STATION_Y + 40, EPD_WIDTH - 80, 0, framebuffer);
    current_y = TOP_MARGIN;

    // Update display
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

bool DisplayManager::displayDeparture(const String &line, const String &destination, const String &time_to_departure)
{
    if (!display_initialized)
        return false;

    int32_t cursor_x, cursor_y;

    // Draw line number (left)
    cursor_x = LINE_X;
    cursor_y = current_y;
    write_string(FONT_LARGE, line.c_str(), &cursor_x, &cursor_y, framebuffer);

    // Draw destination (center)
    cursor_x = DEST_X;
    cursor_y = current_y;
    write_string(FONT_LARGE, destination.c_str(), &cursor_x, &cursor_y, framebuffer);

    // Draw time (right)
    String mins = time_to_departure + " min";
    cursor_x = TIME_X;
    cursor_y = current_y;
    write_string(FONT_LARGE, mins.c_str(), &cursor_x, &cursor_y, framebuffer);

    current_y += LINE_HEIGHT;

    if (current_y > EPD_HEIGHT - LINE_HEIGHT)
    {
        return false;
    }

    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
    return true;
}

void DisplayManager::displaySleepMode()
{
    if (!display_initialized)
        return;

    clear();

    // Display title
    int32_t cursor_x = 50;
    int32_t cursor_y = STATION_Y;
    write_string(FONT_LARGE, "Press left button to search for connections", &cursor_x, &cursor_y, framebuffer);

    // Draw a line under the title
    epd_draw_hline(40, STATION_Y + 40, EPD_WIDTH - 80, 0, framebuffer);

    // Display static departure information
    current_y = TOP_MARGIN;

    // Sample static departures showing typical frequency
    const char *staticDepartures[] = {};

    int numDepartures = sizeof(staticDepartures) / sizeof(staticDepartures[0]);

    for (int i = 0; i < numDepartures && current_y < EPD_HEIGHT - LINE_HEIGHT; i++)
    {
        String departure = staticDepartures[i];
        int firstPipe = departure.indexOf('|');

        if (firstPipe != -1)
        {
            String line = departure.substring(0, firstPipe);
            String frequency = departure.substring(firstPipe + 1);

            // Draw line number
            cursor_x = LINE_X;
            cursor_y = current_y;
            write_string(FONT_LARGE, line.c_str(), &cursor_x, &cursor_y, framebuffer);

            // Draw frequency
            cursor_x = DEST_X;
            cursor_y = current_y;
            write_string(FONT_LARGE, frequency.c_str(), &cursor_x, &cursor_y, framebuffer);

            current_y += LINE_HEIGHT;
        }
    }

    // Update display
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

void DisplayManager::displayConnecting()
{
    if (!display_initialized)
        return;

    clear();

    // Display connecting message
    int32_t cursor_x = 50;
    int32_t cursor_y = STATION_Y;
    write_string(FONT_LARGE, "Live Mode - Connecting to WiFi...", &cursor_x, &cursor_y, framebuffer);

    // Draw a line under the title
    epd_draw_hline(40, STATION_Y + 40, EPD_WIDTH - 80, 0, framebuffer);

    // Update display
    epd_draw_grayscale_image(epd_full_screen(), framebuffer);
}

void DisplayManager::powerOn()
{
    if (display_initialized)
    {
        epd_poweron();
    }
}

void DisplayManager::powerOff()
{
    if (display_initialized)
    {
        epd_poweroff_all();
    }
}