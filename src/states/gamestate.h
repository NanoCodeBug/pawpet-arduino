#pragma once
#include "../common.h"
#include "../global.h"

#include "../graphics/display.h"
#include "../graphics/graphics.h"
#include "../sounds/pitches.h"

class GameState
{
  public:
    const uint8_t id;             // state id for requesting next state?
    bool redraw = true;           // redraw of screen requsted
    uint32_t tick = k_tickTime30; // requested tick time of state

    GameState(uint32_t sid) : id(sid){};

    virtual ~GameState(){};

    virtual GameState *update() = 0;
    virtual void draw(PetDisplay *disp) = 0;
};

class MenuState : public GameState
{
  public:
    MenuState() : GameState(1)
    {
    }

    int8_t index = 0;
    const char *items[6] = {"Game 1", "anim 1", "item 3", "item 4", "stats", "suspend"};

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class TestGame1 : public GameState
{
  public:
    uint8_t score;

    Point2D fallingObjs[8];
    uint8_t paddleX;

    TestGame1();

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class StatsState : public GameState
{
  public:
    StatsState();

    GameState *update() override;

    void draw(PetDisplay *disp) override;
};

class AnimationTest : public GameState
{
  public:
    AnimationTest();

    uint32_t curFrame;
    uint32_t curTick;
    bool dir;
    // uint32_t* testImage;
    // uint16_t* testMeta;

    GameState *update() override;

    void draw(PetDisplay *disp) override;
};

class SleepScreen : public GameState
{
  public:
    SleepScreen();

    GameState *update() override;

    void draw(PetDisplay *disp) override;
};