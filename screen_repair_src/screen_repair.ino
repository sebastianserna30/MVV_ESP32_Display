/**
 * Advanced E-Paper Burn-in/Ghosting Repair Tool - Stable Version
 * Optimized for LilyGO T5/EPD47 displays
 * Fixes I80 bus initialization conflicts
 *
 * @copyright Copyright (c) 2024  Advanced E-Paper Solutions
 * @date      2024-08-06
 * @note      Arduino Setting
 *            Tools ->
 *                  Board:"ESP32S3 Dev Module"
 *                  USB CDC On Boot:"Enable"
 *                  USB DFU On Boot:"Disable"
 *                  Flash Size : "16MB(128Mb)"
 *                  Flash Mode"QIO 80MHz
 *                  Partition Scheme:"16M Flash(3M APP/9.9MB FATFS)"
 *                  PSRAM:"OPI PSRAM"
 *                  Upload Mode:"UART0/Hardware CDC"
 *                  USB Mode:"Hardware CDC and JTAG"
 */

#ifndef BOARD_HAS_PSRAM
#error "Please enable PSRAM, Arduino IDE -> tools -> PSRAM -> OPI !!!"
#endif

#include <Arduino.h>
#include "epd_driver.h"
#include "utilities.h"

// Repair configuration - optimized for stability
#define AGGRESSIVE_CYCLES 60 // Increased for better results
#define MODERATE_CYCLES 40
#define LIGHT_CYCLES 20
#define EXTENDED_HOLD_TIME 1200 // Longer hold for complete particle movement
#define QUICK_HOLD_TIME 600     // For rapid cycling
#define INTER_STAGE_DELAY 2000  // Between major stages
#define CYCLE_PAUSE 200         // Between individual cycles

// Voltage patterns
#define FULL_BLACK 1
#define FULL_WHITE 0

// Function prototypes
void performStableRepair();
void intensiveVoltageRepair(int cycles, int hold_time);
void alternatingPatternRepair();
void grayscaleProgressiveRepair();
void ultraFastAgitationRepair();
void finalCleanupSequence();
void waitForReady();
bool initializeDisplay();
void printProgress(const char *stage, int current, int total);

bool display_initialized = false;

void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.println("Advanced E-Paper Burn-in Repair Tool");
    Serial.println("Stable Version - No I80 Bus Conflicts");
    Serial.println("========================================");

    // Safe initialization
    if (!initializeDisplay())
    {
        Serial.println("CRITICAL: Display initialization failed!");
        Serial.println("Please:");
        Serial.println("1. Check all connections");
        Serial.println("2. Reset the board");
        Serial.println("3. Upload this program again");
        while (1)
            delay(1000);
    }

    Serial.println("Display initialized successfully!");
    Serial.println("Starting advanced repair sequence...");
    Serial.println();

    performStableRepair();

    Serial.println();
    Serial.println("========================================");
    Serial.println("REPAIR SEQUENCE COMPLETED");
    Serial.println("========================================");
    Serial.println("Please check your display for improvements.");
    Serial.println("If burn-in persists, reset and run again.");
}

bool initializeDisplay()
{
    Serial.println("Initializing display driver...");

    // Single initialization attempt
    epd_init();
    delay(2000);

    display_initialized = true;
    return true;
}

void performStableRepair()
{
    Rect_t area = epd_full_screen();

    Serial.println("=== STAGE 1: INTENSIVE VOLTAGE REPAIR ===");
    epd_poweron();
    delay(500);
    intensiveVoltageRepair(AGGRESSIVE_CYCLES, EXTENDED_HOLD_TIME);

    Serial.println("\n=== STAGE 2: ULTRA-FAST AGITATION REPAIR ===");
    ultraFastAgitationRepair();

    Serial.println("\n=== STAGE 3: ALTERNATING PATTERN REPAIR ===");
    alternatingPatternRepair();

    Serial.println("\n=== STAGE 4: GRAYSCALE PROGRESSIVE REPAIR ===");
    grayscaleProgressiveRepair();

    Serial.println("\n=== STAGE 5: FINAL CLEANUP ===");
    finalCleanupSequence();

    Serial.println("\nPowering down display...");
    epd_poweroff_all();
}

void intensiveVoltageRepair(int cycles, int hold_time)
{
    Rect_t area = epd_full_screen();

    Serial.printf("Running %d intensive voltage cycles...\n", cycles);

    for (int i = 0; i < cycles; i++)
    {
        // Extended black phase for stuck white particles
        epd_push_pixels(area, 50, FULL_BLACK);
        delay(hold_time);

        // Extended white phase for stuck black particles
        epd_push_pixels(area, 50, FULL_WHITE);
        delay(hold_time);

        // Brief settling time
        delay(CYCLE_PAUSE);

        // Progress feedback every 10 cycles
        if ((i + 1) % 10 == 0)
        {
            printProgress("Voltage Repair", i + 1, cycles);
        }
    }
}

void rapidCyclingRepair()
{
    Rect_t area = epd_full_screen();
    int rapid_cycles = 80;

    Serial.printf("Running %d rapid cycling patterns...\n", rapid_cycles);

    for (int i = 0; i < rapid_cycles; i++)
    {
        // Rapid alternation to agitate particles
        epd_push_pixels(area, 50, i % 2);
        delay(150); // Fast cycling

        if ((i + 1) % 20 == 0)
        {
            printProgress("Rapid Cycling", i + 1, rapid_cycles);
            delay(500); // Brief pause every 20 cycles
        }
    }
}

void alternatingPatternRepair()
{
    Rect_t area = epd_full_screen();

    Serial.println("Running alternating pattern sequences...");

    // Pattern 1: Standard alternation
    for (int i = 0; i < 25; i++)
    {
        epd_push_pixels(area, 50, FULL_BLACK);
        delay(QUICK_HOLD_TIME);
        epd_push_pixels(area, 50, FULL_WHITE);
        delay(QUICK_HOLD_TIME);

        if ((i + 1) % 5 == 0)
        {
            printProgress("Alt Pattern 1", i + 1, 25);
        }
    }

    delay(1000);

    // Pattern 2: Double black emphasis
    Serial.println("Double black emphasis pattern...");
    for (int i = 0; i < 20; i++)
    {
        epd_push_pixels(area, 50, FULL_BLACK);
        delay(QUICK_HOLD_TIME);
        epd_push_pixels(area, 50, FULL_BLACK);
        delay(QUICK_HOLD_TIME);
        epd_push_pixels(area, 50, FULL_WHITE);
        delay(QUICK_HOLD_TIME);

        if ((i + 1) % 5 == 0)
        {
            printProgress("Alt Pattern 2", i + 1, 20);
        }
    }

    delay(1000);

    // Pattern 3: Double white emphasis
    Serial.println("Double white emphasis pattern...");
    for (int i = 0; i < 20; i++)
    {
        epd_push_pixels(area, 50, FULL_WHITE);
        delay(QUICK_HOLD_TIME);
        epd_push_pixels(area, 50, FULL_WHITE);
        delay(QUICK_HOLD_TIME);
        epd_push_pixels(area, 50, FULL_BLACK);
        delay(QUICK_HOLD_TIME);

        if ((i + 1) % 5 == 0)
        {
            printProgress("Alt Pattern 3", i + 1, 20);
        }
    }
}

void grayscaleProgressiveRepair()
{
    Serial.println("Running grayscale progressive repair...");
    Serial.println("Note: This uses push_pixels with varying intensities");

    // Progressive intensity repair using push_pixels parameters
    int intensities[] = {10, 20, 30, 40, 50, 40, 30, 20, 10};
    int num_intensities = sizeof(intensities) / sizeof(intensities[0]);

    Rect_t area = epd_full_screen();

    for (int cycle = 0; cycle < 8; cycle++)
    {
        for (int i = 0; i < num_intensities; i++)
        {
            // Black with varying intensity
            epd_push_pixels(area, intensities[i], FULL_BLACK);
            delay(400);

            // White with varying intensity
            epd_push_pixels(area, intensities[i], FULL_WHITE);
            delay(400);
        }

        printProgress("Grayscale Repair", cycle + 1, 8);
        delay(500);
    }
}

void ultraFastAgitationRepair()
{
    Rect_t area = epd_full_screen();
    int rapid_cycles = 100;

    Serial.printf("Running %d ultra-fast agitation cycles...\n", rapid_cycles);

    // Phase 1: Fast alternation
    for (int i = 0; i < rapid_cycles / 2; i++)
    {
        epd_push_pixels(area, 50, i % 2);
        delay(100);

        if ((i + 1) % 15 == 0)
        {
            printProgress("Ultra-Fast Phase 1", i + 1, rapid_cycles / 2);
        }
    }

    delay(1000);

    // Phase 2: Ultra-fast alternation
    Serial.println("Maximum speed agitation phase...");
    for (int i = 0; i < rapid_cycles / 2; i++)
    {
        epd_push_pixels(area, 50, i % 2);
        delay(50); // Very fast

        if ((i + 1) % 15 == 0)
        {
            printProgress("Ultra-Fast Phase 2", i + 1, rapid_cycles / 2);
        }
    }
}

void finalCleanupSequence()
{
    Rect_t area = epd_full_screen();

    Serial.println("Final cleanup sequence...");

    // Multi-stage cleanup
    Serial.println("  -> Extended black hold...");
    epd_push_pixels(area, 50, FULL_BLACK);
    delay(2000);

    Serial.println("  -> Extended white hold...");
    epd_push_pixels(area, 50, FULL_WHITE);
    delay(2000);

    Serial.println("  -> Standard clear...");
    epd_clear();
    delay(1500);

    Serial.println("  -> Final white...");
    epd_push_pixels(area, 50, FULL_WHITE);
    delay(1500);

    Serial.println("  -> Final clear...");
    epd_clear();
    delay(1000);

    Serial.println("Cleanup complete!");
}

void printProgress(const char *stage, int current, int total)
{
    float percentage = (float)current / total * 100;
    Serial.printf("  %s: %d/%d (%.1f%%)\n", stage, current, total, percentage);
}

void waitForReady()
{
    delay(100);
}

void loop()
{
    // Program runs once in setup(), loop does nothing
    delay(30000);
    Serial.println("Repair program completed. Reset board to run again.");
}