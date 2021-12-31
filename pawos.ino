#include <Arduino.h>
#include <WInterrupts.h>
#include <wiring_private.h>
#include <SPI.h>
#include <Adafruit_TinyUSB.h>
#include <Adafruit_SPIFlash.h>
#include <FatLib/FatFileSystem.h>

#include "src/lib/PawPet_FlashTransport.h"
#include "src/lib/ArduinoLowPower.h"

#include "src/config.h"
#include "src/global.h"
#include "src/common.h"
#include "src/states/gamestate.h"

// TODO: Move setup of all devices elsewhere
// have the .ino to contain the core update loop only

static const SPIFlash_Device_t possible_devices[] = {
  MX25R1635F
};

SPIClass dispSPI(&sercom4, SHARP_MISO, SHARP_SCK, SHARP_MOSI, SPI_PAD_2_SCK_3, SERCOM_RX_PAD_0);

SPIClass flashSPI(&sercom2, FLASH_MISO, FLASH_SCK, FLASH_MOSI, SPI_PAD_0_SCK_1, SERCOM_RX_PAD_2);

PawPet_FlashTransport_SPI flashTransport(FLASH_CS, flashSPI);
Adafruit_SPIFlash flash(&flashTransport);

PetDisplay display(&dispSPI, SHARP_SS, DISP_WIDTH, DISP_HEIGHT);
GameState *currentState;

volatile bool buttonWakeup;
volatile uint32_t nextSleepTime;

void buttonWakeupCallback();
uint16_t readButtons();
bool drawOverlay();

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;
int32_t msc_read_cb(uint32_t lba, void *buffer, uint32_t bufsize);
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize, uint32_t tag);
void msc_flush_cb(void);
bool msc_ready_cb();

#ifdef DEBUG
#include <ZeroRegs.h>
#endif

// Set to true when PC write to flash
bool changed;
uint32_t usbConnectedTime = false;

static char buffer[80];

void setup(void)
{
    Serial.begin(9600);

    // disable standby usb
    USB->DEVICE.CTRLA.bit.RUNSTDBY = 0;

    // global clocks

    // Disable 8mhz GLCK 3 that bootloader has setup, unused?
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN_GCLK3;
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(3);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // disable DAC clock
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_DAC);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // power manager
    // disable unused counters
    // TODO: find proper way of doing this
    uint32_t currSet = PM->APBCMASK.reg;

    currSet &= ~PM_APBCMASK_SERCOM0; // hardware serial
    currSet &= ~PM_APBCMASK_SERCOM1; // unused
    currSet &= ~PM_APBCMASK_SERCOM3; // i2c
    // why does GLCK_SERCOM4_CORE (flash) show up as set but not GLCK_SERCOM2_CORE (display)?
    // currSet &= ~PM_APBCMASK_SERCOM5; // debug port?

    currSet &= ~PM_APBCMASK_TCC0;
    currSet &= ~PM_APBCMASK_TCC1;
    currSet &= ~PM_APBCMASK_TCC2;
    currSet &= ~PM_APBCMASK_TC3;
    currSet &= ~PM_APBCMASK_TC4;
    currSet &= ~PM_APBCMASK_TC6;
    currSet &= ~PM_APBCMASK_TC7;
    currSet &= ~PM_APBCMASK_DAC;

    PM->APBCMASK.reg = currSet;

    // setup pin mappings
    pinMode(PIN_BEEPER, OUTPUT);
    tone(PIN_BEEPER, NOTE_C4, 250);

    display.begin();
    display.fillDisplayBuffer(PET_BLACK);
    display.refresh();
    display.setRotation(DISP_ROTATION);

    pinMode(PIN_BUTTON_A, INPUT_PULLUP);
    pinMode(PIN_BUTTON_B, INPUT_PULLUP);
    pinMode(PIN_BUTTON_C, INPUT_PULLUP);

    pinMode(PIN_BUTTON_P, INPUT_PULLUP);
    LowPower.attachInterruptWakeup(PIN_BUTTON_P, buttonWakeupCallback, FALLING);

    pinMode(PIN_UP, INPUT_PULLUP);
    pinMode(PIN_LEFT, INPUT_PULLUP);
    pinMode(PIN_DOWN, INPUT_PULLUP);
    pinMode(PIN_RIGHT, INPUT_PULLUP);

    analogReadResolution(12);
    pinMode(PIN_VBAT, INPUT);

    flashSPI.begin();
    pinPeripheral(FLASH_MISO, PIO_SERCOM);
    pinPeripheral(FLASH_SCK, PIO_SERCOM_ALT);
    pinPeripheral(FLASH_MOSI, PIO_SERCOM_ALT);
    pinPeripheral(FLASH_CS, PIO_DIGITAL);

    flash.begin(possible_devices);
    pinPeripheral(FLASH_MISO, PIO_SERCOM);
    pinPeripheral(FLASH_SCK, PIO_SERCOM_ALT);
    pinPeripheral(FLASH_MOSI, PIO_SERCOM_ALT);
    pinPeripheral(FLASH_CS, PIO_DIGITAL);

    // Init file system on the flash
    g::stats.filesysFound = g::g_fatfs.begin(&flash);
    g::stats.flashSize = flash.size();

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

    // disable and re-enable usb to get serial and usb msc at the same time
    USBDevice.detach();
    delay(500);
    USBDevice.attach();

#ifdef DEBUG
    // wait for serial to attach
    while (!Serial)
        ;

    // dump registers for debugging power settings
    ZeroRegOptions opts = {Serial, false};
    printZeroRegs(opts);
#endif

    buttonWakeup = false;
    nextSleepTime = 40000;
    currentState = new MenuState();
    tone(PIN_BEEPER, NOTE_D4, 250);
}

uint32_t sleepTicks = 0;
uint16_t keysPressed = 0;
uint16_t prevKeysPressed = 0;

uint32_t currentTimeMs = 0;
uint32_t frameTimeMs = 0;
uint32_t prevFrameMs = 0;

uint32_t droppedFrames = 0;

uint32_t requestedFpsSleep = k_30_fpsSleepMs;

bool dirtyFrameBuffer = false;
void loop(void)
{
    //// UPDATE ////
    keysPressed |= readButtons();
    
    currentTimeMs = millis();

    if (keysPressed)
    {
        nextSleepTime = currentTimeMs + 60000;
    }

    //// DRAW ////
    frameTimeMs = (currentTimeMs - prevFrameMs);
    if (frameTimeMs >= requestedFpsSleep)
    {
        prevFrameMs = currentTimeMs;

        g::keyPressed = keysPressed;
        g::keyReleased = ~(prevKeysPressed) & (keysPressed);
        
        GameState *nextState = currentState->update();

        if (nextState != currentState)
        {
            delete currentState;
            currentState = nextState;

            switch(nextState->tick)
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
            
        }

        // display is done with dma transfer
        if (!display.isFrameLocked())
        {
            if (currentState->redraw)
            {
                display.fillDisplayBuffer();
                currentState->draw(&display);

                dirtyFrameBuffer = true;
                currentState->redraw = false;
            }

            // TODO, draw overlay should say if it needs an update
            dirtyFrameBuffer |= drawOverlay();

            if (dirtyFrameBuffer)
            {
                dirtyFrameBuffer = false;
                display.refresh();
            }
        }
        // display was still refreshing when next refresh was requested
        else if (currentState->redraw)
        {
            droppedFrames++;
        }

        prevKeysPressed = keysPressed;
        keysPressed = 0;
    }

    //// SLEEP ////
    if (currentTimeMs >= nextSleepTime)
    {
        display.sync();

        // TODO: wakeup on flash chip not being triggered correctly
        // BUG: on reset after sleep flash doesn't show up until second reset
        // flashTransport.runCommand(0xB9); // deep sleep

        LowPower.deepSleep(k_sleepTimeMs);
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
                currentState->draw(&display);
                drawOverlay();
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

uint16_t readButtons()
{
    uint16_t inputState = 0;
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

bool drawOverlay()
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
        display.printf("%02u %02u", frameTimeMs, droppedFrames % 100);
        display.printf(" %02u\n", (nextSleepTime - currentTimeMs) / 1000);
    }
    else
    {
        display.printf("Zz\n");
    }

    int intBat = Util::batteryLevel();

    // 1.5*2 alk
    // 1.4*2 nimh
    // 2.60-2.80 full charge
    // 2.0v discharged?
    // discharge curves are non-linear, needs tunning
    int batFrame = 0;

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
    else if (intBat > 200)
    {
        batFrame = 3;
    }
    else
    {
        batFrame = 4;
    }

    // display.drawFrame(PETPIC(battery), 48, 0, batFrame);
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
    return flash.readBlocks(lba, (uint8_t *)buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when received WRITE10 command.
// Process data in buffer to disk's storage and
// return number of written bytes (must be multiple of block size)
int32_t msc_write_cb(uint32_t lba, uint8_t *buffer, uint32_t bufsize)
{
    // check_uf2_handover(buffer, bufsize, 4, 5, tag);
    // Note: SPIFLash Bock API: readBlocks/writeBlocks/syncBlocks
    // already include 4K sector caching internally. We don't need to cache it, yahhhh!!
    return flash.writeBlocks(lba, buffer, bufsize / 512) ? bufsize : -1;
}

// Callback invoked when WRITE10 command is completed (status received and accepted by host).
// used to flush any pending cache.
void msc_flush_cb(void)
{
    // sync with flash
    flash.syncBlocks();

    // clear file system's cache to force refresh
    g::g_fatfs.cacheClear();

    // changed = true;
}

bool msc_ready_cb(void)
{
    // usbConnectedTime = millis();
    // nextSleepTime = usbConnectedTime + 60000;
    return true;
}