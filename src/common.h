#pragma once

#include <Arduino.h>
#include <WInterrupts.h>
#include <SPI.h>

#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"


#define PETPIC( x ) x, x##_meta

struct ImageMeta
{
    uint16_t width;
    uint16_t height;
    uint16_t alpha;
    uint16_t encoding;
    uint16_t tileCount;
    uint16_t tileOffset;
};

#define PET_WHITE 0
#define PET_BLACK 1
#define PET_CLEAR 2

// definition only headers
#include "pet_graphics.h"
#include "pitches.h"
#include "config.h"


#define UF2_DEFINE_HANDOVER 1
#define SAMD21 1
//#include <Adafruit_SleepyDog.h>

#include "graphic_manager.h"

// global classes

#include "pet_display.h"

