/*
 * DS3231.ino
 *
 * Created: 1/22/2020 3:30:31 PM
 * Author: colef
 */ 

#include "DS3231.h"
#include "Wire.h"

// DS3231 Clock;
RTClib RTC;
DateTime now;
boolean oneShotBits[32];			// oneshot bits available for use for oneshot or toggle calls


void setup()
{
	Wire.begin();
	Serial.begin(115200);
}

void loop()
{
	
	now = RTC.now();
	
	int nowYear = now.year();
	int nowMonth = now.month();
	int nowDay = now.day();
	int nowHour = now.hour();
	int nowMinute = now.minute();
	int nowSecond = now.second();
	
	if (oneShot(FlasherBit(1), 0))
	{
		Serial.println("Current time is " + String(now.year()) + "/" + String(now.month()) + "/" + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) );
	}
	
}

boolean FlasherBit(float hz)
{
	int T = round(1000.0 / hz);
	if ( millis() % T >= T/2 ) return 1;
	else return 0;
}

boolean oneShot(boolean precond, uint8_t OSR)
{
	// use global memory to keep track of oneshot bits
	if (precond == 1 && oneShotBits[OSR] == 0)
	{
		oneShotBits[OSR] = 1;
		return 1;
	}
	else if (precond == 0 && oneShotBits[OSR] == 1)
	{
		oneShotBits[OSR] = 0;
		return 0;
	}
	else return 0;
}