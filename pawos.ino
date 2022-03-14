#include <Adafruit_SPIFlash.h>
#include <Adafruit_TinyUSB.h>
#include <Arduino.h>
#include <FatLib/FatFileSystem.h>
#include <SPI.h>
#include <WInterrupts.h>
#include <wiring_private.h>

#include "src/lib/ArduinoLowPower.h"
#include "src/lib/PawPet_FlashTransport.h"
#include "src/lib/RTCZero.h"

#include "src/common.h"
#include "src/config.h"
#include "src/global.h"
#include "src/lib/PawPet_SleepyDog.h"
#include "src/states/gamestate.h"

// #define DEBUG 1
// #define INSTALLER 1

#ifdef INSTALLER
#include "src/states/pawos_install.h"
#endif

#ifdef DEBUG
#include <ZeroRegs.h>
#endif

// TODO: Move setup of all devices elsewhere
// have the .ino to contain the core update loop only

static const SPIFlash_Device_t possible_devices[] = {MX25R1635F};
SPIClass flashSPI(&sercom2, FLASH_MISO, FLASH_SCK, FLASH_MOSI, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_2);
PawPet_FlashTransport_SPI flashTransport(FLASH_CS, flashSPI);

SPIClass dispSPI(&sercom4, SHARP_MISO, SHARP_SCK, SHARP_MOSI, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0);
PetDisplay display(&dispSPI, SHARP_SS, DISP_WIDTH, DISP_HEIGHT);

// used by usb and button interrupts to keep the device awake when in use
volatile bool buttonWakeup;
volatile uint32_t nextSleepTime;

// button reading and interrupts
void buttonWakeupCallback();
uint8_t readButtons();

// gamestate and ui
bool drawTimeAndBattery();
GameState *currentState;

void disableUnusedClocks();

// USB Mass Storage object functions
Adafruit_USBD_MSC usb_msc;
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize);
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize, uint32_t tag);
void msc_flush_cb(void);
bool msc_ready_cb();
uint16_t intBat = 300;

void setup(void)
{
    g::g_flash = new Adafruit_SPIFlash(&flashTransport);
    Adafruit_SPIFlash& flash = *g::g_flash;

#ifdef DEBUG
    Serial.begin(9600);

    // wait for serial to attach
    while (!Serial) {}
#endif
    // power management
    disableUnusedClocks();

    // Watchdog._initialize_wdt();
    // WDT->CTRL.reg = 0; // Disable watchdog for config
    // while (WDT->STATUS.bit.SYNCBUSY) {};

    // set nvm wait states to 3, allowing for operation down to 1.6v
    // though display cuts out at 1.9v
    NVMCTRL->CTRLB.bit.RWS = 3;

    // setup pin mappings
    display.setRotation(DISP_ROTATION);
    display.begin();
    display.refresh();

    pinMode(PIN_BEEPER, OUTPUT);

    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    pinMode(PIN_BUTTON_P, INPUT_PULLUP);
    // LowPower.attachInterruptWakeup(PIN_BUTTON_P, buttonWakeupCallback, FALLING);

    EExt_Interrupts in = g_APinDescription[PIN_BUTTON_P].ulExtInt;
    if (in == NOT_AN_INTERRUPT || in == EXTERNAL_INT_NMI)
        return;

    attachInterrupt(PIN_BUTTON_P, buttonWakeupCallback, FALLING);

    // enable EIC clock
    GCLK->CLKCTRL.bit.CLKEN = 0; // disable GCLK module
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    GCLK->CLKCTRL.reg = (uint16_t)(GCLK_CLKCTRL_CLKEN | GCLK_CLKCTRL_GEN_GCLK3 |
                                   GCLK_CLKCTRL_ID(GCM_EIC)); // EIC clock switched on GCLK3
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    // GCLK->GENCTRL.reg =
    //     (GCLK_GENCTRL_GENEN | GCLK_GENCTRL_SRC_OSCULP32K | GCLK_GENCTRL_ID(2) | GCLK_GENCTRL_RUNSTDBY); // source for
    //     GCLK2 is OSCULP32K
    // while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

    // GCLK->GENCTRL.bit.RUNSTDBY = 1; // GCLK2 run standby
    // while (GCLK->STATUS.reg & GCLK_STATUS_SYNCBUSY) {}

    // Enable wakeup capability on pin in case being used during sleep
    EIC->WAKEUP.reg |= (1 << in);

    /* Errata: Make sure that the Flash does not power all the way down
     * when in sleep mode. */

    NVMCTRL->CTRLB.bit.SLEEPPRM = NVMCTRL_CTRLB_SLEEPPRM_DISABLED_Val;

    pinMode(PIN_UP, INPUT_PULLUP);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_DOWN, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);

    analogReadResolution(12);
    pinMode(PIN_VBAT, INPUT);
    pinPeripheral(PIN_VMON_EN, PIO_DIGITAL);
    pinMode(PIN_VMON_EN, OUTPUT);
    digitalWrite(PIN_VMON_EN, HIGH);

    flashSPI.begin();
    pinPeripheral(FLASH_MISO, PIO_SERCOM);
    pinPeripheral(FLASH_SCK, PIO_SERCOM_ALT);
    pinPeripheral(FLASH_MOSI, PIO_SERCOM_ALT);
    pinPeripheral(FLASH_CS, PIO_DIGITAL);

    flash.begin(possible_devices);

    // Init file system on the flash
    g::g_fatfs = new FatFileSystem();
    g::g_stats.filesysFound = g::g_fatfs->begin(&flash);
    g::g_stats.flashSize = flash.size();

    {
        // Set disk vendor id, product id and revision with string up to 8, 16, 4 characters respectively
        usb_msc.setID("Adafruit", "External Flash", "1.0");

        // Set callback
        usb_msc.setReadWriteCallback(msc_read_cb, msc_write_cb, msc_flush_cb);
        usb_msc.setReadyCallback(msc_ready_cb);

        // Set disk size, block size should be 512 regardless of spi flash page size
        usb_msc.setCapacity(flash.pageSize() * flash.numPages() / 512, 512);

        // MSC is ready for read/write
        usb_msc.setUnitReady(true);

        usb_msc.begin();
    }

    Watchdog.enable(WatchdogSAMD::WATCHDOG_TIMER_2_S);

    buttonWakeup = false;
    nextSleepTime = 40000;
#ifdef INSTALLER
    currentState = new InstallMenu();
#else
    currentState = new MenuState();
#endif

    g::g_rtc.begin();
    g::g_rtc.setHours(0);
    g::g_rtc.setMinutes(0);
    g::g_rtc.setSeconds(0);
    g::g_rtc.setDate(1, 1, (uint8_t)2022);
    g::g_stats.bootTime = g::g_rtc.getEpoch();

    intBat = Util::batteryLevel();

#ifdef DEBUG
    // dump registers for debugging power settings
    ZeroRegOptions opts = {Serial, false};
    printZeroRegs(opts);
#endif

// disable and re-enable usb to get serial and usb msc at the same time
#ifndef DEBUG
    USBDevice.detach();
    delay(500);
    USBDevice.attach();
#endif
    if (!g::g_stats.filesysFound)
    {
        tone(PIN_BEEPER, NOTE_C4, 250);
    }
}

uint32_t sleepTicks = 0;
uint8_t keysState = 0;
uint8_t prevKeysState = 0;

uint32_t currentTimeMs = 0;
uint32_t frameTimeMs = 0;
uint32_t prevFrameMs = 0;

uint8_t droppedFrames = 0;
uint32_t peakUpdateTime = 0;
uint32_t peakDrawTime = 0;

uint16_t requestedFpsSleep = k_30_fpsSleepMs;

bool dirtyFrameBuffer = false;
void loop(void)
{
    Watchdog.reset();

    //// UPDATE ////
    keysState |= readButtons();

    currentTimeMs = millis();

    if (keysState)
    {
        nextSleepTime = currentTimeMs + 60000;
    }

    //// DRAW ////
    frameTimeMs = (currentTimeMs - prevFrameMs);
    if (frameTimeMs >= requestedFpsSleep)
    {

        prevFrameMs = currentTimeMs;
        // held is currenlty just previous update, a function of update loop speed
        // between 66 and 2000 ms
        g::g_keyPressed = ~(prevKeysState)&keysState;
        g::g_keyReleased = prevKeysState & ~(keysState);
        g::g_keyHeld = prevKeysState & keysState;

        uint32_t t1 = millis();
        GameState *nextState = currentState->update();
        uint32_t d1 = millis() - t1;
        peakUpdateTime = (d1 > peakUpdateTime) ? d1 : peakUpdateTime;

        if (nextState != currentState)
        {
            delete currentState;
            currentState = nextState;

            peakDrawTime = 0;
            peakUpdateTime = 0;

            switch (nextState->tick)
            {
            case k_tickTime30:
                requestedFpsSleep = k_30_fpsSleepMs;
                break;
            case k_tickTime15:
                requestedFpsSleep = k_15_fpsSleepMs;
                break;
            case k_tickTime5:
                requestedFpsSleep = k_5_fpsSleepMs;
                break;
            case k_tickTime1:
                requestedFpsSleep = k_1_fpsSleepMs;
                break;
            }

            intBat = Util::batteryLevel();
        }

        t1 = millis();
        if (currentState->redraw)
        {
            display.fillDisplayBuffer();
            display.setCursor(0, 8);
            display.setTextColor(PET_BLACK);
            currentState->draw(&display);

            // TODO, draw overlay should say if it needs an update
            dirtyFrameBuffer |= drawTimeAndBattery();

            dirtyFrameBuffer = true;
            currentState->redraw = false;
        }

        // display is done with dma transfer, approx 9ms
        if (!display.isFrameLocked())
        {
            if (dirtyFrameBuffer)
            {
                dirtyFrameBuffer = false;

                d1 = millis() - t1;
                peakDrawTime = (d1 > peakDrawTime) ? d1 : peakDrawTime;
                display.refresh();
            }
        }
        // display was still refreshing when next refresh was requested
        // somehow has hit a less than 10 ms draw time
        else if (currentState->redraw)
        {
            droppedFrames++;
        }

        prevKeysState = keysState;
        keysState = 0;
    }
    else
    {
        // skipped update + draw
        // sleep by amount remaining until next frame update?
        // would drop inputs though... sleep by some reasonable polling increment?
    }

    //// SLEEP ////
    if (currentTimeMs >= nextSleepTime)
    {
        intBat = Util::batteryLevel();

        if (intBat < 201)
        {
            // disable clocks and shutdown?
        }
        display.sync();

        // TODO: wakeup on flash chip not being triggered correctly
        // BUG: on reset after sleep flash doesn't show up until second reset
        // flashTransport.runCommand(0xB9); // deep sleep

        Watchdog.disable();
        // LowPower.deepSleep(k_sleepTimeMs);
        Watchdog.sleep(WatchdogSAMD::WATCHDOG_TIMER_64_S);
        Watchdog.enable(WatchdogSAMD::WATCHDOG_TIMER_2_S);

        buttonWakeup = false;

        if (!buttonWakeup)
        {
            sleepTicks++;
        }

        //// SLEEP-BURN-IN-REFRESH ////
        // every 120 * k_sleepTimeMs refresh screen
        // display must be refreshed every 2 hours to avoid pixel burn-in
        if (sleepTicks >= 110)
        {
            sleepTicks = 0;
            display.fillDisplayBuffer();
            display.refresh();
            display.sync();

            // force redraw screen since it was cleared
            currentState->redraw = true;
        }

        //// SLEEP-UPDATE ////
        GameState *nextState = currentState->update();
        if (nextState != currentState)
        {
            delete currentState;
            currentState = nextState;
        }

        // TODO remove, being used to force overlay redraw in sleep mode
        // find better way to update every minute logic
        currentState->redraw = true;

        //// SLEEP-DRAW ////
        if (!display.isFrameLocked())
        {
            if (currentState->redraw)
            {
                display.fillDisplayBuffer();
                display.setCursor(0, 8);
                display.setTextColor(PET_BLACK);
                currentState->draw(&display);
                drawTimeAndBattery();
                dirtyFrameBuffer = true;
                currentState->redraw = false;
            }

            if (dirtyFrameBuffer)
            {
                dirtyFrameBuffer = false;
                display.refresh();
            }
        }
        display.sync();
    }
}

uint8_t readButtons()
{
    uint8_t inputState = 0;
    if (!digitalRead(PIN_BUTTON_A))
    {
        inputState |= BUTTON_A;
    }
    if (!digitalRead(PIN_BUTTON_B))
    {
        inputState |= BUTTON_B;
    }
    if (!digitalRead(PIN_BUTTON_P))
    {
        inputState |= BUTTON_P;
    }
    if (!digitalRead(PIN_BUTTON_C))
    {
        inputState |= BUTTON_C;
    }
    if (!digitalRead(PIN_UP))
    {
        inputState |= UP;
    }
    if (!digitalRead(PIN_LEFT))
    {
        inputState |= LEFT;
    }
    if (!digitalRead(PIN_DOWN))
    {
        inputState |= DOWN;
    }
    if (!digitalRead(PIN_RIGHT))
    {
        inputState |= RIGHT;
    }

    return inputState;
}

PetSprite _battery(battery);
bool drawTimeAndBattery()
{
    uint32_t elapsedTimeSeconds = currentTimeMs / 1000.0;
    // uint32_t hours = (elapsedTimeSeconds / 60 / 60);
    uint32_t minutes = (elapsedTimeSeconds / 60);
    uint32_t seconds = (elapsedTimeSeconds % 60);

    display.setCursor(0, 0);
    display.setTextColor(PET_WHITE, PET_BLACK); // inverted text
    display.fillRect(0, 0, 64, 8, PET_BLACK);

    // display.printf("%02u:%02u ", minutes, seconds);
    if (nextSleepTime > currentTimeMs)
    {
        display.printf("%02u %02u %02u", peakUpdateTime, peakDrawTime, droppedFrames % 100);
    }
    else
    {
        display.printf("Zz\n");
    }

    // 1.5*2 alk, 0.8v cutoff
    // 1.4*2 nimh? 1.0v cutoff
    //
    // discharge curves are non-linear, needs tunning
    // 2.6v >- full
    // 2.4v >- 3/4  - nimh spends most time here
    // 2.2v >- 2/4
    // 2.1v >- 1/4
    // 2.01v >- empty
    // 2.0v < shutdown, refuse to boot, below cutoff voltage
    // 1.9v - display will not turn on at this voltage
    // below 2v, display battery replace logo
    //
    // 2.7v - full
    // 2.6v - 3/4
    // 2.4v - 2/4SWS
    // 2.2v - 1/4
    // 2.0v - empty
    // 1.9v - display will not turn on
    // 1.6v cutoff, display will have already been inoperable

    uint8_t batFrame = 0;

    if (intBat > 260)
    {
        batFrame = 0;
    }
    else if (intBat > 240)
    {
        batFrame = 1;
    }
    else if (intBat > 220)
    {
        batFrame = 2;
    }
    else if (intBat > 210)
    {
        batFrame = 3;
    }
    else
    {
        batFrame = 4;
    }

    _battery.draw(&display, 48, 0, batFrame);
    // display.drawFrame("battery", 48, 0, batFrame);

    return true;
}

void buttonWakeupCallback()
{
    nextSleepTime = millis() + 60000;
    buttonWakeup = true;
}

// Callback invoked when received READ10 command.
// Copy disk's data to buffer (up to bufsize) and
// return number of copied bytes (must be multiple of block size)
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize)
{
    // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
    // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
    return g::g_flash->readBlocks(lba, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize)
{
    // check_uf2_handover(buffer, bufsize, 4, 5, tag);
    // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
    // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
    return g::g_flash->writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
    // sync with flash
    g::g_flash->syncBlocks();

    // clear file system's cache to force refresh
    g::g_fatfs->cacheClear();

    // changed = true;
}

bool msc_ready_cb(void)
{
    // usbConnectedTime = millis();
    nextSleepTime = millis() + 60000;
    return true;
}

void disableUnusedClocks()
{
    // disable standby usb
    USB->DEVICE.CTRLA.bit.RUNSTDBY = 0;

    // global clocks

    // Disable 8mhz GLCK 3 that bootloader has setup, unused?
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN_GCLK3;
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(3);
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    // disable DAC clock
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_DAC);
    while (GCLK->STATUS.bit.SYNCBUSY) {}

    // power manager
    // disable unused counters
    // TODO: find proper way of doing this
    uint32_t currSet = PM->APBCMASK.reg;

    currSet &= ~PM_APBCMASK_SERCOM0; // hardware serial
    currSet &= ~PM_APBCMASK_SERCOM1; // unused
    currSet &= ~PM_APBCMASK_SERCOM3; // i2c
    // why does GLCK_SERCOM4_CORE (flash) show up as set but not GLCK_SERCOM2_CORE (display)?
    // currSet &= ~PM_APBCMASK_SERCOM5; // debug port?

    // currSet &= ~PM_APBCMASK_TCC0; // unused... but bound to TCC0?
    // currSet &= ~PM_APBCMASK_TCC1; // vcom generator
    currSet &= ~PM_APBCMASK_TCC2;
    currSet &= ~PM_APBCMASK_TC3;
    currSet &= ~PM_APBCMASK_TC4;
    // currSet &= ~PM_APBCMASK_TC5; // tone generator
    currSet &= ~PM_APBCMASK_TC6;
    currSet &= ~PM_APBCMASK_TC7;
    currSet &= ~PM_APBCMASK_DAC;

    PM->APBCMASK.reg = currSet;
}