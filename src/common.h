#pragma once

#include "config.h"
#include <Arduino.h>

#define PETPIC(x) x, x##_meta

typedef const uint32_t image_t;
typedef const uint16_t meta_t;

struct ImageMeta
{
    uint16_t width;
    uint16_t height;
    uint16_t alpha;      // has alpha channel
    uint16_t encoding;   // true: span encoded, false: bitmap
    uint16_t tileCount;  // number of tiles
    uint16_t tileOffset; // array of offsets to start of each tile
};

#define PET_WHITE 0
#define PET_BLACK 1
#define PET_CLEAR 2

// #define UF2_DEFINE_HANDOVER 1
// #define SAMD21 1
// #include <Adafruit_SleepyDog.h>

#include "sprites.h"

enum ButtonFlags
{
    BUTTON_A = 0x1,
    BUTTON_B = 0x2,
    BUTTON_C = 0x4,
    BUTTON_P = 0x8,

    UP = 0x10,
    DOWN = 0x20,
    LEFT = 0x40,
    RIGHT = 0x80,

    END = 0x100
};

extern "C" char *sbrk(int i);

class Util
{
  public:
    inline static int batteryLevel()
    {
        while (ADC->STATUS.bit.SYNCBUSY == 1) {}
        ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;  // Gain Factor Selection
        ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val; // 1.0V voltage reference

        float vbat = analogRead(PIN_VBAT);
        // return (int)vbat;
        vbat *= 2;      // we divided by 2, so multiply back
        vbat *= 2.0;    // Multiply by 2.0V, our reference voltage
        vbat /= 4096.0; // convert to voltage
        return vbat * 100;
    }

    static int32_t FreeRam()
    {
        char stack_dummy = 0;
        return &stack_dummy - sbrk(0);
    }
};

struct Point2D
{
    int16_t x;
    int16_t y;
};