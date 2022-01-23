
#include "display.h"

#include <wiring_private.h>

volatile bool PetDisplay::_dma_complete = true;

#ifndef _swap_int16_t
#define _swap_int16_t(a, b)                                                                                            \
    {                                                                                                                  \
        int16_t t = a;                                                                                                 \
        a = b;                                                                                                         \
        b = t;                                                                                                         \
    }
#endif

#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b)                                                                                           \
    {                                                                                                                  \
        uint16_t t = a;                                                                                                \
        a = b;                                                                                                         \
        b = t;                                                                                                         \
    }
#endif

// #define TOGGLE_VCOM                                                                                                    \
//     do                                                                                                                 \
//     {                                                                                                                  \
//         _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM;                                                    \
//         _drawBuffer[-1] = _sharpmem_vcom | SHARPMEM_BIT_WRITECMD;                                                      \
//     } while (0);

// #define TOGGLE_VCOM                                                                                                    \
//     do                                                                                                                 \
//     {                                                                                                                  \
//         _vcom_manual = !_vcom_manual;                                                                                  \
//         if (_vcom_manual)                                                                                              \
//         {                                                                                                              \
//             digitalWrite(DISP_COMIN, LOW);                                                                             \
//         }                                                                                                              \
//         else                                                                                                           \
//         {                                                                                                              \
//             digitalWrite(DISP_COMIN, HIGH);                                                                            \
//         }                                                                                                              \
//                                                                                                                        \
//     } while (0);

#define TOGGLE_VCOM                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
    } while (0);

/**
 * @brief Construct a new PetDisplay object with hardware SPI
 *
 * @param spi Pointer to hardware SPI device you want to use
 * @param cs The display chip select pin - **NOTE** this is ACTIVE HIGH!
 * @param width The display width
 * @param height The display height
 * @param freq The SPI clock frequency desired
 */
PetDisplay::PetDisplay(SPIClass *spi, uint8_t cs, uint16_t width, uint16_t height, uint32_t freq)
    : Adafruit_GFX(width, height), _cs(cs)
{
    _spi = new Adafruit_SPIDevice(cs, freq, SPI_BITORDER_LSBFIRST, SPI_MODE0, spi);
}

/**
 * @brief Start the driver object, setting up pins and configuring a buffer for
 * the screen contents
 *
 * @return boolean true: success false: failure
 */
boolean PetDisplay::begin(void)
{
    _vcom_manual = 0;

    if (!_spi->begin())
    {
        return false;
    }

    // Set the vcom bit to a defined state
    _sharpmem_vcom = SHARPMEM_BIT_VCOM;

    // 2 306 bytes for 128x128 display with command bytes and indexes
    // 7.2% of memory
    // 2 000 000 spi freq
    // 250 000 bytes per second
    // 9.224 ms to send one buffer
    // max possible throughput of 108 fps
    // (leaves 9 ms for game update and logic)
    // (not sure actual lcd can refresh that quick)

    _buffer1 = (uint8_t *)malloc((WIDTH + 16) * HEIGHT / 8 + 2);

    // double buffer?
    _buffer2 = (uint8_t *)malloc((WIDTH + 16) * HEIGHT / 8 + 2);

    if (!_buffer1 || !_buffer2)
        return false;

    _buffer1 += 1;
    _buffer2 += 1;

    initBuffer(_buffer1);
    initBuffer(_buffer2);

    _drawBuffer = _buffer1;
    _sendBuffer = _buffer2;

    // SETUP DMA
    _dma.setTrigger(SERCOM4_DMAC_ID_TX);
    _dma.setAction(DMA_TRIGGER_ACTON_BEAT);

    ZeroDMAstatus stat = _dma.allocate();
    if (stat != DMA_STATUS_OK)
    {
        return false;
    }

    _dma.addDescriptor(_sendBuffer - 1,                  // move data from here
                       (void *)(&SERCOM4->SPI.DATA.reg), // to here (M0)
                       ((WIDTH + 16) * HEIGHT / 8) + 2,  // this many...
                       DMA_BEAT_SIZE_BYTE,               // bytes/hword/words
                       true,                             // increment source addr?
                       false);                           // increment dest addr?

    _dma.setCallback(PetDisplay::dma_callback);

    // divider, linear or 2^(.DIV+1) 0-127, 2^18 = 30hz for 8mhz oscilator
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(4) | GCLK_GENDIV_DIV(17);

    // setup Clock Generator
    // GCLK_GENCTRL_Type genctrl = {};
    // genctrl.bit.RUNSTDBY = 1; // Run in Standby
    // genctrl.bit.DIVSEL = 1;   // .DIV (above) Selection: 0=linear 1=powers of 2
    // genctrl.bit.OE = 1;  // Output Enable to observe on a port pin
    // genctrl.bit.OOV = 1; // Output Off Value
    // genctrl.bit.IDC = 1; // Improve Duty Cycle
    // genctrl.bit.GENEN = 1;    // enable this GCLK
    // genctrl.bit.SRC = GCLK_SOURCE_OSC8M;
    // genctrl.bit.ID = (uint8_t)4; // GCLK_GENERATOR_X

    // GCLK->GENCTRL.reg = genctrl.reg;

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(4) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSC8M | GCLK_GENCTRL_DIVSEL |
                        GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_OE; // GCLK_GENCTRL_IDC

    while (GCLK->STATUS.bit.SYNCBUSY) {};

    // GCLK->CLKCTRL.reg =
    //   GCLK_CLKCTRL_ID_ | GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK4;

    // PORT->Group[0].PMUX[10 / 2].reg |= (uint8_t)PORT_PMUX_PMUXE(MUX_PA10H_GCLK_IO4); // enable port function H (glck)
    // PORT->Group[0].PINCFG[10].reg = (uint8_t)(PORT_PINCFG_PMUXEN | PORT_PINCFG_DRVSTR);

    pinPeripheral(DISP_COMIN, PIO_AC_CLK);

    return true;
}

// 1 << n is a costly operation on AVR -- table usu. smaller & faster
static const uint8_t PROGMEM set[] = {1, 2, 4, 8, 16, 32, 64, 128},
                             clr[] = {(uint8_t)~1,  (uint8_t)~2,  (uint8_t)~4,  (uint8_t)~8,
                                      (uint8_t)~16, (uint8_t)~32, (uint8_t)~64, (uint8_t)~128};

static const uint8_t PROGMEM set1[] = {0x03, 0x0C, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00},
                             clr1[] = {0xFC, 0xF3, 0xCF, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF},
                             set2[] = {0x00, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x30, 0xC0},
                             clr2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF3, 0xCF, 0x3F};

void PetDisplay::drawPixel(int16_t x, int16_t y, uint16_t color)
{
    // TODO can be optimized with memset
    drawSubPixel(x * 2, y * 2, color);
    drawSubPixel(x * 2 + 1, y * 2, color);
    drawSubPixel(x * 2, y * 2 + 1, color);
    drawSubPixel(x * 2 + 1, y * 2 + 1, color);
}

void PetDisplay::drawSubPixel(int16_t x, int16_t y, uint16_t color)
{
    // shift right one location to not write address space holder

    if ((x < 0) || (x >= _width) || (y < 0) || (y >= _height))
        return;

    switch (rotation)
    {
    case 1:
        _swap_int16_t(x, y);
        x = (WIDTH + 16) - 1 - x;
        x -= 8;
        break;
    case 2:
        x += 8;
        x = (WIDTH + 16) - 1 - x;
        y = HEIGHT - 1 - y;
        x += 8;
        break;
    case 3:
        _swap_int16_t(x, y);
        y = HEIGHT - 1 - y;
        y += 8;
        break;
    default:
        x += 8;
        break;
    }

    if (color)
    {
        _drawBuffer[(y * (WIDTH + 16) + x) / 8] &= pgm_read_byte(&clr[x & 7]);
    }
    else
    {
        _drawBuffer[(y * (WIDTH + 16) + x) / 8] |= pgm_read_byte(&set[x & 7]);
    }
}

/**************************************************************************/
/*!
    @brief Gets the value (1 or 0) of the specified pixel from the buffer

    @param[in]  x
                The x position (0 based)
    @param[in]  y
                The y position (0 based)

    @return     1 if the pixel is enabled, 0 if disabled
*/
/**************************************************************************/
uint8_t PetDisplay::getPixel(uint16_t x, uint16_t y)
{
    if ((x >= _width) || (y >= _height))
        return 0; // <0 test not needed, unsigned

    switch (rotation)
    {
    case 1:
        _swap_uint16_t(x, y);
        x = (WIDTH + 16) - 1 - x;
        break;
    case 2:
        x = (WIDTH + 16) - 1 - x;
        y = HEIGHT - 1 - y;
        break;
    case 3:
        _swap_uint16_t(x, y);
        y = HEIGHT - 1 - y;
        break;
    }

    return _drawBuffer[(y * (WIDTH + 16) + x) / 8] & pgm_read_byte(&set1[x & 7]) ? 1 : 0;
}

/**************************************************************************/
/*!
    @brief Clears the screen
*/
/**************************************************************************/
void PetDisplay::clearDisplay()
{
    // Send the clear screen command rather than doing a HW refresh (quicker)
    _spi->beginTransaction();
    digitalWrite(_cs, HIGH);

    uint8_t clear_data[2] = {_sharpmem_vcom | SHARPMEM_BIT_CLEAR, 0x00};
    _spi->transfer(clear_data, 2);

    TOGGLE_VCOM;
    digitalWrite(_cs, LOW);
    _spi->endTransaction();

    // empty display buffer
    fillDisplayBuffer();
}

void PetDisplay::dma_callback(Adafruit_ZeroDMA *dma)
{
    digitalWrite(8, LOW);
    // _spi->endTransaction();
    SPI.endTransaction();
    _dma_complete = true;
}

/**************************************************************************/
/*!
    @brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
bool PetDisplay::refresh(void)
{
    if (!_dma_complete)
    {
        return false;
    }
    _dma_complete = false;

    // SPI_BITORDER_LSBFIRST, SPI_MODE0
    _spi->beginTransaction();
    digitalWrite(_cs, HIGH);

    /**
     * TODO, remove? flips polarity, not clear if this is ever needed
     * sharp recommends toggling every second
     * other sources use every 5 seconds
     * sharp sample code says 1-30 second period
     * experiments with 60 second sleep and wakeup have had no issue so far across all prototypes
     * add timer interrupt or have millis tracked here to to avoid unncessary blinking on display refresh due to
     * toggling this at lower power break out extcomm pin on display and map to pin on samd21, and setup a clock that
     * runs in suspend?
     */

    TOGGLE_VCOM;

    // uint8_t * buf = _sendBuffer;
    // _sendBuffer = _drawBuffer;
    // _drawBuffer = buf;

    // time spent waiting for dma to finish while single bufferring:
    // update, wait (9ms - update), then draw, send

    // time spent waiting while double buffering
    // update, draw, swap (1ms), send
    // at the cost of 2kb of ram ~ 7%

    // TODO find way to swap pointers instead?
    // unless cost of having two dma objects or re-initializing dma is higher than a 1ms
    memcpy(_sendBuffer - 1, _drawBuffer - 1, (WIDTH + 16) * HEIGHT / 8 + 2);

    // start dma job
    ZeroDMAstatus stat = _dma.startJob();
    if (stat != DMA_STATUS_OK)
    {
        return false;
    }

    return true;
}

void PetDisplay::drawFrame(image_t &pm, uint8_t dx, uint8_t dy, uint8_t frame, uint8_t off_color, uint8_t on_color,
                           uint8_t alpha_color)
{
    if (frame >= pm.meta->tileCount)
    {
        return;
    }

    uint32_t *data = pm.data + pm.tileOffsets[frame];

    if (pm.meta->encoding)
    {
        // supposed to contain array of locations of start of each span encoded frame
        if (pm.meta->alpha)
        {
            drawSpanMapA(data, pm.meta->width, pm.meta->height, dx, dy, off_color, on_color, alpha_color);
        }
        else
        {
            drawSpanMap(data, pm.meta->width, pm.meta->height, dx, dy, off_color, on_color);
        }
    }

    // move bitmap to point to start of image
    // frames are not packed into eachother
    // for tiles 4 bytes per uint32_t, 16 pixels alpha, 32 pixels b/w
    else
    {
        if (pm.meta->alpha)
        {
            // pm.data += (pm.meta->width * pm.meta->height) / 16 * frame;
            drawBitmapA(data, pm.meta->width, pm.meta->height, dx, dy, off_color, on_color, alpha_color);
        }
        else
        {
            // pm.data += (pm.meta->width * pm.meta->height) / 32 * frame;
            drawBitmap(data, pm.meta->width, pm.meta->height, dx, dy, off_color, on_color);
        }
    }
}

void PetDisplay::drawSpanMap(uint32_t *bitmap, const uint8_t width, const uint8_t height, uint8_t dx, uint8_t dy,
                             uint8_t off_color, uint8_t on_color)
{
    const uint32_t bitmapLength = width * height;
    uint32_t pixelsRead = 0;
    uint8_t currByte = 0;
    uint8_t currPack = 0;

    while (pixelsRead < bitmapLength)
    {
        uint8_t pixelData = bitmap[currPack] >> currByte * 8;

        uint8_t length = 0;
        uint8_t color = 0;

        length = pixelData & 0x7F;
        color = (pixelData >> 7) & 0x1;

        for (uint8_t p = 0; p < length; p++)
        {
            uint8_t i = pixelsRead % width;
            uint8_t j = pixelsRead / width;

            if (color == 1 && on_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, on_color);
            }
            else if (color == 0 && off_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, off_color);
            }
            pixelsRead++;
        }

        currByte++;
        if (currByte > 3)
        {
            currByte = 0;
            currPack++;
        }
    }
}
void PetDisplay::drawSpanMapA(uint32_t *bitmap, const uint8_t width, const uint8_t height, uint8_t dx, uint8_t dy,
                              uint8_t off_color, uint8_t on_color, uint8_t alpha_color)
{
    const uint32_t bitmapLength = width * height;
    uint32_t pixelsRead = 0;
    uint8_t currByte = 0;
    uint8_t currPack = 0;

    while (pixelsRead < bitmapLength)
    {
        uint8_t pixelData = bitmap[currPack] >> currByte * 8;

        uint8_t length = 0;
        uint8_t color = 0;

        // alpha channel spans one less bit
        length = pixelData & 0x3F;
        color = (pixelData >> 6) & 0x3;

        for (uint8_t p = 0; p < length; p++)
        {
            uint8_t i = pixelsRead % width;
            uint8_t j = pixelsRead / width;

            if (color == 1 && on_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, on_color);
            }
            else if (color == 2 && alpha_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, alpha_color);
            }
            else if (color == 0 && off_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, off_color);
            }
            pixelsRead++;
        }

        currByte++;
        if (currByte > 3)
        {
            currByte = 0;
            currPack++;
        }
    }
}

void PetDisplay::drawBitmap(uint32_t *bitmap, const uint8_t width, const uint8_t height, uint8_t dx, uint8_t dy,
                            uint8_t off_color, uint8_t on_color)
{
    // 32 pixels per uint32_t
    uint32_t pack = 0;
    uint8_t packIndex = 0;
    uint8_t bitmapPack = 0;
    // seq array access for performance
    for (uint8_t j = 0; j < height; j++)
    {
        for (uint8_t i = 0; i < width; i++)
        {
            // shift saved byte to next bit
            if (packIndex > 0)
            {
                pack >>= 1;
                packIndex--;
            }
            // read next byte
            else
            {
                packIndex = 31;
                pack = bitmap[bitmapPack];
                bitmapPack++;
            }

            if (pack & 0x1 && on_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, on_color);
            }
            else if (off_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, off_color);
            }
        }
    }
}

void PetDisplay::drawBitmapA(uint32_t *bitmap, const uint8_t width, const uint8_t height, uint8_t dx, uint8_t dy,
                             uint8_t off_color, uint8_t on_color, uint8_t alpha_color)
{
    // 16 pixels per uint32_t
    uint32_t pack = 0;
    uint8_t packIndex = 0;
    uint8_t bitmapPack = 0;
    // seq array access for performance
    for (uint8_t j = 0; j < height; j++)
    {
        for (uint8_t i = 0; i < width; i++)
        {
            // shift saved byte to next bit
            if (packIndex > 0)
            {
                pack >>= 2;
                packIndex--;
            }
            // read next byte
            else
            {
                packIndex = 15;
                pack = bitmap[bitmapPack];
                bitmapPack++;
            }

            uint8_t pixel = pack & 0x3;

            if (pixel == 1 && on_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, on_color);
            }
            else if (pixel == 2 && alpha_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, alpha_color);
            }
            else if (pixel == 0 && off_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, off_color);
            }
        }
    }
}

void PetDisplay::fillScreen(uint16_t color)
{
    fillDisplayBuffer(color);
}

void PetDisplay::fillDisplayBuffer(uint8_t color) // PET_WHITE
{
    for (uint32_t i = 0; i < k_totalBytes; i += k_bytesPerLine)
    {
        // clear data between first and last byte of line
        memset(_drawBuffer + i + 1, color ? 0x00 : 0xFF, k_bytesPerLine - 2);

        // reset address and end of line data
        // TODO not needed as long as no other memset resets this info
        // sharpmem_buffer[i] = ((i + 1) / ((WIDTH + 16) / 8)) + 1;
        // sharpmem_buffer[i + bytes_per_line - 1] = 0x00;
    }
}

void PetDisplay::initBuffer(uint8_t *buffer)
{
    buffer[-1] = _sharpmem_vcom | SHARPMEM_BIT_WRITECMD;
    buffer[(WIDTH + 16) * HEIGHT / 8 + 1] = 0x00;

    // zero memory buffer and setup display start/end of line info
    for (uint32_t i = 0; i < k_totalBytes; i += k_bytesPerLine)
    {
        // clear data between first and last byte of line
        memset(buffer + i + 1, 0xFF, k_bytesPerLine - 2);

        // reset address and end of line data
        buffer[i] = ((i + 1) / ((WIDTH + 16) / 8)) + 1;
        buffer[i + k_bytesPerLine - 1] = 0x00;
    }
}