
#include "pawos.h"

#include <chrono>

PetDisplay display(nullptr, 0, DISP_WIDTH, DISP_HEIGHT);

// used by usb and button interrupts to keep the device awake when in use
volatile bool buttonWakeup;
volatile uint32_t nextSleepTime;

// button reading and interrupts
void buttonWakeupCallback();
// uint8_t readButtons();

// gamestate and ui
bool drawTimeAndBattery();
GameState *currentState;

// USB Mass Storage object functions

uint16_t intBat = 300;
WatchdogSAMD Watchdog;

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

uint32_t startTime = 0;

void init()
{
    auto millisec_since_epoch = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();

    startTime = (uint32_t) millisec_since_epoch;

    currentState = new AnimationTest();
    display.begin();

    display.fillDisplayBuffer();
    display.setCursor(0, 8);
    display.setTextColor(PET_BLACK);
    currentState->draw(&display);


    nextSleepTime = 40000;
}

uint32_t millis()
{
    auto millisec_since_epoch =  std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    return (uint32_t)millisec_since_epoch - startTime;
}

void LogMsg(const char *msg)
{
    uint32_t uptime = 0;//(g::g_rtc.getEpoch() - g::g_stats.bootTime);
    uint32_t ontime = millis() / 1000 / 60;
    uint16_t bat = Util::batteryLevel();

    // message, uptime, ontime, voltage, buttonWakeup
    printf("%s, %d, %d, %d, %d\n", msg, uptime, ontime, bat, buttonWakeup);
}


void loop(void)
{
    //// UPDATE ////
    // keysState |= readButtons();

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

        buttonWakeup = false;
        // TODO: wakeup on flash chip not being triggered correctly
        // BUG: on reset after sleep flash doesn't show up until second reset
        // flashTransport.runCommand(0xB9); // deep sleep

        // LowPower.deepSleep(k_sleepTimeMs);
        Watchdog.sleep(WatchdogSAMD::WATCHDOG_TIMER_64_S);

        // technically incriments on button wakeup as well, but this is fine
        sleepTicks++;

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

PetSprite _battery(battery);
bool drawTimeAndBattery()
{
    uint32_t elapsedTimeSeconds = currentTimeMs / 1000;
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
    // {
    //     uint32_t t = (g::g_rtc.getEpoch() - g::g_stats.bootTime);

    //     logFile.printf("b: %d\n", t);
    //     logFile.sync();
    // }
}
