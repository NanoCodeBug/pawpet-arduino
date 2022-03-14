#include "pawos_install.h"
#include "../lib/PawPet_SleepyDog.h"

GameState *InstallMenu::update()
{
    if (g::g_keyReleased & DOWN && index < 5)
    {
        index++;
        redraw = true;
    }
    else if (g::g_keyReleased & UP && index > 0)
    {
        index--;
        redraw = true;
    }
    else if (g::g_keyReleased & BUTTON_P)
    {
        switch (index)
        {
        case 0:
            return new ButtonTest();
            break;

        case 1:
            return new FlashFormat();
            break;

        case 2:
            return new FusesState();
            break;

        case 3:
            return new BatteryTest();
            break;

        case 4:
            return new SuspendTest();
            break;

        default:
            break;
        }
    }

    return this;
}

void InstallMenu::draw(PetDisplay *disp)
{
    if (!redraw)
    {
        return;
    }
    redraw = false;

    disp->setCursor(0, 8);
    disp->setTextSize(1);

    for (uint8_t s = 0; s < 5; s++)
    {
        if (index == s)
        {
            disp->setTextColor(PET_WHITE, PET_BLACK);
        }
        else
        {
            disp->setTextColor(PET_BLACK);
        }

        disp->println(items[s]);
    }
}

GameState *BatteryTest::update()
{
    if (g::g_keyReleased & BUTTON_A)
    {
        return new InstallMenu();
    }

    redraw = true;

    return this;
}

void BatteryTest::draw(PetDisplay *disp)
{
    disp->printf("bat: %3uv\n", Util::batteryLevel());
    uint8_t cause = Watchdog.resetCause();
    disp->printf("res:");
    switch (cause)
    {
    case 0x1:
        disp->printf(" %s", "POR");
        break;
    case 0x2:
        disp->printf(" %s", "BOD12");
        break;
    case 0x4:
        disp->printf(" %s", "BOD33");
        break;
    case 0x8:
        disp->printf(" %s", "bit4");
        break;
    case 0x10:
        disp->printf(" %s", "EXT");
        break;
    case 0x20:
        disp->printf(" %s", "WDT");
        break;
    case 0x40:
        disp->printf(" %s", "SYST");
        break;
    case 0x80:
        disp->printf(" %s", "bit8");
        break;
    }
}

GameState *ButtonTest::update()
{
    if (g::g_keyHeld & BUTTON_A && g::g_keyHeld & LEFT)
    {
        return new InstallMenu();
    }
    redraw = true;

    if (g::g_keyPressed > 0)
    {
        tone(PIN_BEEPER, NOTE_C4, 200);
    }
    return this;
}

void ButtonTest::draw(PetDisplay *disp)
{
    disp->setCursor(0, 8);
    disp->setTextColor(PET_BLACK);
    disp->printf("  ^>v<abcp\n");
    disp->printf("u:%d%d%d%d%d%d%d%d\n", (g::g_keyPressed & UP) > 0, (g::g_keyPressed & RIGHT) > 0,
                 (g::g_keyPressed & DOWN) > 0, (g::g_keyPressed & LEFT) > 0, (g::g_keyPressed & BUTTON_A) > 0,
                 (g::g_keyPressed & BUTTON_B) > 0, (g::g_keyPressed & BUTTON_C) > 0, (g::g_keyPressed & BUTTON_P) > 0);
    disp->printf("h:%d%d%d%d%d%d%d%d\n", (g::g_keyHeld & UP) > 0, (g::g_keyHeld & RIGHT) > 0, (g::g_keyHeld & DOWN) > 0,
                 (g::g_keyHeld & LEFT) > 0, (g::g_keyHeld & BUTTON_A) > 0, (g::g_keyHeld & BUTTON_B) > 0,
                 (g::g_keyHeld & BUTTON_C) > 0, (g::g_keyHeld & BUTTON_P) > 0);
    disp->printf("r:%d%d%d%d%d%d%d%d\n", (g::g_keyReleased & UP) > 0, (g::g_keyReleased & RIGHT) > 0,
                 (g::g_keyReleased & DOWN) > 0, (g::g_keyReleased & LEFT) > 0, (g::g_keyReleased & BUTTON_A) > 0,
                 (g::g_keyReleased & BUTTON_B) > 0, (g::g_keyReleased & BUTTON_C) > 0,
                 (g::g_keyReleased & BUTTON_P) > 0);

    disp->printf("a + left\nto quit\n");
}

GameState *FlashFormat::update()
{
    if (g::g_keyReleased & BUTTON_A)
    {
        return new InstallMenu();
    }
    redraw = true;

    // if filesystem not present, and no previous error code, format
    // otherwise wait for forced format command
    if ((g::g_stats.filesysFound == false && resultCode == 0) || (g::g_keyHeld & BUTTON_P && g::g_keyHeld & UP))
    {
        resultCode = InstallUtils::format();
    }

    return this;
}

void FlashFormat::draw(PetDisplay *disp)
{
    disp->printf("size: %uMB\n", g::g_stats.flashSize / 1024 / 1024);
    disp->printf("fs: %s\n", g::g_stats.filesysFound ? "FAT" : "N/A");
    disp->printf("res: %d\n", resultCode);
    disp->printf("\nup + p\n");
    disp->printf("to format\n");
}

GameState *FusesState::update()
{
    if (g::g_keyReleased & BUTTON_A)
    {
        return new InstallMenu();
    }

    if (g::g_keyHeld & BUTTON_P && g::g_keyHeld & UP)
    {
        InstallUtils::SetFuses(true, 0x2);
    }

    if (g::g_keyHeld & BUTTON_P && g::g_keyHeld & DOWN)
    {
        InstallUtils::SetFuses(false, 0x0);
    }

    redraw = true;
    return this;
}

void FusesState::draw(PetDisplay *disp)
{
    uint32_t userWord0 = *((uint32_t *)NVMCTRL_USER);
    uint32_t userWord1 = *((uint32_t *)(NVMCTRL_USER + 4)); 

    disp->printf("  Fuses\n");
    disp->printf("BTPROT: %d\n", (userWord0 & NVMCTRL_FUSES_BOOTPROT_Msk) >> NVMCTRL_FUSES_BOOTPROT_Pos);
    disp->printf("BOD33: %d\n", (userWord0 & FUSES_BOD33_EN_Msk) > 0);

    disp->printf("%x\n%x\n", userWord0, userWord1);
    disp->printf("^ + p\n");
    disp->printf("V + p\n");
}

GameState *SuspendTest::update()
{
    if (g::g_keyReleased & BUTTON_A)
    {
        return new InstallMenu();
    }

    return this;
}

void SuspendTest::draw(PetDisplay *disp)
{
    uint32_t minutes = (g::g_rtc.getEpoch() - g::g_stats.bootTime) / 60;
    disp->printf("uptime:\n %um\n", minutes);
    disp->printf("build:\n %s\n", BUILD_STRING);

    disp->printf("bat: %3uv\n", Util::batteryLevel());
    uint8_t cause = Watchdog.resetCause();
    disp->printf("res:");
    switch (cause)
    {
    case 0x1:
        disp->printf(" %s", "POR");
        break;
    case 0x2:
        disp->printf(" %s", "BOD12");
        break;
    case 0x4:
        disp->printf(" %s", "BOD33");
        break;
    case 0x8:
        disp->printf(" %s", "bit4");
        break;
    case 0x10:
        disp->printf(" %s", "EXT");
        break;
    case 0x20:
        disp->printf(" %s", "WDT");
        break;
    case 0x40:
        disp->printf(" %s", "SYST");
        break;
    case 0x80:
        disp->printf(" %s", "bit8");
        break;
    }
}

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

DRESULT disk_read(BYTE pdrv,    /* Physical drive nmuber to identify the drive */
                  BYTE *buff,   /* Data buffer to store read data */
                  DWORD sector, /* Start sector in LBA */
                  UINT count    /* Number of sectors to read */
)
{
    (void)pdrv;
    return g::g_flash->readBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv,        /* Physical drive nmuber to identify the drive */
                   const BYTE *buff, /* Data to be written */
                   DWORD sector,     /* Start sector in LBA */
                   UINT count        /* Number of sectors to write */
)
{
    (void)pdrv;
    return g::g_flash->writeBlocks(sector, buff, count) ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, /* Physical drive nmuber (0..) */
                   BYTE cmd,  /* Control code */
                   void *buff /* Buffer to send/receive control data */
)
{
    (void)pdrv;

    switch (cmd)
    {
    case CTRL_SYNC:
        g::g_flash->syncBlocks();
        return RES_OK;

    case GET_SECTOR_COUNT:
        *((DWORD *)buff) = g::g_flash->size() / 512;
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