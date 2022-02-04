

#include "WatchdogSAMD.h"
#include <sam.h>

void WatchdogSAMD::enable(uint8_t bits, bool isForSleep)
{
    // Enable the watchdog with a period up to the specified max period in
    // milliseconds.

    // Review the watchdog section from the SAMD21 datasheet section 17:
    // http://www.atmel.com/images/atmel-42181-sam-d21_datasheet.pdf

    int cycles;

    if (!_initialized)
        _initialize_wdt();

    WDT->CTRL.reg = 0; // Disable watchdog for config
    while (WDT->STATUS.bit.SYNCBUSY)
        ;

    // 32,768 / 256 = 128
    // 1 / 128 =7.8125 ms per tick
    // 8ms per tick, max number of ticks is 16384

    // 0x0 8 clock cycles
    // 0x1 16 clock cycles
    // 0x2 32 clock cycles
    // 0x3 64 clock cycles
    // 0x4 128 clock cycles
    // 0x5 256 clocks cycles
    // 0x6 512 clocks cycles
    // 0x7 1024 clock cycles
    // 0x8 2048 clock cycles
    // 0x9 4096 clock cycles
    // 0xA 8192 clock cycles
    // 0xB 16384 clock cycles

    // Watchdog timer on SAMD is a slightly different animal than on AVR.
    // On AVR, the WTD timeout is configured in one register and then an
    // interrupt can optionally be enabled to handle the timeout in code
    // (as in waking from sleep) vs resetting the chip.  Easy.
    // On SAMD, when the WDT fires, that's it, the chip's getting reset.
    // Instead, it has an "early warning interrupt" with a different set
    // interval prior to the reset.  For equivalent behavior to the AVR
    // library, this requires a slightly different configuration depending
    // whether we're coming from the sleep() function (which needs the
    // interrupt), or just enable() (no interrupt, we want the chip reset
    // unless the WDT is cleared first).  In the sleep case, 'windowed'
    // mode is used in order to allow access to the longest available
    // sleep interval (about 16 sec); the WDT 'period' (when a reset
    // occurs) follows this and is always just set to the max, since the
    // interrupt will trigger first.  In the enable case, windowed mode
    // is not used, the WDT period is set and that's that.
    // The 'isForSleep' argument determines which behavior is used;
    // this isn't present in the AVR code, just here.  It defaults to
    // 'false' so existing Arduino code works as normal, while the sleep()
    // function (later in this file) explicitly passes 'true' to get the
    // alternate behavior.

    if (isForSleep)
    {
        WDT->INTENSET.bit.EW = 1;      // Enable early warning interrupt
        WDT->CONFIG.bit.PER = 0xB;     // Period = max
        WDT->CONFIG.bit.WINDOW = bits; // Set time of interrupt
        WDT->CTRL.bit.WEN = 1;         // Enable window mode
        while (WDT->STATUS.bit.SYNCBUSY)
            ; // Sync CTRL write
    }
    else
    {
        WDT->INTENCLR.bit.EW = 1;   // Disable early warning interrupt
        WDT->CONFIG.bit.PER = bits; // Set period for chip reset
        WDT->CTRL.bit.WEN = 0;      // Disable window mode
        while (WDT->STATUS.bit.SYNCBUSY)
            ; // Sync CTRL write
    }

    reset();                  // Clear watchdog interval
    WDT->CTRL.bit.ENABLE = 1; // Start watchdog now!
    while (WDT->STATUS.bit.SYNCBUSY)
        ;
}

void WatchdogSAMD::reset()
{
    // Write the watchdog clear key value (0xA5) to the watchdog
    // clear register to clear the watchdog timer and reset it.

    if (!WDT->STATUS.bit.SYNCBUSY) // Check if the WDT registers are synchronized
    {
        REG_WDT_CLEAR = WDT_CLEAR_CLEAR_KEY; // Clear the watchdog timer
    }
}

uint8_t WatchdogSAMD::resetCause()
{

    return PM->RCAUSE.reg;
}

void WatchdogSAMD::disable()
{

    WDT->CTRL.bit.ENABLE = 0;
    while (WDT->STATUS.bit.SYNCBUSY)
        ;
}

void WDT_Handler(void)
{
    // ISR for watchdog early warning, DO NOT RENAME!

    WDT->CTRL.bit.ENABLE = 0; // Disable watchdog
    while (WDT->STATUS.bit.SYNCBUSY)
        ;                    // Sync CTRL write
    WDT->INTFLAG.bit.EW = 1; // Clear interrupt flag
}

void WatchdogSAMD::sleep(uint8_t bits)
{

    enable(bits, true); // true = for sleep

    // Enable standby sleep mode (deepest sleep) and activate.
    // Insights from Atmel ASF library.

    // Don't fully power down flash when in sleep
    NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;

    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    __DSB(); // Data sync to ensure outgoing memory accesses complete
    __WFI(); // Wait for interrupt (places device in sleep mode)

    // Code resumes here on wake (WDT early warning interrupt).
    // Bug: the return value assumes the WDT has run its course;
    // incorrect if the device woke due to an external interrupt.
    // Without an external RTC there's no way to provide a correct
    // sleep period in the latter case...but at the very least,
    // might indicate said condition occurred by returning 0 instead
    // (assuming we can pin down which interrupt caused the wake).
}

void WatchdogSAMD::_initialize_wdt()
{
    // One-time initialization of watchdog timer.
    // Insights from rickrlh and rbrucemtl in Arduino forum!

    // Generic clock generator 3, divisor = 32 (2^(DIV+1))
    GCLK->GENDIV.reg = GCLK_GENDIV_ID(3) | GCLK_GENDIV_DIV(7);

    // 32KHz /256 divisor above, this yields 125hz(ish) clock.
    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(3) | GCLK_GENCTRL_GENEN | GCLK_GENCTRL_RUNSTDBY | GCLK_GENCTRL_SRC_OSCULP32K |
                        GCLK_GENCTRL_DIVSEL;

    while (GCLK->STATUS.bit.SYNCBUSY) {};

    // WDT clock = clock gen 3
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_ID_WDT | GCLK_CLKCTRL_GEN_GCLK3;
    while (GCLK->STATUS.bit.SYNCBUSY) {};

    // Enable WDT early-warning interrupt
    NVIC_DisableIRQ(WDT_IRQn);
    NVIC_ClearPendingIRQ(WDT_IRQn);
    NVIC_SetPriority(WDT_IRQn, 0); // Top priority
    NVIC_EnableIRQ(WDT_IRQn);

    _initialized = true;
}
