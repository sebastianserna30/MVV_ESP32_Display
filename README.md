# MVV ESP32 E-Paper Display

Displays real-time Munich public transit (MVV) departures on a 4.7" e-paper screen powered by an ESP32-S3.

## Hardware

- [LilyGo T5-ePaper-S3](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47) — ESP32-S3 with integrated 4.7" e-paper display
- LiPo battery (connected via onboard connector)

## Features

- **Live mode**: fetches departures from the MVG API every 60 seconds and cycles through configured stations
- **Sleep mode**: low-power standby, display off, waiting for a button press
- **Auto-sleep**: returns to sleep mode after 5 minutes of inactivity
- **Battery indicator**: shows charge percentage in the top-right corner
- **Configurable stations**: any number of stations and transport type filters

## Setup

### 1. Install dependencies

This project uses [PlatformIO](https://platformio.org/). Install the PlatformIO CLI or the VS Code extension.

### 2. Configure WiFi credentials

Copy the template and fill in your credentials:

```bash
cp include/secrets.h.dist include/secrets.h
```

Edit `include/secrets.h`:

```cpp
static const WifiConfig wifi_config = {
    /* ssid     */ "your-wifi-ssid",
    /* password */ "your-wifi-password",
};
```

### 3. Configure stations

Edit `include/config.h` to set which stations and transport types to display:

```cpp
static const Config configs[] = {
    {
        /* pretty_name  */ String("Stiglmaierpl."),
        /* bahnhof      */ String("de:09162:170"),   // MVG station ID
        /* include_type */ String("TRAM,UBAHN"),      // TRAM, UBAHN, SBAHN, BUS
        /* time_offset  */ String("5"),               // skip departures within N minutes
    },
};
```

Station IDs can be found via the MVG API or tools like [this one](https://github.com/mondbaron/mvg).

### 4. Build and flash

```bash
pio run -t upload
```

## Usage

| Action | Result |
|---|---|
| Press any button (sleep mode) | Enter live mode, fetch departures |
| No button press for 5 min | Return to sleep mode |

## Project Structure

```
├── src/
│   ├── main.cpp            # App loop and mode transitions
│   ├── MVGClient.cpp       # MVG API HTTP client and JSON parsing
│   ├── ModeManager.cpp     # SLEEP/LIVE mode and button handling
│   ├── DisplayManager.cpp  # E-paper framebuffer rendering
│   ├── WiFiManager.cpp     # WiFi connection and NTP sync
│   └── BatteryMonitor.cpp  # ADC battery voltage monitoring
├── include/
│   ├── config.h            # Station configuration
│   ├── secrets.h           # WiFi credentials (not in repo)
│   └── secrets.h.dist      # Credentials template
├── boards/
│   └── T5-ePaper-S3.json   # Custom board definition
└── platformio.ini
```

## MVG API

Uses the unofficial MVG API at `https://www.mvg.de/api/bgw-pt/v3/departures`. There is no official documentation.

## References

- [LilyGo-EPD47](https://github.com/Xinyuan-LilyGO/LilyGo-EPD47) — e-paper display driver
- [florianlederer/mvv-display-for-ESP32](https://github.com/florianlederer/mvv-display-for-ESP32) — original inspiration
- [mondbaron/mvg](https://github.com/mondbaron/mvg) — MVG API reference
