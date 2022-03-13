
#include "gamestate.h"

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
