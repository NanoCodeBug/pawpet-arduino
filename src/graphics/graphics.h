#pragma once
#include "../common.h"
#include "../global.h"

#include "display.h"
/**
 * Maintain a graphic cache of N graphic objects totalling X bytes
 * with expire between state loading (unless next state then reloads it)
 * ideally throws exception if graphic memory is exhausted
 * allows gamestates to ignore managing graphic lifetime if desired
 * allows for easy tracking of total gamestate graphic usage
 */

class PetSprite
{
  public:
    image_t _image;
    bool _progmem;

    PetSprite(const uint32_t *data) : _progmem(true)
    {
        _image.meta = (meta_t *)data;
        _image.tileOffsets = (uint16_t *)(data + (sizeof(meta_t) / 4));

        // round up to next even number
        uint16_t endOfOffsets = _image.meta->tileCount + (_image.meta->tileCount % 2);

        _image.data = (uint32_t *)(_image.tileOffsets + endOfOffsets);
    }

    PetSprite(const char *name) : _progmem(false)
    {
        PetSprite::LoadFromFlash(name, &_image);
    }

    ~PetSprite()
    {
        if (_progmem)
            return;

        delete _image.meta;
        delete [] _image.tileOffsets;
        delete [] _image.data;
    }

    inline void draw(PetDisplay *disp, uint8_t dx, uint8_t dy, uint8_t frame = 0)
    {
        disp->drawFrame(_image, dx, dy, frame);
    }

    static bool LoadFromFlash(const char *name, image_t *image);
};

class PetAnimation
{
  public:
    uint8_t animationDir = 0; // forward, backward
    uint8_t loopType = 1;     // none, loop, ping-pong
    uint8_t ticksPerFrame = 30;

  public:
    PetAnimation(const char *name) : _spriteMap(name){};
    PetAnimation(const uint32_t *image) : _spriteMap(image){};

    void draw(PetDisplay *disp, uint8_t dx, uint8_t dy)
    {
        _spriteMap.draw(disp, dx, dy, _frame);
    }

    bool update(uint32_t tick)
    {
        _currTick += tick;
        if (_currTick <= ticksPerFrame)
        {
            return false;
        }
        _currTick = _currTick % ticksPerFrame;

        _frame += animationDir ? -1 : 1;
        if (_frame >= _spriteMap._image.meta->tileCount)
        {
            // go to first frame
            if (loopType == 1)
            {
                _frame = 0;
            }
            // change frame direction
            else if (loopType == 2)
            {
                _frame--;
                animationDir = 1;
            }
        }
        else if (_frame < 0)
        {
            // go to last frame frame
            if (loopType == 1)
            {
                _frame = _spriteMap._image.meta->tileCount;
            }
            // change frame direction
            else if (loopType == 2)
            {
                _frame = 1;
                animationDir = 0;
            }
        }
        return true;
    }

  private:
    PetSprite _spriteMap;
    uint8_t _currTick = 0;
    int16_t _frame = 0;
};