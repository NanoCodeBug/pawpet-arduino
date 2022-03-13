#include "global.h"

#include "graphics/graphics.h"
#include <FatLib/FatFileSystem.h>

namespace g
{
FatFileSystem *g_fatfs;
Adafruit_SPIFlash* g_flash;

GraphicCache *g_cache;
uint32_t g_keyReleased;
uint32_t g_keyPressed;
uint32_t g_keyHeld;
stats g_stats;
RTCZero g_rtc;
} // namespace g