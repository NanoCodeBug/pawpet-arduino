#include "graphic_manager.h"
#include <FatLib/ArduinoFiles.h>
#include <FatLib/FatFileSystem.h>

CacheEntry GraphicCache::gcache[GRAPHIC_CACHE_COUNT];
uint16_t GraphicCache::gindex;

GraphicCache::GraphicCache()
{
    gindex = 0;
}

int32_t GraphicCache::LoadGraphic(const char *name)
{
    char *fname = new char[strlen(name) + 5];
    strcpy(fname, name);
    strcat(fname, ".paw");
    File dataFile = g::g_fatfs.open(fname, FILE_READ);

    if (dataFile)
    {
        CacheEntry &c = gcache[gindex];
        dataFile.seek(8);
        c.meta.width = (dataFile.read() << 8 | dataFile.read());
        c.meta.height = (dataFile.read() << 8 | dataFile.read());
        c.meta.alpha = (dataFile.read() << 8 | dataFile.read());
        c.meta.encoding = (dataFile.read() << 8 | dataFile.read());
        c.meta.tileCount = (dataFile.read() << 8 | dataFile.read());
        c.meta.tileOffset = (dataFile.read() << 8 | dataFile.read());
        dataFile.seek(26);
        // FIXME: tileoffset is an array of size tileCount

        uint32_t totalImageSize = dataFile.available() / 4;
        c.data = new uint32_t[totalImageSize];
        c.data_size = totalImageSize;
        uint32_t index = 0;
        while (dataFile.available())
        {
            char c1, c2, c3, c4;
            c1 = dataFile.read();
            c2 = dataFile.read();
            c3 = dataFile.read();
            c4 = dataFile.read();

            uint32_t imData = (c1 << 24 | c2 << 16 | c3 << 8 | c4);
            c.data[index++] = imData;
        }

        strncpy(c.name, name, (strlen(name) > 15 ? 15 : strlen(name)));

        gindex++;
        if (gindex > GRAPHIC_CACHE_COUNT)
        {
            gindex = 0;
        }
    }

    return 0;
}

bool GraphicCache::GetGraphic(const char *name, uint16_t **meta, uint32_t **data)
{
    for (size_t s = 0; s < GRAPHIC_CACHE_COUNT; s++)
    {
        if (strcmp(gcache[s].name, name) == 0)
        {
            *meta = (uint16_t *)(&gcache[s].meta);
            *data = gcache[s].data;
            return true;
        }
    }
    return false;
}