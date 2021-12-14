#pragma once

/**
 * Maintain a graphic cache of N graphic objects totalling X bytes
 * with expire between state loading (unless next state then reloads it)
 * ideally throws exception if graphic memory is exhausted
 * allows gamestates to ignore managing graphic lifetime if desired
 * allows for easy tracking of total gamestate graphic usage
 */
#define GRAPHIC_CACHE_SIZE_BYTES 10000
#define GRAPHIC_CACHE_COUNT 16

struct CacheEntry
{
    char name[16];
    uint32_t* data;
    uint32_t data_size;
    ImageMeta meta;
};

class GraphicCache
{
    static CacheEntry gcache[GRAPHIC_CACHE_COUNT];
    static uint16_t gindex;

public:
    GraphicCache();
    
	int32_t LoadGraphic(const char* name);

    bool GetGraphic(const char* name, uint16_t** meta, uint32_t** data);
};