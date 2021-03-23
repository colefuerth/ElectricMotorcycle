/*
 * currentTesting.ino
 *
 * Created: 1/29/2020 11:20:47 AM
 * Author: colef
 
  - lightTesting will check all 12v outputs tied to relays and check for a difference in current between idle and not idle
  - has been expanded to demonstrate optimized input conditioning
 
 */ 

// GLOBAL MEMORY

// input preconditioning memory
boolean oneShotBits[32];		// oneshot bits available for use
boolean debounceInSession[32];	// for speed, only update debounce timers when needed
int debounceTimers[32];			// debounce timers available for function call

// declare pin values and reset flags:
uint8_t	relayPins[4]	=	{2,3,4,5};
boolean relayOutput[4]	=	{0,0,0,0};
boolean relayFault[4]	=	{0,0,0,0};
boolean anyFaultsDetected = 0;
uint8_t currentDrawPin = A0;
boolean upButton, dnButton, inGear;
uint8_t upButtonPin = 22, dnButtonPin = 23, inGearPin = 26;
int currentDraw;


// MAIN PROGRAM
void setup()
{
	delay(100); // give the system time to start
	// checkLights();
	
	for (int i=0; i<32; i++) 
	{
		oneShotBits[i] = 0;			// reset all oneshot bits
		debounceInSession[i] = 0;	// reset all debounce bits
	}
	
	pinMode(upButtonPin, INPUT_PULLUP);
	pinMode(dnButtonPin, INPUT_PULLUP);
	for (int i=0; i<sizeof(relayPins); i++) pinMode(relayPins[i], OUTPUT);
	
	
}

void lcdSetup()
{
	Serial.begin(9600);
	Serial.println("serial channel opened");
	mylcd.Init_LCD();
	Serial.println(mylcd.Read_ID(), HEX);
	mylcd.Fill_Screen(BLACK);
	return;
}

void loop()
{
	// checkAnyFaults();
	MapInputs();
	zzCurrentCalibrate();
}


// SUBROUTINES
void MapInputs()
{
	uint8_t inputNumber = 0;	// there can be up to 32 conditioned inputs, identified by inputNumber
	upButton	=	oneShot(upButtonPin, inputNumber++);			// digitalRead and debounce conditioning is built-in to oneShot and toggleState calls
	dnButton	=	oneShot(dnButtonPin, inputNumber++);
	inGear		=	toggleState(inGear, inGearPin, inputNumber++);
	currentDraw = shuntReadmA(currentDrawPin);
}

void checkLights()
{
	
	for (int i=0; i<sizeof(relayPins); i++)
	{
		digitalWrite(relayPins[i], 1);
	}
	
	for(int i=0; i<sizeof(relayPins); i++)
	{
		// have a delay before each analog read to let the current settle
		delay(10);
		// shuntReadmA is a function that reads the analog voltage and returns the calculated current draw in mA
		int idleCurrent = shuntReadmA(currentDrawPin);
		digitalWrite(relayPins[i], 0);
		delay(10);
		int loadCurrent = shuntReadmA(currentDrawPin);
		// might add individual relay expected values later; for now 200mA seems good enough
		if (abs(loadCurrent - idleCurrent) <= 200) relayFault[1] = 1;
		digitalWrite(relayPins[i], 1);
	}
	
}

void checkAnyFaults()
{
	anyFaultsDetected = 0;
	for (int i=0; i<sizeof(relayFault); i++)
	{
		if (relayFault[i] == 1) anyFaultsDetected = 1;
	}
	return;
}

void zzCurrentCalibrate()
{
	
}

// FUNCTION CALLS
int shuntReadmA(uint8_t pin)
{
	int currentIn = analogRead(pin);
	int currentOut = map(currentIn, 0, 1023, 0, 15000);	// 5v reading is 15A, with the opamp circuit im using
	return currentOut;
}

boolean oneShot(uint8_t pin, uint8_t OSR)
{
	// debounce input first 
	boolean precond = debounce(precond, pin, OSR);
	
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

boolean toggleState(boolean toggled, uint8_t pin, uint8_t OSR)
{
	// debounce input first
	boolean precond = debounce(precond, pin, OSR);
	
	if (precond == 1 && oneShotBits[OSR] == 0)
	{
		oneShotBits[OSR] = 1;
		return !toggled;
	}
	else if (precond == 0 && oneShotBits[OSR] == 1)
	{
		oneShotBits[OSR] = 0;
		return toggled;
	}
	else return toggled;
}

boolean debounce(boolean debouncedBit, uint8_t pin, uint8_t timerNumber)
{
	
	boolean rawInput = digitalRead(pin);
	
	if (debouncedBit == rawInput) 
	{
		return debouncedBit;		
	}
	else if (debouncedBit != rawInput && debounceInSession[timerNumber] == 0)
	{
		debounceInSession[timerNumber] = 1;
		debounceTimers[timerNumber] = millis();
		return debouncedBit;
	}
	else if (debouncedBit != rawInput && millis() - debounceTimers[timerNumber] >= 20)
	{
		debounceInSession[timerNumber] = 0;
		return rawInput;
	}
	else
	{
		return debouncedBit;
	}
}