/*
    Name:       BENCH_TESTING.ino
    Created:	2019-10-26 9:30:28 PM
    Author:     DESKTOP-LLH4OMG\colef
*/

/* notes 20191028
	- relays r1-r6 changed to store raw outputs in relay[6] instead
	- finished revision 1
*/

#include <Wire.h>		// SCL SDA library for clock
#include <DS3231.h>		// DS3231 RTC library

#include <Servo.h>	//servo library for use with ESC


// MapInputs Variables
boolean t1, t2, t3, t4;
boolean button1;
int potentiometer;
// debounce variables:
boolean leftTurnInput, t1mem;
int t1Debounce;

boolean rightTurnInput, t2mem;
int t2Debounce;

boolean hazardInput, t3mem;
int t3Debounce;

boolean highBeamsInput, t4mem;
int t4Debounce;		

boolean inGearButton, button1mem;
int button1Debounce;


// clockRoutine Variables
RTClib RTC;
int lastClockTime;
int nowYear, nowMonth, nowDay, nowHour, nowMinute, nowSecond;


// Modes Variables
boolean leftTurn;						// momentary state
boolean rightTurn;
boolean hazardMode, hazardModeONS;		// toggle-able state
boolean highBeamsMode, highBeamsONS;
boolean inGear, inGearONS;


// LightRoutine Variables
boolean FLTurn, FRTurn, RLTurn, RRTurn, lowBeams, highBeams; 


// DriveSystem Variables
int throttleOutput, throttleInput; 
Servo ESC; 


// MonitorSystem Variables
int throttlePercent;


// MapOutputs Variables
boolean relay[6];
boolean led[8];
boolean inGearInd;
int motor;
	

void setup()
{
	
	// setup debug serial lanes
	Serial.begin(115200);
	
	// setup SDA wire for clock
	Wire.begin();
	
	ESC.attach(8);
	// ESC.attach(8, 1000, 2000);	// (pin, min pulse width, max pulse width in microseconds)
	
	// setup pin modes
	pinMode(10, INPUT);		// t1
	pinMode(11, INPUT);		// t2
	pinMode(12, INPUT);		// t3
	pinMode(13, INPUT);		// t4
	
	pinMode(21, INPUT_PULLUP);	// button1
	
	pinMode(17, OUTPUT);	// lamp 1
	
	pinMode(0, OUTPUT);		// led1
	pinMode(1, OUTPUT);		// led2
	pinMode(2, OUTPUT);		// led3
	pinMode(3, OUTPUT);		// led4
	pinMode(4, OUTPUT);		// led5
	pinMode(5, OUTPUT);		// led6
	pinMode(6, OUTPUT);		// led7
	pinMode(7, OUTPUT);		// led8
	
	pinMode(22, OUTPUT);	// relay 1
	pinMode(23, OUTPUT);	// relay 2
	pinMode(24, OUTPUT);	// relay 3
	pinMode(25, OUTPUT);	// relay 4
	pinMode(26, OUTPUT);	// relay 5
	pinMode(27, OUTPUT);	// relay 6
	
	int currentTime = millis();		// any routine referenceing millis() should grab it at the beginning of the routine
	
	// set debounce logic states
	button1Debounce = currentTime;
	button1mem = 0;
	t1Debounce = currentTime;
	t1mem = 0;
	t2Debounce = currentTime;
	t2mem = 0;
	t3Debounce = currentTime;
	t3mem = 0;
	t4Debounce = currentTime;
	t4mem = 0;
	
	// set debounced inputs
	inGearButton = 0;
	leftTurnInput = 0;
	rightTurnInput = 0;
	hazardInput = 0;
	highBeamsInput = 0;
	
	//set toggled inputs
	hazardModeONS = 0;
	hazardMode = 0;
	inGearONS = 0;
	inGear = 0;
	highBeamsMode = 0;
	highBeamsONS = 0;
	
	// setup clockRoutine() variables
	lastClockTime = millis();
	
}


void loop()
{
	MapInputs();
	ClockRoutine();
	Modes();
	LightRoutine();
	DriveSystem();
	MonitorSystem();
	MapOutputs();
}


void MapInputs()
{
	int currentTime = millis();		// any routine referenceing millis() should grab it at the beginning of the routine
	
	// input from touch buttons
	t1 = digitalRead(10);
	t2 = digitalRead(11);
	t3 = digitalRead(12);
	t4 = digitalRead(13);
	// condition touch buttons
	
	// input from button1
	button1 = !digitalRead(21);	// pullup, so inverting input
	
	// input from pot
	potentiometer = analogRead(A0);
	
	// debounce button1 and output debounced bit to inGearButton
	if (button1 != button1mem){
		button1mem = button1;
		button1Debounce = millis();
	}
	if (millis() - button1Debounce > 20) inGearButton = button1;
	
	// debounce t1 and output debounced bit to hazardInput
	if (t1 != t1mem){
		t1mem = t1;
		t1Debounce = currentTime;
	}
	if (currentTime - t1Debounce > 20) leftTurnInput = t1;
	
	// debounce t2 and output debounced bit to hazardInput
	if (t2 != t2mem){
		t2mem = t2;
		t2Debounce = currentTime;
	}
	if (currentTime - t2Debounce > 20) rightTurnInput = t2;
	
	// debounce t3 and output debounced bit to hazardInput
	if (t3 != t3mem){
		t3mem = t3;
		t3Debounce = currentTime;
	}
	if (currentTime - t3Debounce > 20) hazardInput = t3;
	
	// debounce t4 and output debounced bit to hazardInput
	if (t4 != t4mem){
		t4mem = t4;
		t4Debounce = currentTime;
	}
	if (currentTime - t4Debounce > 20) hazardInput = t4;
	
	
	
	return;
}


void ClockRoutine(){
	
	long currentTime = millis();
	
	if (currentTime - lastClockTime >= 200){		// refresh date and time every second
		
		lastClockTime = currentTime;
		DateTime now = RTC.now();
		
		nowYear = now.year();
		nowMonth = now.month();
		nowDay = now.day();
		nowHour = now.hour();
		nowMinute = now.minute();
		nowSecond = now.second();
		
	}
	
}
	
	
void Modes()
{
	/* Notes on Modes():
		- inGear logic complete
		- hazard latch complete
		- basically just relay the input bits for buttons and make sure modes do not overlap
	*/
	
	// toggle logic for in/out of gear
	if (inGearButton == 1 && inGearONS == 0){
		inGear = !inGear;
		inGearONS = 1;
	}
	else if (inGearButton == 0 && inGearONS == 1) inGearONS = 0;
	else;
	
	// toggle logic for in/out of hazard state
	if (hazardInput == 1 && hazardModeONS == 0){
		hazardMode = !hazardMode;
		hazardModeONS = 1;
	}
	else if (hazardInput == 0 && hazardModeONS == 1) hazardModeONS = 0;
	else;
	
	// toggle logic for in/out of high beams state
	if (highBeamsInput == 1 && highBeamsONS == 0){
		highBeamsMode = !highBeamsMode;
		highBeamsONS = 1;
	}
	else if (highBeamsInput == 0 && highBeamsONS == 1) highBeamsONS = 0;
	else;
	
	leftTurn = leftTurnInput;
	rightTurn = rightTurnInput;
	
	return;
}	


void LightRoutine()
{
	/*
		- blinking logic complete
	*/
	int currentTime = millis();		// any routine referenceing millis() should grab it at the beginning of the routine
	
	if (leftTurn == 1 && hazardMode == 0) {			// Right Turn Lights Blink Logic
		if (currentTime % 1000 < 500){
			FLTurn = 1;
			RLTurn = 1;
		}
		else {
			FLTurn = 0;
			RLTurn = 0;
		}
	}
	
	if (rightTurn == 1 && hazardMode == 0) {		// Left Turn Lights Blink Logic
		if (currentTime % 1000 < 500){
			FRTurn = 1;
			RLTurn = 1;
		}
		else {
			FRTurn = 0;
			RRTurn = 0;
		}
	}
	
	if (hazardMode == 1){							// Hazard Lights Blink Logic
		if (currentTime % 1000 < 500){
			FLTurn = 1;
			FRTurn = 1;
			RLTurn = 1;
			RRTurn = 1;
		} 
		else {
			FLTurn = 0;
			FRTurn = 0;
			RLTurn = 0;
			RRTurn = 0;
		}
	}
	
	highBeams = highBeamsMode;						// High Beams
	
	return;
}


void DriveSystem()
{
	/*
		- take the ingear mode and input from the potentiometer and convert it into a 1000-2000 pwm output
		- add a curve to throttleInput and output to throttleOutput
	*/
	throttleInput = map(potentiometer, 0, 1023, 1000, 2000);			// calculate the pulse width in us for motor out
	
	if (inGear == 1) throttleOutput = throttleInput;				//zzz change to a curve later
	else throttleOutput = 1000;
	
	throttlePercent = map(throttleInput, 1000, 2000, 0, 254);		// calculate the throttle value for display to the LEDs
	
	return;
}


void MonitorSystem()
{
	/*
		- take the inGear status and relay it to inGearInd
		- indicate the throttle output on the LED's 0-254
		- convert throttle percent 0-254 to the led[] array		
	*/
	
	inGearInd = inGear;			// relay in gear status to lamp ind
		
	for (int i=0; i<8; i++){	// relay the input read from 0-5V as 0-254 on LED array
		if (throttlePercent % 2 == 1) led[7-i] = 1;		// output LSB to current LED; leds display MSB to LSB, hence the [7-i]
		else led[i] = 0;
		throttlePercent = throttlePercent / 2;			// BSR
	}
	
	return;
}


void MapOutputs()
{
	/*
		- just output indicators and relays to the proper pin
		- also condition NC and NO relays
		- still need to add motor PWM on pin 8
	*/
	
	// conditioning for relay outputs
	relay[0] = RLTurn;
	relay[1] = RRTurn;
	relay[2] = !FLTurn;		// NC relay
	relay[3] = !FRTurn;		// NC relay
	relay[4] = !lowBeams;	// NC relay
	relay[5] = highBeams;
	
	// output to relays
	for (int i=0; i<6; i++) digitalWrite(i+22, !relay[i]);	// relays are active low
	
	// output LED array
	for (int i=0; i<8; i++)
	{
		digitalWrite(i, !led[i]);	// the lads are MSB to LSB
	}
	
	//output to in gear LED
	digitalWrite(17, inGearInd);
	
	ESC.writeMicroseconds(throttleOutput); // use throttleCurve when using a curve
	
	return;
}	 