/* 
	Editor: https://www.visualmicro.com/
			visual micro and the arduino ide ignore this code during compilation. this code is automatically maintained by visualmicro, manual changes to this file will be overwritten
			the contents of the _vm sub folder can be deleted prior to publishing a project
			all non-arduino files created by visual micro and all visual studio project or solution files can be freely deleted and are not required to compile a sketch (do not delete your own code!).
			note: debugger breakpoints are stored in '.sln' or '.asln' files, knowledge of last uploaded breakpoints is stored in the upload.vmps.xml file. Both files are required to continue a previous debug session without needing to compile and upload again
	
	Hardware: ATmega2560 (Mega 2560) (Arduino Mega), Platform=avr, Package=arduino
*/

#define __AVR_ATmega2560__
#define ARDUINO 108012
#define ARDUINO_MAIN
#define F_CPU 16000000L
#define __AVR__
#define F_CPU 16000000L
#define ARDUINO 108012
#define ARDUINO_AVR_MEGA2560
#define ARDUINO_ARCH_AVR
//
//
void MapInputs(boolean IOUpdate);
void MapOutputs(boolean IOUpdate);
void LightRoutine();
void DriveSystem();
void MonitorSystem();
void HMIControl();
void Faults();
void debugRoutine();
void alarmView0Callback();
void alarmView2Callback();
void STAT0Callback();
void STAT1Callback();
void STAT3Callback();
void MAIN1Callback();
void MAIN2Callback();
void MAIN3Callback();
void FLTRST0Callback();
void FLTRST1Callback();
void FLTRST2Callback();
void settings0Callback();
void settings2Callback();
void sportCallback();
void standardCallback();
void economyCallback();
boolean FlasherBit(float hz);
boolean oneShot(boolean precond, uint8_t OSR);
boolean toggleState(boolean precond, boolean toggled, uint8_t OSR);
float voltsFromAnalogIn (int input);
int voltsToAnalogIn (float input);
int voltsToAnalogOut(float input);
boolean TON(boolean input, int preset, int timerNumber);
boolean limit(float input, float lower, float upper);
String listAllFaults(int spot);
String cycleActiveFaults();
boolean nexTextFromString(String objName, String input, int len);
boolean nexBar(String objName, int val);
boolean nexCheckbox(String objName, boolean val);
boolean nexSetFontColor(String objName, uint32_t number);
boolean nexSetBackColor(String objName, uint32_t number);
boolean nexSetPicture(String objName, int val);
boolean nexCheckButtonPress(uint8_t recv[3]);
boolean nexRead();
boolean nexBuffer(String input);
void clearnexBuffer();
String currentTime();
String currentDate();
void clockUpdate();
boolean chkUpdElem(int page);
String secondsToClock(int input);

#include "pins_arduino.h" 
#include "arduino.h"
#include "ElectricMotorcycleMainProcessor.ino"
