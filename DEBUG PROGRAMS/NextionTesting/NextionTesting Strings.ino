/*
 * NextionTesting.ino
 *
 * Created: 3/3/2020 3:25:54 PM
 * Author: colef
 */ 
#include <Nextion.h>

//define some colour values for Nextion
#define BLACK   0x0000
#define BLUE    0x001F
#define RED     0xF800
#define GREEN   0x07E0
#define CYAN    0x07FF
#define LBLUE	0xAEBF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

// dont copy; variables from other routines needed to compile example:
boolean faultReset = 0;
// simulate three active faults, along with four fault messages available
int numberOfFaults = 3;
boolean faultFlags[4] = {1,1,1,0};
uint8_t faultLevel[4] = {3,2,2,1};
String faultMessages[4] =
{
	"fault 1",
	"fault 2",
	"fault 3",
	"fault 4"
};
boolean oneShotBits[96];	
uint8_t ONSTracker;		
float vBat = 48.3;
float currentDrawMotorAmps = 20.8;
int totalDraw_mAh = 12346;
int speed = 23;
boolean inGear;
float cellVolts[4] = {14.0, 13.9, 13.9, 14.2};
boolean firstScan;
boolean debuggingHMIActive = 1;
boolean debuggingActive = 0;


// START OF NEXTION VARIABLES

// Declare your Nextion objects - Example (page id = 0, component id = 1, component name = "b0")
NexText alarmView0		= NexText(0, 2, "alarmView0");
NexText vBat0			= NexText(0, 14, "vBat0");
NexText currentDis0		= NexText(0, 15, "currentDis0");
NexText powerDis0		= NexText(0, 16, "powerDis0");
NexText mAh0			= NexText(0, 17, "mAh0");
NexButton FLTRST0		= NexButton(0, 3, "FLTRST0");
NexText	speedDis0		= NexText(0, 4, "speedDispla0y");
NexText inGearInd0		= NexText(0, 1, "inGearInd0");
//NexButton STAT0			= NexButton(0, 5, "STAT0");
NexText clockDis		= NexText(0, 13, "clockDis");
NexProgressBar voltBar	= NexProgressBar(0, 9, "voltBar");
NexProgressBar currBar	= NexProgressBar(0, 10, "currBar");
NexProgressBar powerBar = NexProgressBar(0, 11, "powerBar");

NexText alarm0			= NexText(1, 1, "alarm0");
NexText alarm1			= NexText(1, 5, "alarm1");
NexText alarm2			= NexText(1, 6, "alarm2");
NexText alarm3			= NexText(1, 7, "alarm3");
NexText alarm4			= NexText(1, 8, "alarm4");
NexText alarm5			= NexText(1, 9, "alarm5");
NexText alarm6			= NexText(1, 10, "alarm6");
NexText alarm7			= NexText(1, 11, "alarm7");
NexText alarm8			= NexText(1, 12, "alarm8");
NexText alarm9			= NexText(1, 13, "alarm9");
NexButton FLTRST1		= NexButton(1, 4, "FLTRST1");
//NexButton STAT1			= NexButton(1, 3, "STAT1");
//NexButton MAIN1			= NexButton(1, 2, "MAIN1");

NexText alarmView2		= NexText(2, 2, "alarmView2");
NexButton FLTRST2		= NexButton(2, 3, "FLTRST2");
NexText vBat2			= NexText(2, 16, "vBat2");
NexText currentDis2		= NexText(2, 17, "currentDis2");
NexText powerDis2		= NexText(2, 18, "powerDis2");
NexText mAh2			= NexText(2, 19, "mAh2");
NexText speedDis2		= NexText(2, 4, "speedDis2");
NexText inGearInd2		= NexText(2, 15, "inGearInd2");
//NexButton MAIN2			= NexButton(2, 1, "MAIN");
NexText cell1v			= NexText(2, 22, "cell1v");
NexText cell2v			= NexText(2, 23, "cell2v");
NexText cell3v			= NexText(2, 24, "cell3v");
NexText cell4v			= NexText(2, 25, "cell4v");
NexPicture

// objects to listen for object ID presses
NexTouch *nex_listen_list[] = {
	//&alarmView0,
	&FLTRST0,
	//&STAT0,
	&FLTRST1,
	//&STAT1,
	//&MAIN1,
	//&alarmView2,
	&FLTRST2,
	//&MAIN2,
	NULL
};
	
// Nextion Variables for my functions
int nextionPage = 0;		// current page of nextion to refresh; functionality being added later
int activeFaultToDisplay = 0;
int hmiElemToUpd = 0;
int lasthmiElemUpd = 0;
String nexBuffer[30];
boolean nextionDelay = 0;

void setup()
{
	nexInit();
	Serial.begin(9600);
	firstScan = 1;
	
	// buttons get pointers, not buttons do not get pointers
	Serial.print("Attaching Nextion button calls...");
	//alarmView0.attachPop(alarmView0PopCallback);
	FLTRST0.attachPop(FLTRST0PopCallback, &FLTRST0);
	//STAT0.attachPop(STAT0PopCallback, &STAT0);
	FLTRST1.attachPop(FLTRST1PopCallback, &FLTRST1);
	//STAT1.attachPop(STAT1PopCallback, &STAT1);
	//MAIN1.attachPop(MAIN1PopCallback, &MAIN1);
	//alarmView2.attachPop(alarmView2PopCallback);
	FLTRST2.attachPop(FLTRST2PopCallback, &FLTRST2);
	//MAIN2.attachPop(MAIN2PopCallback, &MAIN2);
	Serial.println("Done.");
}

void loop()
{
	inGear = FlasherBit(1);
	ONSTracker = 0;
	HMIControl();
	if (firstScan) firstScan=0;
}

void HMIControl()
{
	long hmiStartTime = millis();

	//if (nexSerial.read() == 0x24 0xFF 0xFF 0xFF) nextionDelay = 1;
	//if (TON(nextionDelay, 200, timerTracker++)) nextionDelay = 0; // if Nextion buffer overloads, pause updates for 200ms
	
	if (oneShot(FlasherBit(4), ONSTracker++) && 0)
	{
		if (debuggingHMIActive) Serial.print("Starting listen for Nextion buttons...");
		nexLoop(nex_listen_list);
		if (debuggingHMIActive) Serial.println("Done.");
	}
	
	// cycleActiveFaults() MUST be called every cycle because it uses oneshot bits, so we need a buffer, 'j'
	String cycleActiveFaultsTemp = "yeet";
	
	int thisSpot = 0;
	int nextUpdate = oneShot(!FlasherBit(5), ONSTracker++);
	if (debuggingHMIActive && nextUpdate) Serial.print("Beginning update of element " + String(hmiElemToUpd) + "...");
	
	if (oneShot(nextionDelay, ONSTracker++)) Serial.println("Nextion overloaded, delaying serial.");
	else if	((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarmView0, cycleActiveFaultsTemp, 50)) hmiElemToUpd++;	// 0
/*	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarmView2, cycleActiveFaultsTemp, 50)) hmiElemToUpd++;	// 1
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm0, listAllFaults(0), 50)) hmiElemToUpd++; 			// 2
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm1, listAllFaults(1), 50)) hmiElemToUpd++;			// 3
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm2, listAllFaults(2), 50)) hmiElemToUpd++;			// 4
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm3, listAllFaults(3), 50)) hmiElemToUpd++;			// 5
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm4, listAllFaults(4), 50)) hmiElemToUpd++;			// 6
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm5, listAllFaults(5), 50)) hmiElemToUpd++;			// 7
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm6, listAllFaults(6), 50)) hmiElemToUpd++;			// 8
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm7, listAllFaults(7), 50)) hmiElemToUpd++;			// 9
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm8, listAllFaults(8), 50)) hmiElemToUpd++;			// 10
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(alarm9, listAllFaults(9), 50)) hmiElemToUpd++;			// 11
*/	
//	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(vBat0, String(vBat) + " V", 10)) hmiElemToUpd++;			// 12
/*	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(vBat2, String(vBat) + " V", 10)) hmiElemToUpd++;			// 13
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(cell1v, String(cellVolts[0]) + " V", 10)) hmiElemToUpd++;// 14
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(cell2v, String(cellVolts[1]) + " V", 10)) hmiElemToUpd++;// 15
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(cell3v, String(cellVolts[2]) + " V", 10)) hmiElemToUpd++;// 16
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(cell4v, String(cellVolts[3]) + " V", 10)) hmiElemToUpd++;// 17
*/	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (voltBar.setValue((int)map(vBat, 0, 96, 0, 100))) hmiElemToUpd++;			// 18
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(currentDis0, String(currentDrawMotorAmps) + " A", 10)) hmiElemToUpd++;	// 19
//	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(currentDis2, String(currentDrawMotorAmps) + " A", 10)) hmiElemToUpd++;	// 20
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (currBar.setValue((int)map(currentDrawMotorAmps, 0, 120, 0, 100))) hmiElemToUpd++;				// 21
	
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(powerDis0, String(round(currentDrawMotorAmps * vBat)) + " W", 10)) hmiElemToUpd++; // 22
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(powerDis2, String(round(currentDrawMotorAmps * vBat)) + " W", 10)) hmiElemToUpd++; // 23
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (powerBar.setValue((int)map(currentDrawMotorAmps * vBat, 0, 9000, 0, 100))) hmiElemToUpd++;		// 24
	
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(mAh0, String(totalDraw_mAh) + " mAh", 10)) hmiElemToUpd++;			// 25
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(mAh2, String(totalDraw_mAh) + " mAh", 10)) hmiElemToUpd++;			// 26
	
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(speedDis0, String(speed) + " km/h", 10)) hmiElemToUpd++;			// 27
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate) if (nexTextFromString(speedDis2, String(speed) + " km/h", 10)) hmiElemToUpd++;		// 28
	
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate)							// 29
	{
		if (inGear && nexTextFromString(inGearInd0, "D", 1)) 
		{
			inGearInd0.Set_font_color_pco(LBLUE);
		}
		if (!inGear && nexTextFromString(inGearInd0, "N", 1)) 
		{
			inGearInd0.Set_font_color_pco(GREEN);
		}
	}
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate)							// 30
	{
		if (inGear && nexTextFromString(inGearInd2, "D", 1))
		{
			inGearInd0.Set_font_color_pco(LBLUE);
			hmiElemToUpd++;
		}
		if (!inGear && nexTextFromString(inGearInd2, "N", 1))
		{
			inGearInd0.Set_font_color_pco(GREEN);
			hmiElemToUpd++;
		}
	}
	
	// ADD CLOCK UPDATE LOGIC FROM 'HMI clock test' STANDARD!!!!!!
	else if ((hmiElemToUpd == thisSpot++) && nextUpdate)							// 31
	{
		char clockBuffer[10];
		//sprintf(clockBuffer,"%02u:%02u:%02u",now.hour(),now.minute(),now.second());
		if (nexTextFromString(clockDis, "yeet", 10)) hmiElemToUpd++;
	}
	else if (hmiElemToUpd == thisSpot++)											// reset
	{
		Serial.println("\nResetting HMI elements counter from " + String(hmiElemToUpd));
		hmiElemToUpd = 0;
	}
	else Serial.println("error in hmi logic");
	
	if (debuggingHMIActive && nextUpdate) 
	{
		if (hmiElemToUpd != lasthmiElemUpd) Serial.println("Done.");
		else Serial.println("Failed.");
	}
	
	/*  BACKUP
	// TODO: update all the elements with what they want
	if (debuggingHMIActive) Serial.print("Starting listen for Nextion buttons...");
	nexLoop(nex_listen_list);
	if (debuggingHMIActive) Serial.println("Done.");
	//String nexBuffer;
	//static char bufferTen[10], bufferOne[1], bufferFifty[50], buffer500[500];
	
	// NOTE: cycleActiveFaults() has a oneShot in it; it MUST be called EVERY scan, or the processor will fault !!
	if (debuggingHMIActive) Serial.println("Updating elements on Nextion");
	
	nexTextFromString(alarmView0, cycleActiveFaults(), 50);
	nexTextFromString(alarmView2, cycleActiveFaults(), 50);
	if (debuggingHMIActive) Serial.println("Updated alarms Dis successfully.");
	
	nexTextFromString(vBat0, String(vBat) + " V", 10);
	nexTextFromString(vBat2, String(vBat) + " V", 10);
	nexTextFromString(cell1v, String(cellVolts[0]) + " V", 10);
	nexTextFromString(cell2v, String(cellVolts[1]) + " V", 10);
	nexTextFromString(cell3v, String(cellVolts[2]) + " V", 10);
	nexTextFromString(cell4v, String(cellVolts[3]) + " V", 10);
	voltBar.setValue(map(vBat, 0, 96, 0, 100));

	nexTextFromString(currentDis0, String(currentDrawMotorAmps) + " A", 10);
	nexTextFromString(currentDis2, String(currentDrawMotorAmps) + " A", 10);
	currBar.setValue(map(currentDrawMotorAmps, 0, 120, 0, 100));
	
	nexTextFromString(powerDis0, String(round(currentDrawMotorAmps * vBat)) + " W", 10);
	nexTextFromString(powerDis2, String(round(currentDrawMotorAmps * vBat)) + " W", 10);
	powerBar.setValue(map(currentDrawMotorAmps * vBat, 0, 9000, 0, 100));
	
	nexTextFromString(mAh0, String(totalDraw_mAh) + " mAh", 10);
	nexTextFromString(mAh2, String(totalDraw_mAh) + " mAh", 10);
	
	nexTextFromString(speedDis0, String(speed) + " km/h", 10);
	nexTextFromString(speedDis2, String(speed++) + " km/h", 10);
	
	if (debuggingHMIActive) Serial.println("Updating inGear on nextion");
	if (inGear)
	{
		nexTextFromString(inGearInd0, "D", 1);
		inGearInd0.Set_font_color_pco(LBLUE);
	}
	else
	{
		nexTextFromString(inGearInd2, "N", 1);
		inGearInd0.Set_font_color_pco(GREEN);
	}
	
	// ADD CLOCK UPDATE LOGIC FROM 'hmi clock test' STANDARD!!!!!!
	nexTextFromString(clockDis, "12:45", 10);
	if (debuggingHMIActive) Serial.println("Done updating nextion elements.");
	*/
	

	// hmiOvertimeLength = millis() - hmiStartTime;
	
}

// Button press functions from Nextion

/*void alarmView0PopCallback()
{
	nextionPage = 1;
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void STAT0PopCallback(void *ptr)
{
	nextionPage = 2;
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void STAT1PopCallback(void *ptr)
{
	nextionPage = 2;
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void MAIN1PopCallback(void *ptr)
{
	nextionPage = 0;
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void alarmView2PopCallback()
{
	nextionPage = 1;
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}
void MAIN2PopCallback(void *ptr)
{
	nextionPage = 0;
	if (debuggingActive) Serial.println("Page is now " + String(nextionPage));
}*/
void FLTRST0PopCallback(void *ptr)
{
	faultReset  = 1;
	if (debuggingActive) Serial.println("Fault reset issued");
}
void FLTRST1PopCallback(void *ptr)
{
	faultReset  = 1;
	if (debuggingActive) Serial.println("Fault reset issued");
}
void FLTRST2PopCallback(void *ptr)
{
	faultReset  = 1;
	if (debuggingActive) Serial.println("Fault reset issued");
}
	
// REMAINDER OF CALLS ARE ALREADY IN THE MAIN PROGRAM


boolean FlasherBit(float hz)
{
	int T = 1000.0 / hz;
	if ( millis() % T >= T/2 ) return 1;
	else return 0;
}

boolean oneShot(uint8_t precond, uint8_t OSR)
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

String cycleActiveFaults()
{
	if (oneShot(FlasherBit(0.5), ONSTracker++)) activeFaultToDisplay++;
	if (activeFaultToDisplay >= numberOfFaults) activeFaultToDisplay = 0;
	int j = 0;
	for (int i=0; i<sizeof(faultFlags); i++)
	{
		if (j == activeFaultToDisplay && faultFlags[i]) break;
		if (faultFlags[i]) j++;
	}
	return faultMessages[j];
}

boolean nexTextFromString(NexText elem, String input, int len)
{
	// function only sets the element text if a change is detected,
	// and returns 'true' if a change is detected
	if (sizeof(input) > len) input = "K";
	if (input != nexBuffer[hmiElemToUpd])
	{
		if (debuggingHMIActive) Serial.println("\nbuffer update on element " + String(hmiElemToUpd));
		nexBuffer[hmiElemToUpd] = input;
		char buffer[len];
		input.toCharArray(buffer, len);
		return elem.setText(buffer);
	}
	else return 0;
}

// TEMP, DONT COPY:
String listAllFaults(int i)
{
	return "yeet";
}