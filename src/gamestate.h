#pragma once
#include "common.h"
#include "global.h"

enum ButtonFlags
{
	BUTTON_A = 0x1,
	BUTTON_B = 0x2,
	BUTTON_C = 0x4,
	BUTTON_P = 0x8,

	UP = 0x10,
	DOWN = 0x20,
	LEFT = 0x40,
	RIGHT = 0x80,

	END = 0x100
};

extern "C" char *sbrk(int i);

class Util
{
public:
	inline static bool OnKeyUp(uint32_t prevInput, uint32_t input, ButtonFlags key)
	{
		return !(prevInput & key) && (input & key);
	}

	inline static int batteryLevel()
	{
		while (ADC->STATUS.bit.SYNCBUSY == 1);
		ADC->INPUTCTRL.bit.GAIN = ADC_INPUTCTRL_GAIN_DIV2_Val;      // Gain Factor Selection
		ADC->REFCTRL.bit.REFSEL = ADC_REFCTRL_REFSEL_INT1V_Val;   // 1.0V voltage reference

		float vbat = analogRead(PIN_VBAT);
		//return (int)vbat;
		vbat *= 2;		// we divided by 2, so multiply back
		vbat *= 2.0;	// Multiply by 2.0V, our reference voltage
		vbat /= 4096.0; // convert to voltage
		return vbat * 100;
	}

	static int32_t FreeRam()
	{
		char stack_dummy = 0;
		return &stack_dummy - sbrk(0);
	}
};

struct Point2D
{
	int16_t x;
	int16_t y;
};

class GameState
{
public:
	uint32_t id = 0;
	bool redraw = true;
	uint32_t prevInput;

	virtual ~GameState() {};

	virtual GameState *update(uint32_t input, uint32_t tick) = 0;
	virtual void draw(PetDisplay *disp) = 0;
};

class MenuState : public GameState
{
public:
	MenuState()
	{
		id = 1;
	}
	int8_t index = 0;
	const char *items[6] = {"Game 1", "item 2", "item 3", "item 4", "stats", "suspend"};

	GameState *update(uint32_t input, uint32_t tick) override;

	void draw(PetDisplay *disp) override
	{
		if (!redraw)
		{
			return;
		}
		redraw = false;

		disp->setCursor(0, 8);
		disp->setTextSize(1);

		for (uint8_t s = 0; s < 6; s++)
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
};

class TestGame1 : public GameState
{
public:
	uint8_t score;

	Point2D fallingObjs[8];
	uint8_t paddleX;

	TestGame1()
	{
		id = 2;
		prevInput = 0;

		score = 0;
		paddleX = 0;

		for (int s = 0; s < 8; s++)
		{
			fallingObjs[s].x = random(0, 8);

			fallingObjs[s].y = -random(0, 32);
		}
	}

	GameState *update(uint32_t input, uint32_t tick) override
	{
		if (Util::OnKeyUp(prevInput, input, LEFT) && paddleX > 0)
		{
			paddleX--;
		}
		else if (Util::OnKeyUp(prevInput, input, RIGHT) && paddleX < 7)
		{
			paddleX++;
		}
		else if (Util::OnKeyUp(prevInput, input, BUTTON_A))
		{
			return new MenuState();
		}

		for (int s = 0; s < 8; s++)
		{
			fallingObjs[s].y += 4;

			if (fallingObjs[s].y > DISP_HEIGHT)
			{

				fallingObjs[s].y = 0;
				fallingObjs[s].x = random(0, 8);
			}
		}

		prevInput = input;
		redraw = true;

		return this;
	}

	void draw(PetDisplay *disp) override
	{
		if (!redraw)
		{
			return;
		}
		redraw = false;

		disp->setTextSize(1);
		disp->println("test game 1");
		for (int s = 0; s < 8; s++)
		{
			disp->drawFrame(PETPIC(icons), fallingObjs[s].x * 8, fallingObjs[s].y, 0);
		}

		disp->drawFrame(PETPIC(icons), paddleX * 8, 64, 11);
		disp->drawFrame(PETPIC(icons), (paddleX + 1) * 8, 64, 11);
	}
};

class StatsState : public GameState
{
public:
	StatsState()
	{
		id = 3;
		prevInput = 0;

		// g::g_cache->LoadGraphic("testface");
		// g::g_cache->GetGraphic("testface", &testMeta, &testImage);
	}
	uint16_t refreshTime = 0;

	// uint32_t* testImage;
	// uint16_t* testMeta;

	GameState *update(uint32_t input, uint32_t tick) override
	{
		if (Util::OnKeyUp(prevInput, input, BUTTON_A))
		{
			return new MenuState();
		}

		refreshTime += tick;
		if (refreshTime > 500)
		{
			redraw = true;
		}

		prevInput = input;
		return this;
	}

	void draw(PetDisplay *disp) override
	{
		disp->setCursor(0, 8);
		disp->setTextColor(PET_BLACK);
		
		int32_t ramUsage = ((int32_t)RAM_SIZE - Util::FreeRam()) * 100 / RAM_SIZE;
		disp->printf(" %02u%% %3uv\n", ramUsage, Util::batteryLevel());
		disp->printf(" %uMB %s\n", g::stats.flashSize/1024/1024, g::stats.filesysFound ? "FAT" : "N/A");
		disp->printf(" ver:%s\n", SOFT_VERSION);
	}
};


class AnimationTest : public GameState
{
public:
	AnimationTest()
	{
		id = 3;
		prevInput = 0;

		// g::g_cache->LoadGraphic("testface");
		// g::g_cache->GetGraphic("testface", &testMeta, &testImage);
	}
	uint16_t refreshTime = 0;

	// uint32_t* testImage;
	// uint16_t* testMeta;

	GameState *update(uint32_t input, uint32_t tick) override
	{
		if (Util::OnKeyUp(prevInput, input, BUTTON_A))
		{
			return new MenuState();
		}

		refreshTime += tick;
		if (refreshTime > 500)
		{
			redraw = true;
		}

		prevInput = input;
		return this;
	}

	void draw(PetDisplay *disp) override
	{
		disp->setCursor(0, 8);
		disp->setTextColor(PET_BLACK);


		disp->drawImage(PETPIC(sleeptest), 0, 0);
		// disp->drawImage(testImage, testMeta, 20, 30);
		// disp->drawImage(testImage, testMeta, 0, 8 );

	}
};

GameState *MenuState::update(uint32_t input, uint32_t tick)
{
	if (Util::OnKeyUp(prevInput, input, DOWN) && index < 5)
	{
		index++;
		redraw = true;
	}
	else if (Util::OnKeyUp(prevInput, input, UP) && index > 0)
	{
		index--;
		redraw = true;
	}
	else if (Util::OnKeyUp(prevInput, input, BUTTON_P))
	{
		switch (index)
		{
		case 0:
			return new TestGame1();
			break;

		case 4:
			return new StatsState();
			break;

		case 1: 
			return new AnimationTest();
		default:
			break;
		}
	}

	prevInput = input;
	return this;
}