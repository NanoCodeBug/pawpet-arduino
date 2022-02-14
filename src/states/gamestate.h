#pragma once
#include "../common.h"
#include "../global.h"

#include "../graphics/display.h"
#include "../graphics/graphics.h"
#include "../sounds/pitches.h"

struct Point2D
{
    int16_t x;
    int16_t y;
};

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
    const char *items[6] = {"game", "animate", "sleep", "music", "stats", "suspend"};

    GameState *update() override;
    void draw(PetDisplay *disp) override;
};

class TestGame1 : public GameState
{
  public:
    uint8_t score;

    Point2D fallingObjs[8];
    uint8_t paddleX;
    PetSprite _icons;

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
    PetAnimation petSit;
    // uint32_t* testImage;
    // uint16_t* testMeta;

    GameState *update() override;

    void draw(PetDisplay *disp) override;
};

class SleepScreen : public GameState
{
  public:
    SleepScreen();

    PetSprite pm;

    GameState *update() override;

    void draw(PetDisplay *disp) override;
};

class ToneScreen : public GameState
{
  public:
    ToneScreen();

    GameState *update() override;

    void draw(PetDisplay *disp) override;

    int notes;
    int currentNote;
    int noteDuration;
    int melody[120];
};