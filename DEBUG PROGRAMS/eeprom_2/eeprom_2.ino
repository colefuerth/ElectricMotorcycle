/*
 * EEPROM_Testing.ino
 *
 * Created: 1/22/2020 6:48:16 PM
 * Author: colef
 */ 


/*
	NOTES ON EEPROM
	- eeprom works like siemens memory; you have to manually increment the pointer to the new data type
	- cannot overlap data or it gets scrambled, even when only writing zeros
	- byte is 1 spot; word is 2, dword is 4
*/

#include <EEPROM.h>

boolean not_zero_flag = 0;

void setup()
{
	/*
	int temp[20];
	Serial.begin(9600);
	// eeprom_read_float(1024);
	Serial.println("Starting byte write");
	for (int i=0; i<4095; i++) {
		if (i%400==0) Serial.print("|");
		eeprom_write_byte(i, 0);
	}
	Serial.println("Starting word write");
	for (int i=0; i<2048; i++)
	{
		eeprom_write_word(0, i);
	}
	Serial.println();
	Serial.println("Starting byte read, checking for !0");
	for (int i=0; i<4095; i++)
	{
		if (eeprom_read_byte(i) != 0) {
			not_zero_flag = 1;
			break;
		}
	}
	Serial.println("Done");
	Serial.println("not_zero_flag = " + (String)not_zero_flag);
	Serial.println("First byte is " + (String)eeprom_read_byte(0));
	Serial.println("First word is " + (String)eeprom_read_word(0));
	Serial.println("First dword is " + (String)eeprom_read_dword(0));
	*/
	float initialOdometer = 10;
	float odometerTune = 0.66;
	float vBatTune = 1;
	float shuntTune = 1;
	// EEPROM setup
	uint8_t	eepromAddress[5] = {0,4,8,12,16};
	float		eepromBuffer[5];
	float		*odometer = eepromBuffer,
				*odometerTune = eepromBuffer + 1,
				*vBatTune = eepromBuffer + 2,
				*shuntTune = eepromBuffer + 3,
				*longestCycleEEPROM = eepromBuffer + 4;
	for (int i=0; )
	
	Serial.println((String)eeprom_read_float(0));
	Serial.println((String)eeprom_read_float(4));
	Serial.println((String)eeprom_read_float(8));
	Serial.println((String)eeprom_read_float(12));
	
	
	
	pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
	if (not_zero_flag) digitalWrite(LED_BUILTIN, FlasherBit(1));
	else digitalWrite(LED_BUILTIN, FlasherBit(10));
	delay(25);
}

boolean FlasherBit(float hz)
{
	int T = 1000.0 / hz;
	if ( millis() % T >= T/2 ) return 1;
	else return 0;
}
