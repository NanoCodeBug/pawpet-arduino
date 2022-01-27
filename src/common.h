#pragma once

#include "config.h"
#include <Arduino.h>

// #define UF2_DEFINE_HANDOVER 1
// #define SAMD21 1

struct meta_t
{
    uint16_t width;
    uint16_t height;
    uint16_t alpha;     // has alpha channel
    uint16_t encoding;  // true: span encoded, false: bitmap
    uint16_t tileCount; // number of tiles
    uint16_t _unused;
    // ALWAYS KEEP 4 BYTE MULTIPLE
};

struct image_t
{
    meta_t *meta = NULL;
    uint16_t *tileOffsets = NULL; // array of offsets to start of each tile
    uint32_t *data = NULL;
};

#define PET_WHITE 0
#define PET_BLACK 1
#define PET_CLEAR 2

#include "graphics/sprites.h"

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
    inline static uint16_t batteryLevel()
    {
        while (ADC->STATUS.bit.SYNCBUSY == 1) {}
        ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;  // Gain Factor Selection
        ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val; // 1.0V voltage reference

        digitalWrite(PIN_VMON_EN, LOW);

        float vbat = analogRead(PIN_VBAT);
        vbat *= 2;      // we divided by 2, so multiply back
        vbat *= 2.0;    // Multiply by 2.0V, our reference voltage
        vbat /= 4096.0; // convert to voltage

        digitalWrite(PIN_VMON_EN, HIGH);
        return vbat * 100;
    }

    static uint32_t FreeRam()
    {
        char stack_dummy = 0;
        return &stack_dummy - reinterpret_cast<char *>(sbrk(0));
    }
};
