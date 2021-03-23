// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
    Name:       control_board.ino
    Created:	2019-12-11 5:17:16 PM
    Author:     DESKTOP-SAM2OE0\colef
*/

// Define User Types below here or use a .h file
//


// Define Function Prototypes that use User Types below here or use a .h file
//


// Define Functions below here or use other .ino or .cpp files
//

#include <Wire.h>
#include "SerialTransfer.h"
SerialTransfer myTransfer;

int potentiometer, feedback;
int potPin = A7;

// The setup() function runs once each time the micro-controller starts
void setup()
{
	// Serial3.begin(9600);	// serial communications with receiver
	Serial.begin(9600);		// serial communications with computer
	
	Serial3.begin(9600);
	myTransfer.begin(Serial3);
	
	pinMode(LED_BUILTIN, OUTPUT);
}

// Add the main program code into the continuous loop() function
void loop()
{
	// read potentiometer input
	potentiometer = analogRead(potPin);
	
	// TRANSMIT CODE
	
	myTransfer.txBuff[0] = potentiometer;	
		
	myTransfer.sendData(1);
	delay(10);
	
	if (Serial.available()){
		// Serial.println("feedback " + feedback);
		Serial.println("local " + potentiometer);
	}
	
	// RECIEVE CODE
	
	if(myTransfer.available())
	{
		Serial.println("New Data: ");
		for(byte i = 0; i < myTransfer.bytesRead; i++)
		Serial.write(myTransfer.rxBuff[i]);
		Serial.println();
	}
	else if(myTransfer.status < 0)
	{
		Serial.print("ERROR: ");
		Serial.println(myTransfer.status);
	}


	/*
	DIVORCE
	Andrew's code he he
	
	int i = 0;
	int arr[SIZE];
	int _read;
	while ((_read = Serial3.read()) != -1)
	{
		arr[i++] = _read;
		if (i > SIZE)
		{
			Serial.println("Too much data in Stream Serial3!");
			break;	
		}
	}
	
	*/

}
