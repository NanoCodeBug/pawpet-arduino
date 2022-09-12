#ifndef _pawos_h
#define _pawos_h

#include <stdint.h>
#include <stdio.h>

#include "Adafruit_GFX.h"

#include <../../src/common.h>
#include <../../src/config.h>
#include <../../src/global.h>
#include <../../src/states/gamestate.h>

#include <SDL.h>
#include <SDL_rect.h>

void init();
void loop(void);

extern PetDisplay display;
extern uint8_t keysState;
extern int32_t sleepMillisRemain;

class WatchdogSAMD {
public:

  enum PawPet_WatchDog_Times {
    WATCHDOG_TIMER_64_MS = 0x0,
    WATCHDOG_TIMER_128_MS = 0x1,
    WATCHDOG_TIMER_256_MS = 0x2,
    WATCHDOG_TIMER_512_MS = 0x3,
    WATCHDOG_TIMER_1_S = 0x4,
    WATCHDOG_TIMER_2_S = 0x5,
    WATCHDOG_TIMER_4_S = 0x6,
    WATCHDOG_TIMER_8_S = 0x7,
    WATCHDOG_TIMER_16_S = 0x8,
    WATCHDOG_TIMER_32_S = 0x9,
    WATCHDOG_TIMER_64_S = 0xA,
    WATCHDOG_TIMER_128_S = 0xB
  };

  void sleep(WatchdogSAMD::PawPet_WatchDog_Times m) {
        sleepMillisRemain = 64000;
  }
};

#endif