#include "global.h"
#include <FatLib/FatFileSystem.h>
#include "graphics/graphic_manager.h"

namespace g
{
 FatFileSystem g_fatfs;
 GraphicCache* g_cache;
 uint32_t keyReleased;
 uint32_t keyPressed;
}