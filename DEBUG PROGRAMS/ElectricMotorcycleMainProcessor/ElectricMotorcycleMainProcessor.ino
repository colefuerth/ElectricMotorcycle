/*
 * ElectricMotorcycleMainProcessor.ino
 *
 * Created: 1/30/2020 1:39:49 PM
 * Author: cole_fuerth
 * Revision: Alpha 0.8.0	April 2020
 * Project: Control System for an Electric Motorcycle
 *
 * THIS SKETCH IS DESIGNED TO BE RUN ON AN ATMEGA 2560
 *
 */ 

/*
CHANGELOG
0.8.0	April 8, 2020
 - Added a fault for unrecognized serial message from Nextion

0.7.2	March 29, 2020
 - Nextion now only cycles through updates on the current screen in its cycle, so elements update much faster
 - voltsFromAnalogIn() was fixed, had a resolution problem and would only output integers
 - Nextion now sets its own baud to 115200 at startup
 - fixed some pointers not having an *asterisk
 - calibrated analog input values
 - analog boards were changed from 5v to 12v rails, and 5v safety diodes were added to protect the arduino
 - fixed current values on STAT page not updating
 - added throttle in and out bars to the stat page
 - added analog debugging settings
 - added fault debugging settings
 - fixed toggle logic
 - Fixed 'throttle out of bounds' fault always being on

0.7.1	March 27, 2020
 - Fixed button presses not being recognized
 - Some elements have become instant update rather than periodic
 - calibrated cell voltages
 - fixed speed logic

0.7.0	March 26, 2020
 - added a settings page to HMI with performance options
 - added indicators to main screen on HMI for brights and fog light indicators
 - added logic for updating HMI buffers to optimize HMI writing
 - added logic so that the 50hz update on elements only cycles through the current page, not all pages
 - added functionality so that the alarmView window changes colors based on the presence of faults
 - alarmView windows now flash red and gray when any critical faults are present
*/


/*  TODO
- Throttle to update on ISR instead of routinely
- Comment code more heavily
- Finish CLI logic
*/


#include <DS3231.h>
#include <Wire.h>
#include <Nextion.h>	// I have my own logic for this now
#include <Servo.h>
#include <Nextion.h>	

//define some color values for Nextion
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define ALRMRED	0xFC10
#define GREY	0x8410
#define GREEN   0x07E0
#define CYAN    0x07FF
#define LBLUE	0xAEBF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF
#define NEX_RET_EVENT_TOUCH_HEAD 0x65
#define nexSerial Serial2
#define dbSerial Serial

// function call memory bits available
	boolean oneShotBits[64];			// oneshot bits available for use for oneshot or toggle calls
										// BITS 32-63 ARE FOR FAULTS ONLY!!	
	
	uint8_t ONSTracker;
	// uint8_t debounceTracker;
	
	boolean timerInSession[32];		// for speed, so we only update timer timers when needed
	boolean timerMemory[sizeof(timerInSession)];			// make function calls smaller by remembering previous output state
	unsigned long timerTimers[sizeof(timerInSession)];	// debounce timers available for use
	uint8_t timerTracker;

// declare opto input arrays and input pins
	boolean opto1[8], opto2[8];
	const uint8_t opto1pins[8] = {52,50,48,46,44,42,40,38};
	const uint8_t opto2pins[8] = {36,34,32,30,28,26,24,22};
	
	boolean *killswitch				= opto2 ,
			*fogLightsIn			= opto2 + 1,
			*lightsOn				= opto2 + 2,
			*startPb				= opto2 + 3,
			*frontBrake				= opto2 + 4,
			*rearBrake				= opto2 + 5,
			*handlebarPowerRight	= opto2 + 6;
		
	boolean *rightTurnInput			= opto1 ,
			*leftTurnInput			= opto1 + 1,
			*lowBeamInput			= opto1 + 2,
			*highBeamInput			= opto1 + 3,
			*hornInput				= opto1 + 4,
			*clutchSwitchInput		= opto1 + 5,	
			*handlebarPowerLeft		= opto1 + 6,
			*optoPower				= opto1 + 7;
			
			
	boolean speedoPXInput;
	uint8_t speedoPXInputPin		= 10;
		
	
// declare analog inputs and pins
	const uint8_t auxPowerPin = 11;
	uint8_t analogInputTracker = 1;		// refresh one at a time, at 25Hz, except A0 is read at 25Hz on its own
	const uint8_t analogInputPins[8] = {PIN_A0,PIN_A9,PIN_A10,PIN_A11,PIN_A12,PIN_A13,PIN_A14,PIN_A15};
	int analogInputs[8];
	int	*throttleInput			= analogInputs,
		*currentDrawInput		= analogInputs + 1,
		*motorDrawInput			= analogInputs + 2,
		*vBatInput				= analogInputs + 3,
		*cellVoltageRawIn[4]	= { analogInputs+4,	analogInputs+5,
									analogInputs+6,analogInputs+7 };

// declare relay arrays and pointers
	boolean relayR1[6] = {0,0,0,0,0,0};	
	boolean relayF2[6] = {0,0,0,0,0,0};
	boolean relayM3[4] = {0,0,0,0};
	/*	note that the pins before and after each array are used for 
		power and ground for optocouplers on the relay inputs	*/
	const uint8_t relayR1Pins[6] = {41,43,45,47,49,51};
	const uint8_t relayF2Pins[6] = {35,33,31,29,27,25};
	const uint8_t relayM3Pins[4] = {3,4,5,6};	
		
	boolean *brakeLowOutput		= relayR1 + 0,
			*brakeHighOutput	= relayR1 + 1,
			*RRTurn				= relayR1 + 2,
			*RLTurn				= relayR1 + 3;
	
	boolean *highBeamsOut		= relayF2 + 0,
			*lowBeamsOut		= relayF2 + 1,
			*FRTurn				= relayF2 + 2, 
			*FLTurn				= relayF2 + 3,
			*hornOutput			= relayF2 + 4,
			*fogLightsOut		= relayF2 + 5;
			
	boolean *ESCSolenoid		= relayM3 + 0,
			*speedHighOut		= relayM3 + 1,
			*speedLowOut		= relayM3 + 2;
	
	
// Modes
	boolean inGear;
	boolean exitingGear = 0;

// clockRoutine Variables
	RTClib RTC;
	DateTime now;
	
// DriveSystem variables
	int throttlePercent;
	boolean killswOffSinceBoot = 0;
	boolean sportMode = 0,
			standardMode = 1,
			economyMode = 0;
	
// monitor system variables
	long last_cycleStart, totalDraw_mAh, last_analogCycleComplete, last_cycleStartus;
	int watchdog, longestCycle = 0, fastestCycle = 1000, watchdogus;
	int currentDrawMotorAmps, currentDrawShuntmA, speed, speedCount;
	int totalInGearSeconds = 0;
	float cellVolts[4], vBat;
	
	
// misc IO Variables, not done in the analog block
	int throttleOutput, pwmThrottleOutput;
	const uint8_t throttleOutputPin = 13;
	const uint8_t zzpwmOutputPin = 12;
	Servo zzpwmMotor;
	
	/*	NOT YET WORKING OR IMPLEMENTED
// SDCARD STUFF
	#include <SPI.h>
	#include <SD.h>
	File myFile;
	boolean SDActive, USBActive;
	uint8_t SDPin = 8;
	String logFileName;
	
// EEPROM values
	uint8_t	eepromAddress[1] = {0};
	float		eepromBuffer[1];
	float		*odometer		= eepromBuffer;*/
				
// FAULTS
	boolean faultFlags[32];
	boolean faultReset = 0;
	const uint8_t faultLevel[32] =		{
		0,	// 0
		1,	// 1
		1,	// 2
		1,	// 3
		1,	// 4
		1,	// 5
		3,	// 6
		1,	// 7
		2,	// 8
		2,	// 9
		2,	// 10
		2,	// 11
		0,	// 12
		1,	// 13
		1,	// 14
		3,	// 15
		2,	// 16
		1,	// 17
		2,	// 18
		1,	// 19
		1,	// 20
		0,	// 21
		0,	// 22
		1,	// 23
		1,	// 24
		2,	// 25
		2,	// 26
		0,	// 27
		0,	// 28
		0,	// 29
		0,	// 30
		0,	// 31
	};	// 1=critical	2=warning	3=status	0=unused
	const String faultMessages[32] =	{
		"",	// 0
		"Lost connection to an essential component!",	// 1
		"Processor cycle overtime!",	// 2
		"Opto board power loss detected",	// 3
		"Left handlebar lost connection to controller!",	// 4
		"Right handlebar lost connection to controller!",	// 5
		"Unrecognized serial msg received from HMI",	// 6
		"Throttle reading out of bounds detected!",	// 7
		"Cell 1 Low!",	// 8
		"Cell 2 Low!",	// 9
		"Cell 3 Low!",	// 10
		"Cell 4 Low!",	// 11
		"",	// 12
		"Problem detected with the motor contactor!",	// 13
		"Function tracking bits overloaded!",	// 14
		"Throttle must be at idle to enter gear",	// 15
		"High current detected at 12V shunt (>12A)",	// 16
		"Critical fault detected!",	// 17
		"EEPROM values unavailable; cannot record odometer",	// 18
		"Battery cell at critical level; please stop riding",	// 19		50 in length, for reference (faults cannot be longer than 50 in length
		"Throttle out feedback error",	// 20
		"",	// 21
		"",	// 22
		"Unexpected timerTracker value detected!!",	// 23
		"Unexpected ONSTracker value detected!!",	// 24
		"Aux power is off; IO will not work correctly. ",	// 25
		"HMI overtime, disabling Nextion",	// 26
		"",	// 27
		"",	// 28
		"",	// 29
		"",	// 30
		"", // 31
	};	
	int overTimeLength;	
	int expectedONS, expectedTimer;	
	int numberOfFaults;
	int numberScansPerSecond = 0;
	boolean anyFaultsDetected = 0;
	
	boolean *allConnectionsOK			= faultFlags + 1,
			*watchdogFlag				= faultFlags + 2,
			*optoPowerLost				= faultFlags + 3,
			*handlebarPowerLeftFault	= faultFlags + 4,
			*handlebarPowerRightFault	= faultFlags + 5,
			*nexReadUnrecognized		= faultFlags + 6,
			*throttleOutOfBounds		= faultFlags + 7,
			*cellOneLow					= faultFlags + 8,
			*cellTwoLow					= faultFlags + 9,
			*cellThreeLow				= faultFlags + 10,
			*cellFourLow				= faultFlags + 11,
			*motorContactorFault		= faultFlags + 13,
			*trackerBitsOverload		= faultFlags + 14,
			*thlNotIdleWhenReq			= faultFlags + 15,
			*controlOvercurrentFlag		= faultFlags + 16,
			*anyLevel1FaultDetected		= faultFlags + 17,
			*unableToLoadEEPROM			= faultFlags + 18,	
			*anyCellCritical			= faultFlags + 19,
			//*throttleFeedbackFault		= faultFlags + 20,
			*unexpectedTimer			= faultFlags + 23,
			*unexpectedONS				= faultFlags + 24,
			*auxPowerFault				= faultFlags + 25,
			*hmiOvertimeFault			= faultFlags + 26;


	// Nextion Variables for my functions
	int nextionPage = 0;		// current page of nextion to refresh; functionality being added later
	int activeFaultToDisplay = 0;
	int hmiElemToUpd = 0, thisSpot;
	boolean nextUpdate;
	String nexBufferMem[30];
	boolean nextionDelay = 0;
	uint8_t nexBytesRead[3];
			
	// DEBUG MEMORY ---------------------
	// settings that affect vehicle behavior
	boolean nextionEnabled				= 1;
	boolean contactorSafetiesActive		= 1;
	boolean faultsOnSerial				= 1;
	boolean debuggingActive				= 1;
	// settings that affect debug messages
	boolean debuggingHMIActive			= 0;
	boolean scanTimeDebugEnabled		= 0;
	boolean faultsDebuggingActive		= 0;
	boolean driveSystemDebuggingActive	= 0;
	boolean analogDebuggingActive		= 0;
	boolean speedDebugActive			= 0;
	boolean clockDebugActive			= 0;
	boolean motorControlDebugEnable		= 1;
	
	boolean firstScan					= 1;
	boolean auxPower;
	long debugTimer;
	int hmiOvertimeLength;


// MAIN PROGRAM
void setup()	
{
	zzpwmMotor.attach(zzpwmOutputPin);	// FOR PRESENTATION
	pinMode(9, OUTPUT);
	digitalWrite(9, LOW);	// 9 is the ground reference for the presentation motor
	
	int setupTime = millis();
	
	pinMode(auxPowerPin, INPUT);
	auxPower = digitalRead(auxPowerPin);
	
	// SETUP SERIAL FUNCTIONS
	if (debuggingActive) 
	{
		Serial.begin(115200);
		while (!TON(1, 50, 31) || !Serial) delay(10);	// wait 200ms for serial debugging to initialize
		//USBActive = Serial;
	}
	
	// setup Nextion
	if (nextionEnabled)
	{
		Serial.print("Initializing Nextion...");
		nexSerial.begin(115200);
		while (!TON(1, 50, 31) || !nexSerial) delay(10);
		if (nexSerial) Serial.println("Done.");
		else Serial.println("Failed.");
	}
	else Serial.println("Nextion is disabled in settings.");
	
	
	if (auxPower || clockDebugActive)
	{
		// setup RTC
		Serial.print("Wire.begin()...");
		Wire.begin();
		Serial.println("Done.");
		delay(10);
		Serial.print("initial read of DS3231...");
		now = RTC.now();				// update RTC value before starting Serial.println
		Serial.println("Done.");
		Serial.println("Current time is " + currentDate() + " " + currentTime());
		//logFileName = String(now.year()) + String(now.month()) + String(now.day()) + " " + String(now.hour()) + ":" + String(now.minute()) + ".txt";
		//if (SDActive) Serial.println("SDCard Serial.println now Active");
	}
	else Serial.println("No 5V aux power detected; skipping DS3231");
	
	
	// declare all pinModes
	
	if (debuggingActive) Serial.print("setting up opto inputs...");
	// opto inputs
	for (uint8_t i=0; i<sizeof(opto1pins); i++) pinMode(opto1pins[i], INPUT);
	for (uint8_t i=0; i<sizeof(opto2pins); i++) pinMode(opto2pins[i], INPUT);
	if (debuggingActive) Serial.println("Done.");
		
		
	if (debuggingActive) Serial.print("setting up relay outputs...");
	// relay outputs (R1 and R2):
	for (uint8_t i=0; i<sizeof(relayR1Pins); i++) pinMode(relayR1Pins[i], OUTPUT);
	for (uint8_t i=0; i<sizeof(relayF2Pins); i++) pinMode(relayF2Pins[i], OUTPUT);
	for (uint8_t i=0; i<sizeof(relayM3Pins); i++) pinMode(relayM3Pins[i], OUTPUT);
		
	// ensure the pulse to all relays so longer happens on startup
	for (uint8_t i=0; i<sizeof(relayR1); i++) digitalWrite(relayR1Pins[i], 1);
	for (uint8_t i=0; i<sizeof(relayF2); i++) digitalWrite(relayF2Pins[i], 1);
	for (uint8_t i=0; i<sizeof(relayM3); i++) digitalWrite(relayM3Pins[i], 1);
			
	// 5v/gnd constant pins for relay on-board optocouplers
	if (auxPower){
		// relay 1 power/gnd (front)
		pinMode(23, OUTPUT);
		digitalWrite(23, HIGH);
		pinMode(37, OUTPUT);
		digitalWrite(37, LOW);
	
		// relay 2 power/gnd (rear)
		pinMode(39, OUTPUT);
		digitalWrite(39, LOW);
		pinMode(53, OUTPUT);
		digitalWrite(53, HIGH);
	}
	
	// relay 3 is powered by jumpers on the arduino screw terminal board
	// 2 is GND, 7 is Vcc
	if (debuggingActive) Serial.println("Done.");
	
	// reset all oneshot/toggle/debounce bits before use
	if (debuggingActive) Serial.print("Resetting logic states...");
	for (uint8_t i=0; i<32; i++)
	{
		oneShotBits[i] = 0;				// general use oneshot bits
		oneShotBits[i+32] = 0;			// fault oneshot bits
		//debounceInSession[i] = 0;		// for speed, so we only update debounce timers when needed
		//debounceMemory[i] = 0;		// make function calls smaller by remembering previous output state
		faultFlags[i] = 0;				// fault bits
	}
	for(uint8_t i=0; i<sizeof(oneShotBits); i++) oneShotBits[i] = 0;
	if (debuggingActive) Serial.println("Done.");
	
	// reset all fault flags
	for(uint8_t i=0; i<sizeof(faultFlags); faultFlags[i++] = 0); 
	
	/*// load EEPROM values
	Serial.print("Loading initial eeprom values...");
	for (uint8_t i=0; i<sizeof(eepromAddress); i++) eepromBuffer[i] = eeprom_read_float(eepromAddress[i]);
	Serial.println("Done.");*/
	
	setupTime = millis() - setupTime;
	if (debuggingActive) Serial.println("\nSETUP COMPLETE in " + String(setupTime) + "ms\n");
	
	// update IO before starting main cycle
	MapInputs(1);
	MapOutputs(1);
	
	last_cycleStart = millis();
	last_cycleStartus = micros();
	if (debuggingActive) Serial.println("Beginning first scan");
}

void loop()		
{
	ONSTracker = 0;
	// debounceTracker = 0;
	timerTracker = 0;
	
	// IO updates at 50hz
	boolean updateIOPulse = oneShot(FlasherBit(50), ONSTracker++);
	MapInputs(updateIOPulse);				// routine io updates done when io pulse true
	MapOutputs(updateIOPulse);				// routine io updates done when io pulse true
	// both are PET, because the serial is updated on a NET of a 50Hz pulse, so IO and serial updates are staggered
	// speedometer PX needs to be updated FAST	
		
	LightRoutine();							// sets the status of lights on front and rear relay boards using pointers
	
	// only update clock if there is 5V aux available
	clockUpdate();
	
	DriveSystem();							// determines if it is safe to move the vehicle, and if so, generates a throttle value accordingly
	
	MonitorSystem();						// monitors analog inputs for voltage and current readings, and front wheel speed
	
	if (nextionEnabled) HMIControl();		// displays data collected in DriveSystem and MonitorSystem and converts it into strings and sends data to Nextion
	
	if (debuggingActive) debugRoutine();	// temporary fixes and misc message generation for debugging purposes
	
	Faults();								// sets fault flags based on system status
	
	if (debuggingActive && firstScan) Serial.println("First scan complete.");
	if (firstScan) firstScan = 0;
	
	delayMicroseconds(500);	// for stability
}

// SUBROUTINES
void MapInputs(boolean IOUpdate)	
{
	
	if (IOUpdate) 
	{
		for (uint8_t i=0; i<sizeof(opto1); i++)  opto1[i] = !digitalRead(opto1pins[i]);
		for (uint8_t i=0; i<sizeof(opto2); i++)  opto2[i] = !digitalRead(opto2pins[i]);
		*optoPower = !*optoPower;	// *optoPower is high when on, so need to re-invert input

		// MapInputs is read at 50Hz, so anything that is not throttleInput is done one at a time for speed
		if (analogInputTracker >= sizeof(analogInputs) / 2)
		{
			analogInputTracker = 1;
			last_analogCycleComplete = millis();
		}
		analogInputs[0] = analogRead(analogInputPins[0]);
		analogInputs[analogInputTracker] = analogRead(analogInputPins[analogInputTracker]);
		analogInputTracker++;
	}
	
	speedoPXInput = !digitalRead(speedoPXInputPin);	// this pin needs to update as fast as it can
	
}

void MapOutputs(boolean IOUpdate)	
{
	if (IOUpdate)
	{
		// outputs are active low
		if (auxPower || debuggingActive)
		{
			for (uint8_t i=0; i<sizeof(relayR1); i++) digitalWrite(relayR1Pins[i], !relayR1[i]);
			for (uint8_t i=0; i<sizeof(relayF2); i++) digitalWrite(relayF2Pins[i], !relayF2[i]);
			for (uint8_t i=0; i<sizeof(relayM3); i++) digitalWrite(relayM3Pins[i], !relayM3[i]);
		}
		else
		{
			for (uint8_t i=0; i<sizeof(relayR1); i++) digitalWrite(relayR1Pins[i], 1);
			for (uint8_t i=0; i<sizeof(relayF2); i++) digitalWrite(relayF2Pins[i], 1);
			for (uint8_t i=0; i<sizeof(relayM3); i++) digitalWrite(relayM3Pins[i], 1);
		}
		analogWrite(throttleOutputPin, throttleOutput);
		
		// repeat the throttleOutput DC as a PWM for demonstration purposes
		if (!inGear) pwmThrottleOutput = 1000;
		else pwmThrottleOutput = map(throttlePercent, 0, 100, 1100, 2000);
		zzpwmMotor.writeMicroseconds(pwmThrottleOutput);
	}
}

void LightRoutine()			
{
	if (!*lightsOn)
	{
		*FLTurn = *RLTurn = *FRTurn = *RRTurn = 0;
	}
	else if (*leftTurnInput && *rightTurnInput)
	{
		*FLTurn = *RLTurn = *FRTurn = *RRTurn = FlasherBit(1.5);
	}
	else if (*leftTurnInput)
	{
		*FLTurn = *RLTurn = FlasherBit(1.5);
		*FRTurn = 1;
		*RRTurn = 0;
	}
	else if (*rightTurnInput)
	{
		*FRTurn = *RRTurn = FlasherBit(1.5);
		*FLTurn = 1;
		*RLTurn = 0;
	}
	else // front turn signals are on at idle on motorcycles
	{
		*FLTurn = *FRTurn = 1;
		*RLTurn = *RRTurn = 0;
	}
	
	// headlight
	*lowBeamsOut = *lowBeamInput && *lightsOn;
	*highBeamsOut = *highBeamInput && *lightsOn;
	
	// brake light
	*brakeHighOutput = ((*rearBrake || *frontBrake) && *lightsOn);
	*brakeLowOutput =  *lightsOn;
	
	// fog lights
	*fogLightsOut = *fogLightsIn;
	
	// just repeat the horn out to the BOI
	*hornOutput = *hornInput;
}

void DriveSystem()			
{
	// fixes bug where exiting gear with the pb throws a fault
	if (inGear && *startPb) exitingGear = 1;
	if (exitingGear && !*startPb) exitingGear = 0;
	
	// in/out of gear logic
	if (!killswOffSinceBoot || !*killswitch) inGear = 0;
	else if (!inGear && throttlePercent > 2);	// do NOT go into gear unless throttle is idle
	else inGear = toggleState(*startPb, inGear, ONSTracker);
	ONSTracker++; // this must be incremented EVERY scan
	
	// check if killswitch has been off since boot
	if (!killswOffSinceBoot && !*killswitch) killswOffSinceBoot = 1;
	
	// control logic for the ESC solenoid
	if (!killswOffSinceBoot) *ESCSolenoid = 0;
	else if (contactorSafetiesActive && *anyLevel1FaultDetected) killswOffSinceBoot = 0;
	else *ESCSolenoid = *killswitch;
	
	throttlePercent = map(*throttleInput, voltsToAnalogIn(1), voltsToAnalogIn(4), 0, 100);		// calculate the pulse width in us for motor out
	
	if (inGear) throttleOutput = map(*throttleInput, voltsToAnalogIn(1), voltsToAnalogIn(4), voltsToAnalogOut(1), voltsToAnalogOut(4));
	else throttleOutput = voltsToAnalogOut(1);
	
	if (sportMode) 
	{
		*speedHighOut = 1; 
		*speedLowOut = 0;
	}
	else if (economyMode)
	{
		*speedHighOut = 0;
		*speedLowOut = 1;
	}
	else
	{
		*speedHighOut = 0;
		*speedLowOut = 0;
	}
	
	return;
}

void MonitorSystem()		
{
	
	// reset watchdog timer
	watchdog = millis() - last_cycleStart;
	last_cycleStart = millis();
	
	// check for fastest/longest cycle
	if (watchdog > longestCycle) longestCycle = watchdog;
	if (watchdog < fastestCycle) fastestCycle = watchdog;
	
	// value calculated based on what ina126p output should be
	currentDrawShuntmA = map(*currentDrawInput, 0, voltsToAnalogIn(4.425), 0, 15000); // calculates a precise float in ma
	currentDrawMotorAmps = map(*motorDrawInput, 0, voltsToAnalogIn(4.25), 0, 150); // calculates a precise float in amps
	for (uint8_t i=0; i < sizeof(cellVolts) / 4; i++) cellVolts[i] = (float)map(*cellVoltageRawIn[i], 0, 648, 0, 1512) / 100.0;	// calculates a precise cell level in volts
	vBat = (float)(map(*vBatInput, 0, 570, 0, 5992)) / 100.0;
	
	// draw in mAh to be added is draw * 0.2778 = (mAh in 0.5s at draw rate in mA)
	if (oneShot(FlasherBit(2), ONSTracker++)) totalDraw_mAh += (int)(currentDrawMotorAmps * 0.1389);		// calculation done in Amps
	/*
		10 A for an hour is 10000mAh
		for a second is 2.778mAh
		for half a second is 1.389mAh
		Therefore, for each amp drawn, 0.1389mAh is expended in 0.5s
	*/
	
	// keep track of how many seconds the system has been in gear
	if (oneShot(FlasherBit(1), ONSTracker++) && inGear) totalInGearSeconds++;
	
	// SPEED LOGIC
	if (oneShot(speedoPXInput, ONSTracker++)) speedCount++;
	if (oneShot(FlasherBit(2), ONSTracker++))
	{
		speed = map(speedCount, 0, 50, 0, 101);
							// 50 pulses in 0.5s is 101km/h
		if (speedDebugActive)
		{
			Serial.println("speed is " + String(speed) + "km/h");
			Serial.println("speedCount is " + String(speedCount));
		}						
		speedCount = 0;	
	}
	/*	speed is an integer representing speed in km/h
		front wheel has 8 spokes on the brake
		front wheel has a 70 cm OD
		c = pi * 70
		each PET = c/8 cm traveled
		I will use the time between PETs to calculate the speed
		At 100kph, the wheel rotates at 12.64rps
		That is 101 pulses per second, meaning we need at least 200-300 samples per second
		
		101 pulses per second at 100kph
		10ms between each PET
		Likewise, 25kph would be 40ms between each pulse
		We will use a map() function with these two points to calculate the speed, using the micros() function, for accuracy
	*/
		
}

void HMIControl()			
{
	long hmiStartTime = millis();
	/*
	if (oneShot(FlasherBit(4), ONSTracker++))
	{
		if (debuggingHMIActive) Serial.print("Starting listen for Nextion buttons...");
		nexLoop(nex_listen_list);
		if (debuggingHMIActive) Serial.println("Done.");
	}*/
	
	if (nexRead())
	{
		if (nexCheckButtonPress(nexBytesRead)) *nexReadUnrecognized = 1;
	}
	

	boolean level1FaultFlasher = FlasherBit(1);	// flash alarm window if critical faults exist
	
	thisSpot = 0;
	nextUpdate = oneShot(!FlasherBit(20), ONSTracker++);
	if (nextUpdate) hmiElemToUpd++;
	//if (debuggingHMIActive && nextUpdate) Serial.print("Beginning update of element " + String(hmiElemToUpd) + "...");
	
	// nextionDelay not fully implemented yet, will be used for updating speed PX more reliably in the future
	if (oneShot(nextionDelay, ONSTracker++)) Serial.println("Nextion overloaded, delaying serial.");	
	else if	(chkUpdElem(0)) nexTextFromString("alarmView0", cycleActiveFaults(), 60);	// 0
	else if (chkUpdElem(2)) nexTextFromString("alarmView2", cycleActiveFaults(), 60);	// 1
	else if (chkUpdElem(1)) nexTextFromString("alarm0", listAllFaults(0), 60);			// 2
	else if (chkUpdElem(1)) nexTextFromString("alarm1", listAllFaults(1), 60);			// 3
	else if (chkUpdElem(1)) nexTextFromString("alarm2", listAllFaults(2), 60);			// 4
	else if (chkUpdElem(1)) nexTextFromString("alarm3", listAllFaults(3), 60);			// 5
	else if (chkUpdElem(1)) nexTextFromString("alarm4", listAllFaults(4), 60);			// 6
	else if (chkUpdElem(1)) nexTextFromString("alarm5", listAllFaults(5), 60);			// 7
	else if (chkUpdElem(1)) nexTextFromString("alarm6", listAllFaults(6), 60);			// 8
	else if (chkUpdElem(1)) nexTextFromString("alarm7", listAllFaults(7), 60);			// 9
	else if (chkUpdElem(1)) nexTextFromString("alarm8", listAllFaults(8), 60);			// 10
	else if (chkUpdElem(1)) nexTextFromString("alarm9", listAllFaults(9), 60);			// 11
	
	else if (chkUpdElem(0)) nexTextFromString("vBat0", String(vBat) + " V", 10);			// 12
	else if (chkUpdElem(2)) nexTextFromString("vBat2", String(vBat) + " V", 10);			// 13
	else if (chkUpdElem(2)) nexTextFromString("cell1v", String(cellVolts[0]) + " V", 10);// 14
	else if (chkUpdElem(2)) nexTextFromString("cell2v", String(cellVolts[1]) + " V", 10);// 15
	else if (chkUpdElem(2)) nexTextFromString("cell3v", String(cellVolts[2]) + " V", 10);// 16
	else if (chkUpdElem(2)) nexTextFromString("cell4v", String(cellVolts[3]) + " V", 10);// 17
	else if (chkUpdElem(0)) nexBar("voltBar", (int)map(vBat, 0, 96, 0, 100));			// 18
	else if (chkUpdElem(0)) nexTextFromString("currentDis0", String(currentDrawMotorAmps) + " A", 10);	// 19
	else if (chkUpdElem(2)) nexTextFromString("currentDis2", String(currentDrawMotorAmps) + " A", 10);	// 20
	else if (chkUpdElem(0)) nexBar("currBar", (int)map(currentDrawMotorAmps, 0, 120, 0, 100));				// 21
	
	else if (chkUpdElem(0)) nexTextFromString("powerDis0", String(round(currentDrawMotorAmps * vBat)) + " W", 10); // 22
	else if (chkUpdElem(2)) nexTextFromString("powerDis2", String(round(currentDrawMotorAmps * vBat)) + " W", 10); // 23
	else if (chkUpdElem(0)) nexBar("powerBar", (int)map(currentDrawMotorAmps * vBat, 0, 9000, 0, 100));		// 24
	
	else if (chkUpdElem(0)) nexTextFromString("mAh0", String(totalDraw_mAh) + " mAh", 10);			// 25
	else if (chkUpdElem(2)) nexTextFromString("mAh2", String(totalDraw_mAh) + " mAh", 10);			// 26
	
	else if (chkUpdElem(0)) nexTextFromString("speedDis0", String(speed) + " km/h", 10);			// 27
	else if (chkUpdElem(2)) nexTextFromString("speedDis2", String(speed) + " km/h", 10);		// 28
	
	else if (chkUpdElem(2)) nexTextFromString("smshunt", String(currentDrawShuntmA) + " mA", 10);
	else if (chkUpdElem(2)) nexTextFromString("mtrcur", String(currentDrawMotorAmps) + " A", 10);
	else if (chkUpdElem(2)) nexBar("thlin", throttlePercent);
	else if (chkUpdElem(2)) nexBar("thlout", map(pwmThrottleOutput, 1000, 2000, 0, 100));
	
	// total in gear seconds indicator
	else if (chkUpdElem(2)) nexTextFromString("inGearTime", secondsToClock(totalInGearSeconds), 30);
	
	//Settings Page Updates
	else if (chkUpdElem(3)) nexCheckbox("sport", sportMode);
	else if (chkUpdElem(3)) nexCheckbox("standard", standardMode);
	else if (chkUpdElem(3)) nexCheckbox("economy", economyMode);
	
	// ADD CLOCK UPDATE LOGIC FROM 'HMI clock test' STANDARD!!!!!!
	else if (chkUpdElem(0)) nexTextFromString("clockDis", currentTime(), 14);
	
	else if (nextUpdate && (hmiElemToUpd == thisSpot++))											// reset element to update
	{
		//if (debuggingHMIActive) Serial.println("\nResetting HMI elements counter from " + String(hmiElemToUpd));
		hmiElemToUpd = -1;
	}
	
	else;	// END OF PERIODIC UPDATE TABLE
	// START OF FAST UPDATE TABLE
	int hmiElemToUpdTemp = hmiElemToUpd++;
	
	if (nextUpdate && nextionPage == 0)							// 29
	{
		if (inGear)
		{
			if (nexTextFromString("inGearInd0", "D", 2)) nexSetFontColor("inGearInd0", LBLUE);
		}
		else
		{
			if (nexTextFromString("inGearInd0", "N", 2)) nexSetFontColor("inGearInd0", GREEN);
		}
	}
	hmiElemToUpd++; // needed bc this is how elements are buffered
	
	if (nextUpdate && nextionPage == 2)							// 29
	{
		if (inGear)
		{
			if (nexTextFromString("inGearInd2", "D", 2)) nexSetFontColor("inGearInd2", LBLUE);
		}
		else
		{
			if (nexTextFromString("inGearInd2", "N", 2)) nexSetFontColor("inGearInd2", GREEN);
		}
	}
	hmiElemToUpd++; // needed bc this is how elements are buffered
		
	if (nextUpdate && nextionPage == 0)							// 31
	{
		if (anyFaultsDetected && !(*anyLevel1FaultDetected && level1FaultFlasher)) nexSetBackColor("alarmView0", ALRMRED);
		else nexSetBackColor("alarmView0", GREY);
	}
	hmiElemToUpd++; // needed bc this is how elements are buffered
	
	if (nextUpdate && nextionPage == 2)							// 31
	{
		if (anyFaultsDetected && !(*anyLevel1FaultDetected && level1FaultFlasher)) nexSetBackColor("alarmView2", ALRMRED);
		else nexSetBackColor("alarmView2", GREY);
	}
	
	// Brights/Fog lights
	if (nextUpdate && nextionPage == 0)
	{
		if (*highBeamsOut) nexSetPicture("brights", 2);
		else nexSetPicture("brights", 1);
	}
	hmiElemToUpd++; // needed bc this is how elements are buffered
	
	if (nextUpdate && nextionPage == 0)
	{
		if (*fogLightsOut) nexSetPicture("fog", 3);
		else nexSetPicture("fog", 1);
	}
	hmiElemToUpd++; // needed bc this is how elements are buffered
	
	hmiElemToUpd = hmiElemToUpdTemp;
	//if (debuggingHMIActive && nextUpdate && (hmiElemToUpd != -1)) Serial.println("Done.");

	hmiOvertimeLength = millis() - hmiStartTime;
}

void Faults()				
{
	
	// fault reset
	if (faultReset) 
	{
		for (uint8_t i=0; i<sizeof(faultFlags); faultFlags[i++] = 0);
		faultReset = 0;
		if (debuggingActive) Serial.println("Faults reset.");
	}
	
	
	// update active fault for cycleActiveFaults()
	if (oneShot(FlasherBit(0.2), ONSTracker++))
	{
		activeFaultToDisplay++;
		if (faultsDebuggingActive) Serial.println("active fault is now " + String(activeFaultToDisplay));
	}
	
	
	// COMMS errors
	//if(!*optoPower || !*handlebarPowerLeft || !*handlebarPowerRight) *allConnectionsOK = 1;		// active bit for comms lost
	if (!*optoPower) *optoPowerLost = 1;
	if (!*handlebarPowerLeft) *handlebarPowerLeftFault = 1;
	if (!*handlebarPowerRight) *handlebarPowerRightFault = 1;
	
	// overtime errors
	if (watchdog > 100)
	{
		*watchdogFlag = 1;
		overTimeLength = watchdog;
	}
	
	// control system errors
	if (!limit(voltsFromAnalogIn(*throttleInput), 0.5, 4.5)) *throttleOutOfBounds = 1;
	if (ONSTracker >= sizeof(oneShotBits) || timerTracker >= 32) *trackerBitsOverload = 1;
	if (!inGear && !exitingGear && throttlePercent > 10 && *startPb) *thlNotIdleWhenReq = 1;
	//if (currentDrawShuntmA > 12000) *controlOvercurrentFlag = 1; ZZZZZZZZZZZZZZZZZZZZZZZZZZZ
	
	// battery faults
	// 14.5 is the 'low' cell value
	float k = 14.5;
	*cellOneLow		= (cellVolts[0] < k);
	*cellTwoLow		= (cellVolts[0] < k);
	*cellThreeLow	= (cellVolts[0] < k);
	*cellFourLow	= (cellVolts[0] < k);
	for(uint8_t i=0; i<4; i++)	// check for critical cell voltages
	{	// only cover 2.5-3.3V; dont want to estop for a disconnected cell
		if (limit(cellVolts[i] / 4.0, 2.5, 3.3)) { *anyCellCritical = 1;	break; }	// each cell is 4S
	}
	
	// motor control safety faults
	//if (TON(*killswitch != *ESCSolenoidFeedback, 100, timerTracker++)) *motorContactorFault = 1;
	// check that ESC control voltage outputted is within +-0.25V of expected value
	/*if ( limit(	voltsToAnalogOut(voltsFromAnalogIn(*throttleFeedback)) + voltsToAnalogOut(0.7),	// analog feedback will be 0.7V lower than output because of the BJT used
	throttleOutput - voltsToAnalogOut(0.25),		// all comparisons done at the 0-255 analogOut level
	throttleOutput + voltsToAnalogOut(0.25) ) ) {
	*throttleFeedbackFault = 1; }*/
	
	// check for existing faults
	for (uint8_t i=0; i<sizeof(faultFlags); i++)
	{
		if (faultFlags[i])
		{
			anyFaultsDetected = 1;
			break;
		}
	}
	for (uint8_t i=0; i<sizeof(faultFlags); i++)
	{
		if (faultFlags[i] && faultLevel[i] == 1) 
		{
			*anyLevel1FaultDetected = 1;
			break;
		}
	}
	
	// count number of faults
	numberOfFaults = 0;
	if (TON(1, 250, timerTracker++)) // don't display faults for the first 1/4s of operation
	{
		for (uint8_t i=0; i<sizeof(faultFlags); i++)
		{
			if (faultFlags[i]) 
			{
				numberOfFaults++;
			}
		}
	}
	
	//AUX POWER FAULT
	*auxPowerFault = !auxPower;	
	
	//  HMI FAULTS
	if (hmiOvertimeLength > 150) // disable Nextion if overtime detected
	{
		*hmiOvertimeFault = 1;
		nextionEnabled = 0;
	}
	
	// TRACKER FAULTS MUST BE AT THE END OF EVERY CYCLE
	if (oneShot(timerTracker != expectedTimer, ONSTracker++) && !firstScan) {
		*unexpectedTimer = 1;
		Serial.println("timerTracker expected was " + String(expectedTimer) + ", finished cycle with " + String(timerTracker));
	}
	if (oneShot((ONSTracker + 1) != expectedONS, ONSTracker++) && !firstScan) {
		*unexpectedONS = 1;
		Serial.println("ONSTracker expected was " + String(expectedONS) + ", finished cycle with " + String(ONSTracker));
	}
	if (firstScan)
	{
		expectedONS = ONSTracker;
		expectedTimer = timerTracker;
		if (debuggingActive) Serial.println("ONS used on first scan was: " + String(ONSTracker));
	}
	
}

void debugRoutine()			
{
	boolean debugFlasherBit = oneShot(FlasherBit(1), ONSTracker++);
	// Scan time debug
	if (scanTimeDebugEnabled)
	{
		// monitor cycle time in microseconds
		watchdogus = micros() - last_cycleStartus;
		last_cycleStartus = micros();
		numberScansPerSecond++;
		if (debugFlasherBit)
		{
			Serial.println("last scan was " + String(watchdogus) + "us");
			Serial.println("longest scan in the last second was " + String(longestCycle) + "ms");
			Serial.println("number of scans last second was " + String(numberScansPerSecond));
			longestCycle = 0;
			numberScansPerSecond = 0;
		}
	}
	
	// Time debug
	if (clockDebugActive && (clockDebugActive || auxPower) && oneShot(FlasherBit(1), ONSTracker++))
	{
		Serial.println("Current time is " + currentDate() + " " + currentTime());
	}
	
	if (motorControlDebugEnable && debugFlasherBit)
	{
		
	}
	
	// analog debug
	if (analogDebuggingActive && debugFlasherBit)
	{
		Serial.println("--------- ANALOG DEBUG START -----------");
		for (uint8_t i=0; i < sizeof(cellVolts) / 4; i++)
		{
			// print  raw cell volts
			Serial.println("*cellVoltageRawIn[" + String(i) + "] = " + String(*cellVoltageRawIn[i]));
			// print cellVolts
			Serial.println("cellVolts[" + String(i) + "] = " + String(cellVolts[i]));
		}
		Serial.println("*vBatInput is " + String(*vBatInput));
		Serial.println("vBat is " + String(vBat) + " V");
		Serial.println("*throttleInput is " + String(*throttleInput));
		Serial.println("throttle in voltage is " + String(voltsFromAnalogIn(*throttleInput)) + " V");
		Serial.println("throttle out is " + String(throttleOutput));
		Serial.println("mtr shunt is " + String(*motorDrawInput));
		Serial.println("12v shunt is " + String(*currentDrawInput));
		Serial.println("---------- ANALOG DEBUG END -----------");
	}
	
	// fault debugging
	if (faultsDebuggingActive && debugFlasherBit)
	{
		Serial.println("activeFaultToDisplay is " + String(activeFaultToDisplay) + ": " + cycleActiveFaults());
		Serial.println(String(numberOfFaults) + " faults active.");
		for (int i=0; i<numberOfFaults && i<10; i++) Serial.println("fault " + String(i) + " is: " + listAllFaults(i));
	}
	
	if (debugFlasherBit && *watchdogFlag) Serial.println("Watchdog was " + String(overTimeLength) + "ms");
	
	// DISPLAY FAULTS ON SERIAL
	if (faultsOnSerial && TON(1, 200, timerTracker++))
	{
		for (int i=0; i<32; i++)
		{
			if (oneShot(faultFlags[i], i+32)) Serial.println(faultMessages[i]);
		}
	}
	//if (oneShot(FlasherBit(4), ONSTracker++)) Serial.println("px in " + String(*speedoPXInput));
	
	if (driveSystemDebuggingActive && debugFlasherBit) Serial.println("*killswitch is " + String(*killswitch));
	if (driveSystemDebuggingActive && debugFlasherBit) Serial.println("*escsolenoid is " + String(*ESCSolenoid));
	
	//if (oneShot(FlasherBit(1), ONSTracker++)) Serial.println("shunt is " + String(currentDrawShuntmA) + " mA");
	//if(oneShot(FlasherBit(2), ONSTracker++)) Serial.println("left turn out is " + String(*FLTurn));
	
	// RESET WATCHDOG FAULT AT BEGINNING OF SCAN
	if (oneShot(TON(1, 20, timerTracker++), ONSTracker++)) faultReset = 1;
}


// Button press function calls from Nextion

void alarmView0Callback()	
{
	nextionPage = 1;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void alarmView2Callback()	
{
	nextionPage = 1;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void STAT0Callback()		
{
	nextionPage = 2;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void STAT1Callback()		
{
	nextionPage = 2;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void STAT3Callback()		
{
	nextionPage = 2;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void MAIN1Callback()		
{
	nextionPage = 0;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void MAIN2Callback()		
{
	nextionPage = 0;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void MAIN3Callback()		
{
	nextionPage = 0;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void FLTRST0Callback()		
{
	faultReset  = 1;
	if (debuggingActive) Serial.println("Fault reset issued");
}
void FLTRST1Callback()		
{
	faultReset  = 1;
	if (debuggingActive) Serial.println("Fault reset issued");
}
void FLTRST2Callback()		
{
	faultReset  = 1;
	if (debuggingActive) Serial.println("Fault reset issued");
}
void settings0Callback()	
{
	nextionPage = 3;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void settings2Callback()	
{
	nextionPage = 3;
	clearnexBuffer();
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void sportCallback()		
{
	sportMode = 1;
	standardMode = 0;
	economyMode = 0;
	clearnexBuffer();
}
void standardCallback()		
{
	sportMode = 0;
	standardMode = 1;
	economyMode = 0;
	clearnexBuffer();
}
void economyCallback()		
{
	sportMode = 0;
	standardMode = 0;
	economyMode = 1;
	clearnexBuffer();
}


// FUNCTION CALLS
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

boolean toggleState(boolean precond, boolean toggled, uint8_t OSR)
{
	if (oneShot(precond, OSR)) toggled = !toggled;
	return toggled;
}

/*
void logging(String input)
{
	if (!SDActive && !USBActive) return;	// if nothing is there to log to, dont bother trying to log
	Serial.println(input);					// print message to serial
	if (SDActive)							// print message to sdcard with timestamp
	{
		String timestmp = "(" + String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second()) + ") ";
		myFile = SD.open(logFileName, FILE_WRITE);
		
		if (myFile) {	// if the file opened okay, write to it
			myFile.println(timestmp + input);
			// close the file:
			myFile.close();
		}
		else {
			// if the file didn't open, print an error:
			Serial.println("logging error");
		}
	}
	// SAMPLE CODE FOR SDCARD
	/*
	// open the file. note that only one file can be open at a time,
	// so you have to close this one before opening another.
	myFile = SD.open("test.txt", FILE_WRITE);

	// if the file opened okay, write to it:
	if (myFile) {
		Serial.print("Writing to test.txt...");
		myFile.println("testing 1, 2, 3.");
		// close the file:
		myFile.close();
		Serial.println("done.");
	}
	else {
		// if the file didn't open, print an error:
		Serial.println("error opening test.txt");
	}

	// re-open the file for reading:
	myFile = SD.open("test.txt");
	if (myFile) {
		Serial.println("test.txt:");

		// read from the file until there's nothing else in it:
		while (myFile.available()) {
			Serial.write(myFile.read());
		}
		// close the file:
		myFile.close();
	}
	else {
		// if the file didn't open, print an error:
		Serial.println("error opening test.txt");
	}
	
}*/

float voltsFromAnalogIn (int input)
{
	float output = ((float)input * 5.0) / 1024.0;
	return output;
}

int voltsToAnalogIn (float input)
{
	int output = (int)((input * 1024.0) / 5.0);
	return output;
}

int voltsToAnalogOut(float input)
{
	int output = (input * 255.0 / 5.0);
	return output;
}

boolean TON(boolean input, int preset, int timerNumber)
{
	if (input && !timerInSession[timerNumber]) timerTimers[timerNumber] = millis();
	else if (input && timerMemory[timerNumber]) return 1;
	else if (input && millis() - timerTimers[timerNumber] >= preset) 
	{
		timerMemory[timerNumber] = 1;
		return 1;
	}
	else;
	timerMemory[timerNumber] = 0;
	timerInSession[timerNumber] = input;
	return 0;
}

boolean limit(float input, float lower, float upper) 
{
	if (input < lower) return 0;
	else if (input > upper) return 0;
	else return 1;
}

String listAllFaults(int spot)
{
	/* //I don't think I need this part anymore
	int totalFaults = 0;
	for (int i=0; i<sizeof(faultFlags); i++) if (faultFlags[i]) totalFaults++;
	*/  
	// list all faults in order of importance
	int j = 0;
	String buffer[10] = {"-","-","-","-","-","-","-","-","-","-"};
	for (int i=0; i<sizeof(faultFlags) && j<10; i++) { 
		if (faultFlags[i] && faultLevel[i] == 1) {
			buffer[j] = "1: " + faultMessages[i];
			j++;
		}
	}
	for (int i=0; i<sizeof(faultFlags) && j<10; i++) { 
		if (faultFlags[i] && faultLevel[i] == 2) {
			buffer[j] = "2: " + faultMessages[i];
			j++;
		}
	}
	for (int i=0; i<sizeof(faultFlags) && j<10; i++) { 
		if (faultFlags[i] && faultLevel[i] == 3) {
			buffer[j] = "3: " + faultMessages[i];
			j++;
		}
	}
	return buffer[spot];
}

String cycleActiveFaults()
{
	if (numberOfFaults == 0) return " ";
	if (activeFaultToDisplay >= numberOfFaults) activeFaultToDisplay = 0;
	return listAllFaults(activeFaultToDisplay);
	
	/*	ARCHIVE BEFORE RUNNING OFF OF LIST FAULTS
		if (numberOfFaults == 0) return " ";
		if (oneShot(FlasherBit(0.2), ONSTracker++))
		{
			activeFaultToDisplay++;
		}
		if (activeFaultToDisplay >= numberOfFaults) activeFaultToDisplay = 0;
		int j = 0;
		for (int i=0; i<32; i++)
		{
			if (faultFlags[i])
			{
				if (j++ == activeFaultToDisplay) return faultMessages[i];
			}
		}
		return "error";
		*/
}

boolean nexTextFromString(String objName, String input, int len)	// set the text displayed in a text box
{
	// function only sets the element text if a change is detected, 
	// and returns 'true' if a change is detected	
	//if (sizeof(input) > len) input = "K";
	if (nexBuffer(input))
	{
		if (debuggingHMIActive) Serial.println("buffer update on nextion element " + objName);
		String cmd;
		cmd += objName;
		cmd += ".txt=\"";
		cmd += input;
		cmd += "\"";
		nexSerial.print(cmd);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		return 1;
	}
	else return 0;
}

boolean nexBar(String objName, int val)		// set nextion bar value; ranges 0-100
{
	if (nexBuffer(String(val)))
	{
		if (debuggingHMIActive) Serial.println("buffer update on nextion element " + objName);
		String cmd;
		cmd += objName;
		cmd += ".val=";
		cmd += String(val);
		nexSerial.print(cmd);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		return 1;
	}
	return 0;
}

boolean nexCheckbox(String objName, boolean val)	// set nextion checkbox value; used for settings page5
{
	if (nexBuffer(String(val)))
	{
		if (debuggingHMIActive) Serial.println("buffer update on nextion element " + objName);
		String cmd;
		cmd += objName;
		cmd += ".val=";
		cmd += String(val);
		nexSerial.print(cmd);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		return 1;
	}
	return 0;
}

boolean nexSetFontColor(String objName, uint32_t number)	// set text box font color; used for gear indicator
{
	char buf[10] = {0};
	String cmd;
	
	utoa(number, buf, 10);
	cmd += objName;
	cmd += ".pco=";
	cmd += buf;
	nexSerial.print(cmd);
	nexSerial.write(0xFF);
	nexSerial.write(0xFF);
	nexSerial.write(0xFF);
	
	cmd = "";
	cmd += "ref ";
	cmd += objName;
	nexSerial.print(cmd);
	nexSerial.write(0xFF);
	nexSerial.write(0xFF);
	nexSerial.write(0xFF);
	
	return 1;
}

boolean nexSetBackColor(String objName, uint32_t number)	// set the background color on a text element; used for alarm window 'flash'
{
	if (nexBuffer(String(number)))
	{
		if (debuggingHMIActive) Serial.println("buffer update on Nextion element " + objName);
		
		char buf[10] = {0};
		String cmd;
		
		utoa(number, buf, 10);
		cmd += objName;
		cmd += ".bco=";
		cmd += buf;
		nexSerial.print(cmd);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		
		cmd = "";
		cmd += "ref ";
		cmd += objName;
		nexSerial.print(cmd);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		
		return 1;
	}
	return 0;
}

boolean nexSetPicture(String objName, int val)	// use Nextion serial protocol to set serial image
{
	if (nexBuffer(String(val)))
	{
		if (debuggingHMIActive) Serial.println("buffer update on Nextion element " + objName);
		String cmd;
		cmd += objName;
		cmd += ".pic=";
		cmd += String(val);
		nexSerial.print(cmd);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		nexSerial.write(0xFF);
		return 1;
	}
	return 0;
}

boolean nexCheckButtonPress(uint8_t recv[3])
{
	// check if message received is for any buttons, and return 1 if a valid button press is detected
	if (recv[0] == 0x00 && recv[1] == 0x04 && recv[2] == 0x01)
	{
		STAT0Callback();
		return 1;
	}
	if (recv[0] == 0x00 && recv[1] == 0x11 && recv[2] == 0x01)
	{
		alarmView0Callback();
		return 1;
	}
	if (recv[0] == 0x00 && recv[1] == 0x02 && recv[2] == 0x01)
	{
		FLTRST0Callback();
		return 1;
	}
	if (recv[0] == 0x00 && recv[1] == 0x12 && recv[2] == 0x01)
	{
		settings0Callback();
		return 1;
	}
	if (recv[0] == 0x01 && recv[1] == 0x04 && recv[2] == 0x01)
	{
		FLTRST1Callback();
		return 1;
	}
	if (recv[0] == 0x01 && recv[1] == 0x03 && recv[2] == 0x01)
	{
		STAT1Callback();
		return 1;
	}
	if (recv[0] == 0x01 && recv[1] == 0x02 && recv[2] == 0x01)
	{
		MAIN1Callback();
		return 1;
	}
	if (recv[0] == 0x02 && recv[1] == 0x18 && recv[2] == 0x01)
	{
		alarmView2Callback();
		return 1;
	}
	if (recv[0] == 0x02 && recv[1] == 0x02 && recv[2] == 0x01)
	{
		FLTRST2Callback();
		return 1;
	}
	if (recv[0] == 0x02 && recv[1] == 0x01 && recv[2] == 0x01)
	{
		MAIN2Callback();
		return 1;
	}
	if (recv[0] == 0x02 && recv[1] == 0x19 && recv[2] == 0x01)
	{
		settings2Callback();
		return 1;
	}
	if (recv[0] == 0x03 && recv[1] == 0x06 && recv[2] == 0x01)
	{
		sportCallback();
		return 1;
	}
	if (recv[0] == 0x03 && recv[1] == 0x07 && recv[2] == 0x01)
	{
		standardCallback();
		return 1;
	}
	if (recv[0] == 0x03 && recv[1] == 0x08 && recv[2] == 0x01)
	{
		economyCallback();
		return 1;
	}
	if (recv[0] == 0x03 && recv[1] == 0x09 && recv[2] == 0x01)
	{
		STAT3Callback();
		return 1;
	}
	if (recv[0] == 0x03 && recv[1] == 0x01 && recv[2] == 0x01)
	{
		MAIN3Callback();
		return 1;
	}
	
	return 0;
}

boolean nexRead()	// interpret incoming data from Nextion and store in a uint8_t array
{
	uint8_t __buffer[10];
	
	uint16_t i;
	uint8_t c;
	
	while (nexSerial.available() > 0)
	{
		delay(1);
		c = nexSerial.read();
		
		if (NEX_RET_EVENT_TOUCH_HEAD == c)
		{
			if (nexSerial.available() >= 6)
			{
				__buffer[0] = c;
				for (i = 1; i < 7; i++)
				{
					__buffer[i] = nexSerial.read();
				}
				__buffer[i] = 0x00;
				
				if (0xFF == __buffer[4] && 0xFF == __buffer[5] && 0xFF == __buffer[6])
				{
					nexBytesRead[0] = __buffer[1];
					nexBytesRead[1] = __buffer[2];
					nexBytesRead[2] = __buffer[3];
					return 1;
				}
				
			}
		}
	}
	return 0;
}

boolean nexBuffer(String input)
{
	if (input != nexBufferMem[hmiElemToUpd])
	{
		nexBufferMem[hmiElemToUpd] = input;
		return 1;
	}
	return 0;
}

void clearnexBuffer()
{
	for (int i=0; i<30; i++)
	{
		nexBufferMem[i] = "";
	}
}

String currentTime()
{
	if (!(auxPower || clockDebugActive)) return "--:--:--";
	char clockBuffer[14];
	sprintf(clockBuffer,"%02u:%02u:%02u",now.hour(),now.minute(),now.second());
	return String(clockBuffer);
}

String currentDate()
{	
	if (!(auxPower || clockDebugActive)) return "----/--/--";
	char clockBuffer[14];
	sprintf(clockBuffer,"%04u/%02u/%02u",now.year(),now.month(),now.day());
	return String(clockBuffer);
}

void clockUpdate()
{
	if ((auxPower || clockDebugActive) && oneShot(FlasherBit(2), ONSTracker++)) now = RTC.now();
}

boolean chkUpdElem(int page)
{
	if (nextionPage == page && nextUpdate && hmiElemToUpd == thisSpot++) return 1;
	else return 0;
}

String secondsToClock(int input)
{
	int seconds, minutes, hours;
	
	seconds = input % 60;
	minutes = (input / 60) % 60;
	hours = input / 3600;
	
	char outputBuffer[14];
	sprintf(outputBuffer,"%02u:%02u:%02u",hours,minutes,seconds);
	
	return String(outputBuffer);
}