#include "global.h"

#include "graphics/graphics.h"
#ifndef SIMULATOR
#include <FatLib/FatFileSystem.h>
#endif

namespace g
{
#ifndef SIMULATOR
FatFileSystem *g_fatfs;
Adafruit_SPIFlash* g_flash;
RTCZero g_rtc;
#endif

GraphicCache *g_cache;

uint32_t g_keyReleased;
uint32_t g_keyPressed;
uint32_t g_keyHeld;

stats g_stats;
} // namespace g