
#include "common.h"
#include "pet_display.h"
#include "global.h"

volatile bool PetDisplay::transfer_is_done = true;

#ifndef _swap_int16_t
#define _swap_int16_t(a, b) \
  {                         \
    int16_t t = a;          \
    a = b;                  \
    b = t;                  \
  }
#endif
#ifndef _swap_uint16_t
#define _swap_uint16_t(a, b) \
  {                          \
    uint16_t t = a;          \
    a = b;                   \
    b = t;                   \
  }
#endif

#define TOGGLE_VCOM                                               \
  do                                                              \
  {                                                               \
    _sharpmem_vcom = _sharpmem_vcom ? 0x00 : SHARPMEM_BIT_VCOM;   \
    sharpmem_buffer[-1] = _sharpmem_vcom | SHARPMEM_BIT_WRITECMD; \
  } while (0);

/**
 * @brief Construct a new PetDisplay object with hardware SPI
 *
 * @param theSPI Pointer to hardware SPI device you want to use
 * @param cs The display chip select pin - **NOTE** this is ACTIVE HIGH!
 * @param width The display width
 * @param height The display height
 * @param freq The SPI clock frequency desired
 */
PetDisplay::PetDisplay(SPIClass *theSPI, uint8_t cs,
                       uint16_t width, uint16_t height,
                       uint32_t freq)
    : Adafruit_GFX(width, height)
{
  _cs = cs;
  if (spidev)
  {
    delete spidev;
  }
  spidev = new Adafruit_SPIDevice(cs, freq, SPI_BITORDER_LSBFIRST, SPI_MODE0,
                                  theSPI);
}

/**
 * @brief Start the driver object, setting up pins and configuring a buffer for
 * the screen contents
 *
 * @return boolean true: success false: failure
 */
boolean PetDisplay::begin(void)
{
  if (!spidev->begin())
  {
    return false;
  }
  // this display is weird in that _cs is active HIGH not LOW like every other
  // SPI device
  digitalWrite(_cs, LOW);

  // Set the vcom bit to a defined state
  _sharpmem_vcom = SHARPMEM_BIT_VCOM;

  sharpmem_buffer = (uint8_t *)malloc((WIDTH + 16) * HEIGHT / 8 + 2);

  if (!sharpmem_buffer)
    return false;

  sharpmem_buffer += 1;
  sharpmem_buffer[-1] = _sharpmem_vcom | SHARPMEM_BIT_WRITECMD;
  sharpmem_buffer[(WIDTH + 16) * HEIGHT / 8 + 1] = 0x00;
  setRotation(0);

  // SETUP DMA
  myDMA.setTrigger(SERCOM4_DMAC_ID_TX);
  myDMA.setAction(DMA_TRIGGER_ACTON_BEAT);

  stat = myDMA.allocate();

  myDMA.addDescriptor(
      sharpmem_buffer - 1,              // move data from here
      (void *)(&SERCOM4->SPI.DATA.reg), // to here (M0)
      ((WIDTH + 16) * HEIGHT / 8) + 2,  // this many...
      DMA_BEAT_SIZE_BYTE,               // bytes/hword/words
      true,                             // increment source addr?
      false);                           // increment dest addr?

  myDMA.setCallback(PetDisplay::dma_callback);

  return true;
}

// 1<<n is a costly operation on AVR -- table usu. smaller & faster
static const uint8_t PROGMEM set[] = {1, 2, 4, 8, 16, 32, 64, 128},
                             clr[] = {(uint8_t)~1, (uint8_t)~2, (uint8_t)~4,
                                      (uint8_t)~8, (uint8_t)~16, (uint8_t)~32,
                                      (uint8_t)~64, (uint8_t)~128};

static const uint8_t PROGMEM set1[] = {0x03, 0x0C, 0x30, 0xC0, 0x00, 0x00, 0x00, 0x00},
                             clr1[] = {0xFC, 0xF3, 0xCF, 0x3F, 0xFF, 0xFF, 0xFF, 0xFF},
                             set2[] = {0x00, 0x00, 0x00, 0x00, 0x03, 0x0C, 0x30, 0xC0},
                             clr2[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0xF3, 0xCF, 0x3F};

void PetDisplay::drawPixel(int16_t x, int16_t y, uint16_t color)
{
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
    //y += 8;
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
    sharpmem_buffer[(y * (WIDTH + 16) + x) / 8] &= pgm_read_byte(&clr[x & 7]);
  }
  else
  {
    sharpmem_buffer[(y * (WIDTH + 16) + x) / 8] |= pgm_read_byte(&set[x & 7]);
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

  return sharpmem_buffer[(y * (WIDTH + 16) + x) / 8] & pgm_read_byte(&set1[x & 7]) ? 1 : 0;
}

/**************************************************************************/
/*!
    @brief Clears the screen
*/
/**************************************************************************/
void PetDisplay::clearDisplay()
{
  memset(sharpmem_buffer, 0xFF, ((WIDTH + 16) * HEIGHT) / 8);
  spidev->beginTransaction();
  // Send the clear screen command rather than doing a HW refresh (quicker)
  digitalWrite(_cs, HIGH);

  uint8_t clear_data[2] = {_sharpmem_vcom | SHARPMEM_BIT_CLEAR, 0x00};
  spidev->transfer(clear_data, 2);

  TOGGLE_VCOM;
  digitalWrite(_cs, LOW);
  spidev->endTransaction();
}

void PetDisplay::dma_callback(Adafruit_ZeroDMA *dma)
{
  digitalWrite(8, LOW);
  SPI.endTransaction();
  transfer_is_done = true;
}

/**************************************************************************/
/*!
    @brief Renders the contents of the pixel buffer on the LCD
*/
/**************************************************************************/
void PetDisplay::refresh(void)
{
  if (!transfer_is_done)
  {
    //Serial.println("skipping frame, still transmitting");
    return;
  }
  transfer_is_done = false;

  // SPI_BITORDER_LSBFIRST, SPI_MODE0
  spidev->beginTransaction();

  digitalWrite(_cs, HIGH);
  TOGGLE_VCOM;

  // START DMA JOB
  stat = myDMA.startJob();
}


void PetDisplay::drawFrame(image_t *image,
                meta_t *meta,
                uint8_t dx, uint8_t dy,
                uint8_t frame,
                uint8_t off_color,
                uint8_t on_color,
                uint8_t alpha_color)
{
    ImageMeta &m = *((ImageMeta *)meta);
    
    image_t *bitmap = image;

    if (frame < m.tileCount)
    {

        if (m.encoding)
        {
            bitmap += ((meta_t*)(&m.tileOffset))[frame];
            drawSpanMap(bitmap, m, dx, dy, off_color, on_color, alpha_color);
        }
        // move bitmap to point to start of image
        // frames are not packed into eachother
        // for tiles 4 bytes per image_t, 16 pixels alpha, 32 pixels b/w
        else
        {
            if (m.alpha)
            {
                bitmap = bitmap + (m.width * m.height) / 16 * frame;
            }
            else
            {
                bitmap = bitmap + (m.width * m.height) / 32 * frame;
            }

            drawBitmap(bitmap, m, dx, dy, off_color, on_color, alpha_color);
        }
    }
}


void PetDisplay::drawFrame(const char* name,
                uint8_t dx, uint8_t dy,
                uint8_t frame,
                uint8_t off_color,
                uint8_t on_color,
                uint8_t alpha_color)
{
    uint16_t *meta;
    uint32_t *image;
    g::g_cache->GetGraphic(name, &meta, &image);
    drawFrame(image, meta, dx, dy, frame, off_color, on_color, alpha_color);
}

void PetDisplay::drawImage(image_t *image,
                meta_t *meta,
                uint8_t dx, uint8_t dy,
                uint8_t off_color,
                uint8_t on_color,
                uint8_t alpha_color)
{
    ImageMeta &m = *((ImageMeta *)meta);
    image_t *bitmap = image;

    if (m.encoding)
    {
        drawSpanMap(bitmap, m, dx, dy, off_color, on_color, alpha_color);
    }
    else
    {
        drawBitmap(bitmap, m, dx, dy, off_color, on_color, alpha_color);
    }
}

void PetDisplay::drawImage(const char* name,
                uint8_t dx, uint8_t dy,
                uint8_t off_color,
                uint8_t on_color,
                uint8_t alpha_color)
{
    uint16_t *meta;
    uint32_t *image;
    g::g_cache->GetGraphic(name, &meta, &image);
    drawImage(image, meta, dx, dy, off_color, on_color, alpha_color);
}

void PetDisplay::drawSpanMap(image_t *bitmap, const ImageMeta &meta,
                  uint8_t dx, uint8_t dy,
                  uint8_t off_color,
                  uint8_t on_color,
                  uint8_t alpha_color)
{
    uint32_t bitmapLength = meta.width * meta.height;
    uint32_t pixelsRead = 0;
    uint8_t currByte = 0;
    uint8_t currPack = 0;

    while (pixelsRead < bitmapLength)
    {
        uint8_t pixelData = bitmap[currPack] >> currByte * 8;

        uint8_t length = 0;
        uint8_t color = 0;

        if (meta.alpha)
        {
            length = pixelData & 0x3F;
            color = (pixelData >> 6) & 0x3;
        }
        else
        {
            length = pixelData & 0x7F;
            color = (pixelData >> 7) & 0x1;
        }

        for (uint8_t p = 0; p < length; p++)
        {
            uint8_t i = pixelsRead % meta.width;
            uint8_t j = pixelsRead / meta.width;

            if (color == 1 && on_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, on_color);
            }
            else if (color == 2 && alpha_color != PET_CLEAR)
            {
                setPixel(dx + i, dy + j, alpha_color);
            }
            // else if(pixel == 0x3 && on_color != PET_CLEAR)
            // {
            //     setPixel(dx + i, dy + j, on_color);
            // }
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


void PetDisplay::drawBitmap(image_t *bitmap, const ImageMeta &meta,
                uint8_t dx, uint8_t dy,
                uint8_t off_color,
                uint8_t on_color,
                uint8_t alpha_color)
{
    if (meta.alpha)
    {
        // 16 pixels per uint32_t
        uint32_t pack = 0;
        uint8_t packIndex = 0;
        uint8_t bitmapPack = 0;
        // seq array access for performance
        for (uint8_t j = 0; j < meta.height; j++)
        {
            for (uint8_t i = 0; i < meta.width; i++)
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
                // else if(pixel == 0x3 && on_color != PET_CLEAR)
                // {
                //     setPixel(dx + i, dy + j, on_color);
                // }
                else if (pixel == 0 && off_color != PET_CLEAR)
                {
                    setPixel(dx + i, dy + j, off_color);
                }
            }
        }
    }
    else
    {
        // 32 pixels per uint32_t
        uint32_t pack = 0;
        uint8_t packIndex = 0;
        uint8_t bitmapPack = 0;
        // seq array access for performance
        for (uint8_t j = 0; j < meta.height; j++)
        {
            for (uint8_t i = 0; i < meta.width; i++)
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
}

inline void PetDisplay::setPixel(uint8_t x, uint8_t y, uint8_t c)
{
    drawPixel(x, y, c);
}

inline void PetDisplay::setPixel8(uint8_t x, int8_t y, uint8_t data)
{
    sharpmem_buffer[(y * WIDTH + x) / 8] = pgm_read_byte(data);
}

void PetDisplay::fillDisplay(uint8_t color)
{
    // memset(sharpmem_buffer, color ? 0x00 : 0xFF, (WIDTH * HEIGHT) / 8);
    fillDisplayBuffer(color);
    refresh();
}

void PetDisplay::fillDisplayBuffer(uint8_t color)
{
    memset(sharpmem_buffer, color ? 0x00 : 0xFF, ((WIDTH + 16) * HEIGHT) / 8);
    uint8_t bytes_per_line = (WIDTH + 16) / 8;
    uint32_t totalbytes = ((WIDTH + 16) * HEIGHT) / 8;

    for (uint32_t i = 0; i < totalbytes; i += bytes_per_line)
    {

        // clear data between first and last byte of line
        memset(sharpmem_buffer + i + 1, color ? 0x00 : 0xFF, bytes_per_line - 2);

        // reset address and end of line data
        sharpmem_buffer[i] = ((i + 1) / ((WIDTH + 16) / 8)) + 1;
        sharpmem_buffer[i + bytes_per_line - 1] = 0x00;
    }
}