
#include "gamestate.h"
#include "install/ff.h"
#include "install/diskio.h"

class InstallUtils
{
  public:
    static void SetFuses(bool bod33Enable, uint32_t bootProtPages)
    {
        uint32_t userWord0 = *((uint32_t *)NVMCTRL_USER);       // Read fuses for user word 0
        uint32_t userWord1 = *((uint32_t *)(NVMCTRL_USER + 4)); // Read fuses for user word 1
        NVMCTRL->CTRLB.bit.CACHEDIS = 1;                        // Disable the cache
        NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2;           // Set the address
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_EAR |            // Erase the auxiliary user page row
                             NVMCTRL_CTRLA_CMDEX_KEY;

        while (!NVMCTRL->INTFLAG.bit.READY) {}        // Wait for the NVM command to complete
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;   // Clear the error flags
        NVMCTRL->ADDR.reg = NVMCTRL_AUX0_ADDRESS / 2; // Set the address
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_PBC |  // Clear the page buffer
                             NVMCTRL_CTRLA_CMDEX_KEY;

        while (!NVMCTRL->INTFLAG.bit.READY) {}      // Wait for the NVM command to complete
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK; // Clear the error flags

        if(bod33Enable)
        {
          userWord0 &= ~FUSES_BOD33_EN_Msk;
        }
        else
        {
          userWord0 |= FUSES_BOD33_EN_Msk;
        }

        userWord0 &= ~NVMCTRL_FUSES_BOOTPROT_Msk;
        userWord0 |= NVMCTRL_FUSES_BOOTPROT(bootProtPages);

        *((uint32_t *)NVMCTRL_USER) = userWord0;
        *((uint32_t *)(NVMCTRL_USER + 4)) = userWord1; // Copy back user word 1 unchanged
        NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_WAP |   // Write to the user page
                             NVMCTRL_CTRLA_CMDEX_KEY;

        while (!NVMCTRL->INTFLAG.bit.READY) {}      // Wait for the NVM command to complete
        NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK; // Clear the error flags
        NVMCTRL->CTRLB.bit.CACHEDIS = 0;            // Enable the cache
    }

    static FRESULT format()
    {
        FATFS elmchamFatfs;
        uint8_t workbuf[4096];

        // Make filesystem.
        FRESULT r = f_mkfs("", FM_FAT | FM_SFD, 0, workbuf, sizeof(workbuf));
        if (r != FR_OK)
        {
            return r;
        }

        // mount to set disk label
        r = f_mount(&elmchamFatfs, "0:", 1);
        if (r != FR_OK)
        {
            return r;
        }

        // Setting label
        r = f_setlabel("PAWPET");
        if (r != FR_OK)
        {
            return r;
        }

        // unmount
        f_unmount("0:");

        // sync to make sure all data is written to flash
        g::g_flash->syncBlocks();

        // Check new filesystem
        g::g_stats.filesysFound = g::g_fatfs->begin(g::g_flash);

        return r;
    }
};

class InstallMenu : public GameState
{
  public:
    InstallMenu() : GameState(1)
    {
    }

    int8_t index = 0;
    const char *items[5] = {"buttons", "flash", "fuses", "battery", "stats"};

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class ButtonTest : public GameState
{
  public:
    ButtonTest() : GameState(2)
    {
        tick = k_tickTime5;
    }

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class FlashFormat : public GameState
{
  public:
    FlashFormat() : GameState(3)
    {
        resultCode = 0;
    }

    uint32_t resultCode;

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class FusesState : public GameState
{
  public:
    FusesState() : GameState(4)
    {
    }

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class SuspendTest : public GameState
{
  public:
    SuspendTest() : GameState(5)
    {
    }

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class BatteryTest : public GameState
{
  public:
    BatteryTest() : GameState(6)
    {
    }

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};
