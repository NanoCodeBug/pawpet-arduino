#ifndef _ARDUINO_LOW_POWER_H_
#define _ARDUINO_LOW_POWER_H_

#include <Arduino.h>
#include "RTCZero.h"

#define RTC_ALARM_WAKEUP 0xFF

typedef void (*onOffFuncPtr)(bool);

typedef enum
{
	OTHER_WAKEUP = 0,
	GPIO_WAKEUP = 1,
	NFC_WAKEUP = 2,
	ANALOG_COMPARATOR_WAKEUP = 3
} wakeup_reason;

class ArduinoLowPowerClass
{
public:
	void idle(void);
	void idle(uint32_t millis);
	void idle(int millis)
	{
		idle((uint32_t)millis);
	}

	void sleep(void);
	void sleep(uint32_t millis);
	void sleep(int millis)
	{
		sleep((uint32_t)millis);
	}

	void deepSleep(void);
	void deepSleep(uint32_t millis);
	void deepSleep(int millis)
	{
		deepSleep((uint32_t)millis);
	}

	void attachInterruptWakeup(uint32_t pin, voidFuncPtr callback, uint32_t mode);

private:
	void setAlarmIn(uint32_t millis);
	RTCZero rtc;
};

extern ArduinoLowPowerClass LowPower;

#endif
