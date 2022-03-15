#ifndef PAWPET_InterruptHandler_H
#define PAWPET_InterruptHandler_H

#include <Arduino.h>

class InterruptHandler
{
    public:
    static void attachInterruptWakeup(uint32_t pin, voidFuncPtr callback, uint32_t mode)
    {
        EExt_Interrupts in = g_APinDescription[pin].ulExtInt;
        if (in == NOT_AN_INTERRUPT || in == EXTERNAL_INT_NMI)
            return;

        attachInterrupt(pin, callback, mode);

        // enable EIC clock
        GCLK->CLKCTRL.bit.CLKEN = 0; // disable GCLK module
        while (GCLK->STATUS.bit.SYNCBUSY) {}

        GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK2 |
                                    GCLK_CLKCTRL_ID(GCM_EIC)); // EIC clock switched on GCLK2
        while (GCLK->STATUS.bit.SYNCBUSY) {}

        GCLK->GENCTRL.reg =
            (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_RUNSTDBY); // source for GCLK2 is OSCULP32K
        while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        // GCLK->GENCTRL.bit.RUNSTDBY = 1; // GCLK2 run standby
        // while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

        // Enable wakeup capability on pin in case being used during sleep
        EIC->WAKEUP.reg |= (1 << in);

        /* Errata: Make sure that the Flash does not power all the way down
        * when in sleep mode. */

        NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;
    }
};

#endif