// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       salve_board.ino
    Created:	2019-12-11 5:32:41 PM
    Author:     DESKTOP-SAM2OE0\colef
*/

// Define User Types below here or use a .h file
//


// Define Function Prototypes that use User Types below here or use a .h file
//

#include "SerialTransfer.h"

SerialTransfer myTransfer;

int ledPin[8];


// Define Functions below here or use other .ino or cpp files
//

// The setup() function runs once each time the micro-controller starts
void setup()
{
	Serial1.begin(9600);
	myTransfer.begin(Serial1);
	
	// declare ledPin values
	for (int i=0; i<8; i++)
	{
		ledPin[i] = i;
	}
	ledPin[0] = 15;
	ledPin[1] = 14;
	
	// set led pins as outputs
	for (int i=0; i<8; i++)
	{
		pinMode(ledPin[i], OUTPUT);		// led1
	}
	

}

// Add the main program code into the continuous loop() function
void loop()
{
	
	boolean led[8];
	int serialInput, throttleDisplay;
	
	// RECIEVING CODE
	
	 if(myTransfer.available())
	 {
		 for(byte i = 0; i < myTransfer.bytesRead; i++)
		 {
			serialInput = myTransfer.rxBuff[i];
		 }
	 }
	 else if(myTransfer.status < 0)
	 {
		 Serial.print("ERROR: ");
		 Serial.println(myTransfer.status);
	 }
	 
	 // HANDLE DATA
	
	throttleDisplay = serialInput;
	
	int voltageReturn = map(throttleDisplay, 0, 255, 0, 100);
	
	for (int i=0; i<8; i++)
	{	// relay the input read from 0-5V as 0-254 on LED array
		if (throttleDisplay % 2 == 1) led[7-i] = 1;		// output LSB to current LED; leds display MSB to LSB, hence the [7-i]
		else led[7-i] = 0;
		throttleDisplay = throttleDisplay >> 1;			// BSR
	}

	// output LED array
	for (int i=0; i<8; i++)
	{
		digitalWrite(ledPin[i], led[i]);	
	}
	
	// TRANSMIT RETURN DATA

	myTransfer.txBuff[0] = voltageReturn;
	myTransfer.sendData(1);
	// delay(10);
	
	

}
