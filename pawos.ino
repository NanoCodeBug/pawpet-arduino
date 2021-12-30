#include "src/common.h"
#include "RTCZero.h"
#include "src/lib/ArduinoLowPower.h"
#include "src/gamestate.h"
#include "wiring_private.h"
#include "src/lib/PawPet_FlashTransport.h"

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
void drawOverlay(uint16_t inputState);

// USB Mass Storage object
Adafruit_USBD_MSC usb_msc;

#ifdef FLASH_SETUP
#include "src/lib/ff.h"
#include "src/lib/diskio.h"
#endif

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

    USB->DEVICE.CTRLA.bit.RUNSTDBY = 0;

    // Disable 8mhz GLCK 3 that bootloader has setup, unused?

    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_GEN_GCLK3;
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    GCLK->GENCTRL.reg = GCLK_GENCTRL_ID(3);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // disable GLCK_DAC
    GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(GCM_DAC);
    while (GCLK->STATUS.bit.SYNCBUSY)
        ;

    // power manager

    // Clock SERCOM for Serial, TC/TCC for Pulse and Analog
    uint32_t currSet = PM->APBCMASK.reg;

    currSet &= ~PM_APBCMASK_SERCOM0; // hardware serial
    currSet &= ~PM_APBCMASK_SERCOM1; // unused
    currSet &= ~PM_APBCMASK_SERCOM3; // i2c
    // currSet &= ~PM_APBCMASK_SERCOM5; // debug port

    currSet &= ~PM_APBCMASK_TCC0;
    currSet &= ~PM_APBCMASK_TCC1;
    currSet &= ~PM_APBCMASK_TCC2;
    currSet &= ~PM_APBCMASK_TC3;
    currSet &= ~PM_APBCMASK_TC4;
    currSet &= ~PM_APBCMASK_TC6;
    currSet &= ~PM_APBCMASK_TC7;
    currSet &= ~PM_APBCMASK_DAC;

    PM->APBCMASK.reg = currSet;
    // why does GLCK_SERCOM4_CORE (flash) show up but not GLCK_SERCOM2_CORE? because 2 is being used with dma??


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
    // FORMAT FS - should only need to be done once
    g::stats.filesysFound = g::g_fatfs.begin(&flash);
    g::stats.flashSize = flash.size();

    if (!g::stats.filesysFound && g::stats.flashSize != 0)
    {
#ifdef FLASH_SETUP
        FATFS elmchamFatfs;
        uint8_t workbuf[4096]; // Working buffer for f_fdisk function.

        // Make filesystem.
        FRESULT r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));
        if (r != FR_OK)
        {
            while (1)
                yield();
        }

        // mount to set disk label
        r = f_mount(&elmchamFatfs, "0:", 1);
        if (r != FR_OK)
        {
            while (1)
                yield();
        }

        // Setting label
        r = f_setlabel("PAWPET");
        if (r != FR_OK)
        {
            while (1)
                yield();
        }

        // unmount
        f_unmount("0:");

        // sync to make sure all data is written to flash
        flash.syncBlocks();

#endif
    }
    else if (g::stats.flashSize == 0)
    {
        Serial.println("Flash chip not found");
    }
    
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

    USBDevice.detach();
    delay(500);
    USBDevice.attach();

    #ifdef DEBUG
        while (!Serial)
        {
            ; // wait for serial port to connect.
        }
    #endif

    buttonWakeup = false;
    nextSleepTime = 40000;
    currentState = new MenuState();
    tone(PIN_BEEPER, NOTE_D4, 250);

#ifdef DEBUG
    ZeroRegOptions opts = {Serial, false};
    printZeroRegs(opts);
#endif
}

uint32_t sleepTicks = 0;
uint16_t frameInputState = 0;
uint32_t currentTimeMs = 0;
uint32_t frameTimeMs = 0;
uint32_t prevFrameMs = 0;

uint32_t droppedFrames = 0;

bool dirtyFrameBuffer = false;
void loop(void)
{
    //// UPDATE ////
    frameInputState |= readButtons();

    currentTimeMs = millis();

    if (frameInputState)
    {
        nextSleepTime = currentTimeMs + 60000;
    }

    //// DRAW ////
    frameTimeMs = (currentTimeMs - prevFrameMs);
    if (frameTimeMs > k_frameSleepTimeMs)
    {
        prevFrameMs = currentTimeMs;

        GameState *nextState = currentState->update(frameInputState, k_tickTime);

        if (nextState != currentState)
        {
            delete currentState;
            currentState = nextState;
        }

        if (!display.isFrameLocked())
        {
            if (currentState->redraw)
            {
                display.fillDisplayBuffer();
                currentState->draw(&display);
                // drawOverlay(frameInputState);
                // dirtyFrameBuffer = true;
            }

            drawOverlay(frameInputState);
            dirtyFrameBuffer = true;

            if (dirtyFrameBuffer)
            {
                dirtyFrameBuffer = false;
                display.refresh();
            }
        }
        else if (currentState->redraw)
        {
            droppedFrames++;
        }

        frameInputState = 0;
    }

    //// SLEEP ////
    if (currentTimeMs > nextSleepTime)
    {
        display.sync();

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
        GameState *nextState = currentState->update(frameInputState, 1);
        if (nextState != currentState)
        {
            delete currentState;
            currentState = nextState;
        }
        currentState->redraw = true;

        //// SLEEP-DRAW ////
        if (!display.isFrameLocked())
        {
            if (currentState->redraw)
            {
                display.fillDisplayBuffer();
                currentState->draw(&display);
                drawOverlay(frameInputState);
                dirtyFrameBuffer = true;
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

void drawOverlay(uint16_t inputState)
{
    uint32_t elapsedTimeSeconds = currentTimeMs / 1000.0;
    // uint32_t hours = (elapsedTimeSeconds / 60 / 60);
    uint32_t minutes = (elapsedTimeSeconds / 60);
    uint32_t seconds = (elapsedTimeSeconds % 60);

    // text display tests
    display.setCursor(0, 0);
    display.setTextColor(PET_WHITE, PET_BLACK); // inverted text
    display.fillRect(0, 0, 64, 8, PET_BLACK);

    // char buf[PRINTF_BUF];
    display.printf("%02u:%02u ", minutes, seconds);
    if (nextSleepTime > millis())
    {
        // display.printf("%02u %02u", frameTimeMs, droppedFrames);
        // display.printf(" %02u\n", (nextSleepTime - millis()) / 1000);
    }
    else
    {
        display.printf("Zz\n");
    }

    int intBat = Util::batteryLevel();
    //display.printf(" %03u\n", intBat);

    // 1.5*2 alk
    // 1.4*2 nimh
    // -0.2 shottky
    // 260-280 full charge
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

    display.drawFrame(PETPIC(battery), 48, 0, batFrame);

    // display.drawFrame("battery", 48, 0, batFrame);
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

#ifdef FLASH_SETUP

DSTATUS disk_status(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    (void)pdrv;
    return 0;
}

DRESULT disk_read(
    BYTE pdrv,    /* Physical drive nmuber to identify the drive */
    BYTE *buff,   /* Data buffer to store read data */
    DWORD sector, /* Start sector in LBA */
    UINT count    /* Number of sectors to read */
)
{
    (void)pdrv;
    return flash.readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(
    BYTE pdrv,        /* Physical drive nmuber to identify the drive */
    const BYTE *buff, /* Data to be written */
    DWORD sector,     /* Start sector in LBA */
    UINT count        /* Number of sectors to write */
)
{
    (void)pdrv;
    return flash.writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(
    BYTE pdrv, /* Physical drive nmuber (0..) */
    BYTE cmd,  /* Control code */
    void *buff /* Buffer to send/receive control data */
)
{
    (void)pdrv;

    switch (cmd)
    {
    case CTRL_SYNC:
        flash.syncBlocks();
        return RES_OK;

    case GET_SECTOR_COUNT:
        *((DWORD *)buff) = flash.size() / 512;
        return RES_OK;

    case GET_SECTOR_SIZE:
        *((WORD *)buff) = 512;
        return RES_OK;

    case GET_BLOCK_SIZE:
        *((DWORD *)buff) = 8; // erase block size in units of sector size
        return RES_OK;

    default:
        return RES_PARERR;
    }
}

#endif