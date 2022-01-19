#include "graphics.h"
#include <FatLib/ArduinoFiles.h>
#include <FatLib/FatFileSystem.h>

bool PetSprite::LoadFromFlash(const char *name, image_t *image)
{
    char *fname = new char[strlen(name) + 5];
    strcpy(fname, name);
    strcat(fname, ".paw");
    File dataFile = g::g_fatfs->open(fname, FILE_READ);

    image->meta = new meta_t;
    meta_t *meta = image->meta;

    if (!dataFile)
    {
        return false;
    }

    meta->height = (dataFile.read() << 8 | dataFile.read());
    meta->width = (dataFile.read() << 8 | dataFile.read());

    meta->encoding = (dataFile.read() << 8 | dataFile.read());
    meta->alpha = (dataFile.read() << 8 | dataFile.read());

    dataFile.read();
    dataFile.read();
    meta->tileCount = (dataFile.read() << 8 | dataFile.read());

    image->tileOffsets = new uint16_t[meta->tileCount];
    for (int s = 0; s < meta->tileCount; s += 2)
    {
        if (s + 1 < meta->tileCount)
        {
            image->tileOffsets[s + 1] = (dataFile.read() << 8 | dataFile.read());
        }
        else
        {
            dataFile.read();
            dataFile.read();
        }

        image->tileOffsets[s] = (dataFile.read() << 8 | dataFile.read());
    }

    uint32_t totalImageSize = dataFile.available() / 4;
    image->data = new uint32_t[totalImageSize];

    uint32_t index = 0;
    while (dataFile.available())
    {
        char c1, c2, c3, c4;
        c1 = dataFile.read();
        c2 = dataFile.read();
        c3 = dataFile.read();
        c4 = dataFile.read();

        image->data[index++] = (c1 << 24 | c2 << 16 | c3 << 8 | c4);
    }
    dataFile.close();

    return true;
}