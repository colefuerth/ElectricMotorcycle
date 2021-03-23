
/*
    Name:       BIGBOY20200112.ino
    Created:	2020-01-12 7:41:47 PM
    Author:     COLE-VM\colefuerth
	
	DEMO PROGRAM FOR TESTING SOFTWARE AND COMMS
	
	Goals of this demo:
	- read potentiometer value as 0-100% on main and display on HMI, as well as on LEDs
	- Read inductive sensor value and forward value to HMI, as well as the relay
	- Button will toggle in/out of gear; status will be displayed on LED as well as make an icon appear/disappear on HMI for neutral/drive
	- Store in/out of gear status on sdcard; restore previous state when booting next time
	- do some kind of integral to keep track of current draw from motor in gear and track it in sdcard
	- have console logging for debug, as well as sdcard logging
	
	
	NOTES
	- add serial logic and monitoring system logic; most hardware logic from the standard is done as far as I can see
	- probably still some standard logic missing but thats ok ill add it as it pops up
	- might do logging of console and sdcard, and make logic so it sends a string as a parameter for a call, and decide in the call whether to send the string to console or not to avoid all the waiting for that shit
	- NEEDS ALARMS OF SOME KIND
	- should probably do an interface so I can tune everything from the HMI to make setting up 600 times faster
*/

// clock library
#include <Wire.h>
// sdcard libraries
#include <SPI.h>
#include <SD.h>

#include <DS3231.h>		// RTC library

//#include "SerialTransfer-master/SerialTransfer.h"
//SerialTransfer myTransfer;

// IO variables
int potentiometer; 
int throttlePercent;
boolean leds[8];
boolean gearInd;
int transmissionBuffer[10];  
boolean proxStatus;
boolean buttonInput;
boolean button1;

// debounce variables:
boolean inGearButton, button1mem;
int button1Debounce;

// modes variables
boolean inGear, inGearONS;

// monitorSystem variables
int currentDraw, lastDrawUpdate;
float totalDraw_mAh;
int watchdog, last_cycleStart, longestCycle, fastestCycle;
// int watchdog_Buffer[20];		//zzz	will be used for debug later

// ClockRoutine variables
int lastClockRefreshTime;
RTClib RTC;
DateTime now;

// declare pin values
const int inGearIndPin = 17;
const int ledPins[8] = {15,14,2,3,4,5,6,7};
const int potentiometerPin = PIN_A0;
const int relayOutput = 52;
const int proxPin = 53;
const int button1Pin = 21;




void setup()
{
	
	Serial.begin(9600);	// computer comms
	Serial.write("serial communication initiated");
/*
	if (Serial.write_error())
	{
		logging("serial 0 communication init failed, disabling serial comms");	//zzz maybe create logic to turn off attempts to log to serial if serial fails?
		Serial.end();
	}
*/
	// fetch the current date and time before any other functions, for logging:
	Wire.begin();
	Serial.println("Wire initialized");
	lastClockRefreshTime = millis();
	now = RTC.now();
	// Serial.println
	
	pinMode(inGearIndPin, OUTPUT);
	
	for (int i=0; i<8; i++)
	{
		pinMode(ledPins[i], OUTPUT);
	}
	
	// reset transmission buffer
	for (int i=0; i>10; i++)	
	{
		transmissionBuffer[i] = 0;
	}
	
	// set initial modes
	inGearONS = 0;
	inGear = 0;
	
	// reset monitor system for session
	totalDraw_mAh = 0;
	fastestCycle = 1000;
	longestCycle = 0;

}

void loop()
{
	// call all functions in a loop here
	MapInputs();
	Modes();
	MonitorSystem();
	SDCard();
	SerialComms();
	MapOutputs();
	
	delay(5);	// for stability in a low demand program
}

void MapInputs()
{
	int currentTime = millis();		// temp variable to check the time only once at the beginning of any call that needs it
	
	proxStatus = digitalRead(proxPin);
	potentiometer = analogRead(potentiometerPin);
	
	
	// input from button1
	button1 = !digitalRead(button1Pin);	// pullup, so inverting input
	
	// debounce button1 and output debounced bit to inGearButton
	if (button1 != button1mem){
		button1mem = button1;
		button1Debounce = currentTime;
	}
	if (currentTime - button1Debounce > 20) inGearButton = button1;
}

void Modes()
{
	// toggle logic for in/out of gear
	if (inGearButton == 1 && inGearONS == 0){
		inGear = !inGear;
		inGearONS = 1;
	}
	else if (inGearButton == 0 && inGearONS == 1) inGearONS = 0;
	else;
}

void MonitorSystem()
{
	int currentTime = millis();	// for cycle speed
	
	// reset watchdog timer
	watchdog = currentTime - last_cycleStart;
	last_cycleStart = currentTime;

	// check for fastest/longest cycle
	if (watchdog > longestCycle) longestCycle = watchdog;
	if (watchdog < fastestCycle) fastestCycle = watchdog;
	
	// add logic for monitoring cycles per second
	throttlePercent = map(potentiometer, 0, 1023, 0, 100);		// integer 0-100 for throttle position
	float currentPercent = pow((float)throttlePercent / 100, 2);// returns a float 0-1 with a quadratic curve for currentPercent
	float j = currentPercent * 200;								// stores the float value for current draw in a temp variable
	currentDraw = (int)currentPercent;							// converts temporary current draw to an integer value, 0-200A draw
	
	// need to make an integral to keep track of total draw in mAh
	// all calculations will be done with floats for precision
	// draw in mAh to be added is draw * 0.2778 = (mAh in 0.5s at draw rate)
	if (currentTime - lastDrawUpdate >= 500)	// update total draw at 2Hz
	{
		totalDraw_mAh += j * 0.2778;
		lastDrawUpdate = currentTime;
	}
	
	
}

void ClockRoutine()
{
	
	int currentTime = millis();
	
	if (currentTime - lastClockRefreshTime >= 1000){		// refresh date and time every second
		
		lastClockRefreshTime = currentTime;
		now = RTC.now();
		
		
		/*		// for now I'lll just use now.year(), etc
		nowYear = now.year();
		nowMonth = now.month();
		nowDay = now.day();
		nowHour = now.hour();
		nowMinute = now.minute();
		nowSecond = now.second();
		*/
	}
	
}

void SDCard()
{
	
}

void SerialComms()
{
	// serial communications between master and HMI CPUs are done here
	
	
	
	
}

void MapOutputs()
{
	// led outputs:
	for (int i=0; i<8; i++)
	{
		digitalWrite(leds[i], ledPins[i]);
	}
	
	// display in gear status to output pin
	digitalWrite(inGear, inGearIndPin);
}


// function calls
void logging(String input_data)
{
	Serial.println(input_data);
	// add logic to store events in SDCard
}