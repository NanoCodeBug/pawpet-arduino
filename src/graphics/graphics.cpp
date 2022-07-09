#include "graphics.h"

#ifdef SIMULATOR
#include <cstdio>
#include <cstdlib>
#else
#include <FatLib/ArduinoFiles.h>
#include <FatLib/FatFileSystem.h>
#endif

bool PetSprite::LoadFromFlash(const char *name, image_t *image)
{
#ifndef SIMULATOR
    char *fname = new char[strlen(name) + 5];
    strcpy(fname, name);
    strcat(fname, ".paw");
    File dataFile = g::g_fatfs->open(fname, FILE_READ);

    image->meta = new meta_t;
    meta_t *meta = image->meta;

    if (!dataFile)
    {
        delete fname;
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
#else

    char *fname = new char[strlen(name) + 5];
    strcpy(fname, name);
    strcat(fname, ".paw");
    FILE* dataFile = std::fopen(fname, "r");

    image->meta = new meta_t;
    meta_t *meta = image->meta;
    if (!dataFile)
    {
        delete [] fname;
        return false;
    }

    meta->height = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));
    meta->width = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));

    meta->encoding = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));
    meta->alpha = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));

    std::fgetc(dataFile);
    std::fgetc(dataFile);
    meta->tileCount = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));

    image->tileOffsets = new uint16_t[meta->tileCount];
    for (int s = 0; s < meta->tileCount; s += 2)
    {
        if (s + 1 < meta->tileCount)
        {
            image->tileOffsets[s + 1] = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));
        }
        else
        {
            std::fgetc(dataFile);
            std::fgetc(dataFile);
        }

        image->tileOffsets[s] = (std::fgetc(dataFile) << 8 | std::fgetc(dataFile));
    }
    std::size_t curr = std::ftell(dataFile);
    std::fseek(dataFile, 0, SEEK_END);
    std::size_t filesize = std::ftell(dataFile);
    std::fseek(dataFile, curr, SEEK_SET);

    uint32_t totalImageSize = filesize / 4;
    image->data = new uint32_t[totalImageSize];

    uint32_t index = 0;
    while (index < totalImageSize)
    {
        uint8_t c1, c2, c3, c4;
        c1 = std::fgetc(dataFile);
        c2 = std::fgetc(dataFile);
        c3 = std::fgetc(dataFile);
        c4 = std::fgetc(dataFile);

        image->data[index++] = (c1 << 24 | c2 << 16 | c3 << 8 | c4);
    }
 
    std::fclose(dataFile);

#endif
    delete [] fname;
    return true;
}