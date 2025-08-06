#include "ModeManager.h"
#include <esp_sleep.h>
#include "utilities.h"

// Static instance pointer for button callback
static ModeManager *instance = nullptr;

ModeManager::ModeManager()
    : btn1(BUTTON_1), currentMode(DisplayMode::SLEEP), sleepRequested(false), lastInteraction(0)
{
    instance = this;

#if defined(CONFIG_IDF_TARGET_ESP32)
    btn2 = Button2(BUTTON_2);
    btn3 = Button2(BUTTON_3);
#endif
}

void ModeManager::init()
{
    Serial.println("Initializing ModeManager...");

    // Configure button handlers
    btn1.setPressedHandler(onButtonPressed);

#if defined(CONFIG_IDF_TARGET_ESP32)
    btn2.setPressedHandler(onButtonPressed);
    btn3.setPressedHandler(onButtonPressed);
#endif

    // Start in sleep mode
    currentMode = DisplayMode::SLEEP;
    lastInteraction = millis();

    Serial.println("ModeManager initialized in SLEEP mode");
}

void ModeManager::update()
{
    // Update button states
    btn1.loop();

#if defined(CONFIG_IDF_TARGET_ESP32)
    btn2.loop();
    btn3.loop();
#endif

    // Check for automatic sleep timeout in live mode
    if (currentMode == DisplayMode::LIVE)
    {
        if (millis() - lastInteraction > LIVE_MODE_TIMEOUT)
        {
            Serial.println("Live mode timeout - requesting sleep");
            sleepRequested = true;
        }
    }
}

void ModeManager::onButtonPressed(Button2 &btn)
{
    if (instance)
    {
        instance->handleButtonPress();
    }
}

void ModeManager::handleButtonPress()
{
    Serial.println("Button pressed!");
    lastInteraction = millis();

    if (currentMode == DisplayMode::SLEEP)
    {
        // Wake up to live mode
        enterLiveMode();
    }
    else
    {
        // In live mode, reset timeout
        Serial.println("Live mode timeout reset");
    }
}

void ModeManager::enterSleepMode()
{
    Serial.println("Entering SLEEP mode");
    currentMode = DisplayMode::SLEEP;
    sleepRequested = false;

// Configure wake-up source
#if defined(CONFIG_IDF_TARGET_ESP32S3)
    esp_sleep_enable_ext1_wakeup(_BV(BUTTON_1), ESP_EXT1_WAKEUP_ANY_LOW);
#elif defined(CONFIG_IDF_TARGET_ESP32)
    esp_sleep_enable_ext1_wakeup(_BV(BUTTON_3), ESP_EXT1_WAKEUP_ANY_LOW);
#endif
}

void ModeManager::enterLiveMode()
{
    Serial.println("Entering LIVE mode");
    currentMode = DisplayMode::LIVE;
    lastInteraction = millis();
}
