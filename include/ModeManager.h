#pragma once

#include <Button2.h>
#include <Arduino.h>

enum class DisplayMode
{
    SLEEP,
    LIVE
};

class ModeManager
{
public:
    ModeManager();
    void init();
    void update();
    DisplayMode getCurrentMode() const { return currentMode; }
    bool shouldEnterSleep() const { return sleepRequested; }
    void enterSleepMode();
    void enterLiveMode();

private:
    Button2 btn1;
#if defined(CONFIG_IDF_TARGET_ESP32)
    Button2 btn2;
    Button2 btn3;
#endif

    DisplayMode currentMode;
    bool sleepRequested;
    unsigned long lastInteraction;
    static const unsigned long LIVE_MODE_TIMEOUT = 300000; // 5 minutes

    static void onButtonPressed(Button2 &btn);
    void handleButtonPress();
};
