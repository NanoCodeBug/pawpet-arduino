#pragma once
#include "../common.h"

#include <Adafruit_GFX.h>
#include <Adafruit_SPIDevice.h>
#include <Adafruit_ZeroDMA.h>
#include <utility/dma.h>

/**
 * Rewrite of Adafruit implementation of sharp memory lcd driver
 * adds DMA support
 * adds compressed image support
 * adds alpha channel support
 *
 * TODO:
 * - dippled greyscale support?
 */

#define SHARPMEM_BIT_WRITECMD (0x01) // 0x80 in LSB format
#define SHARPMEM_BIT_VCOM (0x02)     // 0x40 in LSB format
#define SHARPMEM_BIT_CLEAR (0x04)    // 0x20 in LSB format

class PetDisplay : public Adafruit_GFX
{
  public:
    PetDisplay(SPIClass *spi, uint8_t cs, uint16_t width, uint16_t height, uint32_t freq = 2000000);
    boolean begin();  

    // Overridden functions from Adafruit_GFX
    // special care to override users of memset
    // since the row/column data needed by the display is 
    // being kept in the graphics buffer
    void drawPixel(int16_t x, int16_t y, uint16_t color) override;
    void fillScreen(uint16_t color = PET_WHITE) override;

    void drawSubPixel(int16_t x, int16_t y, uint16_t color);
    uint8_t getPixel(uint16_t x, uint16_t y);

    // clears display builtin command
    void clearDisplay();

    // send data to display
    bool refresh();

  protected:
    uint8_t *sharpmem_buffer = NULL;

  private:
    static void dma_callback(Adafruit_ZeroDMA *dma);
    static volatile bool _dma_complete;

    Adafruit_SPIDevice *_spi = NULL;
    uint8_t _cs;
    uint8_t _sharpmem_vcom;

    Adafruit_ZeroDMA _dma;

    const uint8_t k_bytesPerLine = (WIDTH + 16) / 8;
    const uint32_t k_totalBytes = ((WIDTH + 16) * HEIGHT) / 8;

  public:
    void drawFrame(image_t *image, meta_t *meta, uint8_t dx, uint8_t dy, uint8_t frame, uint8_t off_color = PET_WHITE,
                   uint8_t on_color = PET_BLACK, uint8_t alpha_color = PET_CLEAR);

    // void drawFrame(const char *name,
    //                uint8_t dx, uint8_t dy,
    //                uint8_t frame,
    //                uint8_t off_color = PET_WHITE,
    //                uint8_t on_color = PET_BLACK,
    //                uint8_t alpha_color = PET_CLEAR);

    void drawImage(image_t *image, meta_t *meta, uint8_t dx, uint8_t dy, uint8_t off_color = PET_WHITE,
                   uint8_t on_color = PET_BLACK, uint8_t alpha_color = PET_CLEAR);

    // void drawImage(const char *name,
    //                uint8_t dx, uint8_t dy,
    //                uint8_t off_color = PET_WHITE,
    //                uint8_t on_color = PET_BLACK,
    //                uint8_t alpha_color = PET_CLEAR);

    void drawSpanMap(image_t *bitmap, const ImageMeta &meta, uint8_t dx, uint8_t dy, uint8_t off_color = PET_WHITE,
                     uint8_t on_color = PET_BLACK, uint8_t alpha_color = PET_CLEAR);

    void drawBitmap(image_t *bitmap, const ImageMeta &meta, uint8_t dx, uint8_t dy, uint8_t off_color = PET_WHITE,
                    uint8_t on_color = PET_BLACK, uint8_t alpha_color = PET_CLEAR);

    inline void setPixel(uint8_t x, uint8_t y, uint8_t c);
    inline void setPixel8(uint8_t x, int8_t y, uint8_t data);

    void fillDisplayBuffer(uint8_t color = PET_WHITE);

    void sync()
    {
        while (!_dma_complete) {}
    }

    bool isFrameLocked()
    {
        return !_dma_complete;
    }
};