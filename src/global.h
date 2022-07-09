#pragma once
#ifndef SIMULATOR
#include "lib/RTCZero.h"
#include <Arduino.h>
#include <Adafruit_SPIFlash.h>
#else
#include <stdint.h>
#endif

/**
 * Global objects and constants
 *
 */

constexpr uint32_t k_sleepTimeMs = 1 * 60 * 1000; // 1 minute sleep

constexpr uint32_t k_30_fpsSleepMs = 33;  // 30 fps
constexpr uint32_t k_15_fpsSleepMs = 66;  // 15 fps
constexpr uint32_t k_5_fpsSleepMs = 200;  // 5 fps
constexpr uint32_t k_1_fpsSleepMs = 1000; // 1 fps

// ticks per update, arbitrary tick rate that should scale across fps
// leave overhead for 60 fps mode if possible

constexpr uint32_t k_tickTime30 = 2; // k_frameSleepTimeMs / 16; 30 fps ticks
constexpr uint32_t k_tickTime15 = 4; // k_frameSleepTimeMs / 16; 15 fps ticks
constexpr uint32_t k_tickTime5 = 13; // k_frameSleepTimeMs / 16; 5 fps ticks
constexpr uint32_t k_tickTime1 = 63; // k_frameSleepTimeMs / 16; 1 fps ticks

class FatFileSystem;
class GraphicCache;

struct stats
{
    uint32_t flashSize;
    uint32_t freeSpace;
    bool filesysFound;
    uint32_t bootTime;
};

namespace g
{
extern FatFileSystem *g_fatfs;
extern GraphicCache *g_cache;
extern stats g_stats;

#ifndef SIMULATOR
extern Adafruit_SPIFlash *g_flash;
extern RTCZero g_rtc;
#endif

extern uint32_t g_keyReleased;
extern uint32_t g_keyPressed;
extern uint32_t g_keyHeld;
} // namespace g
