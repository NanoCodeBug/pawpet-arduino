#include "global.h"
#include "graphics/graphic_manager.h"
#include <FatLib/FatFileSystem.h>

namespace g
{
FatFileSystem g_fatfs;
GraphicCache *g_cache;
uint32_t keyReleased;
uint32_t keyPressed;
} // namespace g