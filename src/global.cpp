#include "global.h"

#include "graphics/graphics.h"
#include <FatLib/FatFileSystem.h>

namespace g
{
FatFileSystem *g_fatfs;
GraphicCache *g_cache;
uint32_t g_keyReleased;
uint32_t g_keyPressed;
stats g_stats;
RTCZero g_rtc;
} // namespace g