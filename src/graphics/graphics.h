#pragma once
#include "../common.h"
#include "../global.h"

class PetDisplay;
/**
 * Maintain a graphic cache of N graphic objects totalling X bytes
 * with expire between state loading (unless next state then reloads it)
 * ideally throws exception if graphic memory is exhausted
 * allows gamestates to ignore managing graphic lifetime if desired
 * allows for easy tracking of total gamestate graphic usage
 */
#define GRAPHIC_CACHE_SIZE_BYTES 10000
#define GRAPHIC_CACHE_COUNT 16

class GraphicCache
{

  public:
    GraphicCache();

    bool LoadGraphic(const char *name, PetImage** image);
};

class Animation
{
  public:
    Animation();

    void draw(PetDisplay *disp);
    void update(uint32_t tick);

  private:
    meta_t *meta;
    image_t *image;
};

class Image
{
  public:
};