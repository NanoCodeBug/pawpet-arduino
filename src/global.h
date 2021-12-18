#ifndef global_h
#define global_h
#pragma once

constexpr uint32_t k_sleepTimeMs = 1 * 60 * 1000; // 1 minute sleep
constexpr uint32_t k_frameSleepTimeMs = 33;               // 33, 30 fps // 50 ms for 20 fps
constexpr uint32_t k_tickTime = 3; //k_frameSleepTimeMs / 10;

namespace g
{
extern FatFileSystem g_fatfs;
extern GraphicCache* g_cache;
}

int32_t msc_read_cb (uint32_t lba, void* buffer, uint32_t bufsize);
int32_t msc_write_cb (uint32_t lba, uint8_t* buffer, uint32_t bufsize, uint32_t tag);
void msc_flush_cb (void);

bool msc_ready_cb();

#endif