/*
UP NEXT:
 - Make values retentive, stored and recovered using EEPROM
 - values will be compressed into a DWORD, and stored in EEPROM
 - in setup, DWORD will be uncompressed and assigned to each value
 - whenever values change, values will be re-compressed into a DWORD and stored in EEPROM
 
 
 todo:
 - test eeprom logic
 - fix read and write logic, it wont compile
 - commands all compile, but most still need extensive testing
*/

#include <EEPROM.h>

boolean oneShotBits[32];

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
	boolean CLIDebug = 1;
	
	// POINTERS FOR CLI LOGIC
	boolean *cliParamPtrs[32] = 
	{
		&nextionEnabled,
		&contactorSafetiesActive,
		&faultsOnSerial,
		&debuggingActive,
		&debuggingHMIActive,
		&scanTimeDebugEnabled,
		&faultsDebuggingActive,
		&driveSystemDebuggingActive,
		&analogDebuggingActive,
		&speedDebugActive,
		&clockDebugActive,
		&motorControlDebugEnable,
		&CLIDebug
	};
	
	String cliParams[32] = 
	{
		"nextionEnabled",
		"contactorSafetiesActive",
		"faultsOnSerial",
		"debuggingActive",
		"debuggingHMIActive",
		"scanTimeDebugEnabled",
		"faultsDebuggingActive",
		"driveSystemDebuggingActive",
		"analogDebuggingActive",
		"speedDebugActive",
		"clockDebugActive",
		"motorControlDebugEnable",
		"CLIDebug",
		""	// last one needs to be defined as "" for list command to work properly
	};

String commands[16] = 
{
	"help",				// 0
	"?",				// 1
	"set",				// 2
	"getVal",			// 3
	"listVal",				// 4
	"HIGH",				// 5
	"LOW",				// 6
	"restoreDefaults",					// 7
	"allDebugOn",					// 8
	"",					// 9
	"",					// 10
	"",					// 11
	"",					// 12
	"",					// 13
	"",					// 14
	"",					// 15
};
String commandDescriptions[16] = 
{
	"list all available commands",				// 0
	"list all available commands",				// 1
	"designate a value to a setting or variable",				// 2
	"display the status of a specific variable",				// 3
	"list all variables that can be changed",					// 4
	"Set LED-internal to high",					// 5
	"Set LED-internal to low",					// 6
	"Reset all debug options to default settings",					// 7
	"Turn on ALL debug options",					// 8
	"",					// 9
	"",					// 10
	"",					// 11
	"",					// 12
	"",					// 13
	"",					// 14
	"",					// 15
};



void setup(void)
{
	Serial.begin(115200);
	pinMode(LED_BUILTIN, OUTPUT);
	digitalWrite(LED_BUILTIN, HIGH);
}

void loop(void)
{
	//if (Serial.available()) chkCommand(Serial.readString());
	if (Serial.available()) chkCommand(recvSerial());
	
	delay(10);
}

String recvSerial()
{
	if (CLIDebug) Serial.print("Serial message...");
	char buffer[30] = "";
	for (int n=0; Serial.available() > 0; n++)
	{
		buffer[n] = Serial.read();
		delay(1);
	}
	if (CLIDebug) Serial.println("Done.");
	if (CLIDebug) Serial.println("Msg was '" + String(buffer) + "'");
	return String(buffer);
}

boolean chkCommand(String input)
{
	
	// input string contains raw CLI input
	// this code separates input into terms, using spaces to mark new terms
	int n = 0;
	char buffer[30];
	input.toCharArray(buffer, 30); // just an arbitrary length lel
	String term[3] = {"","",""};
	for (int i=0; i<input.length(); i++)
	{
		if (buffer[i] == 0x20) n++;	// 0x20 is ASCII space
		else term[n] += buffer[i];	// inefficient; use character arrays for concatentation instead of strings next time big boy
		// string redeclares every time you add a character
	}
	
	// give the terms usable names
	String *cmd		= term;
	String *param	= term+1;
	String *val		= term+2;
	
	if (CLIDebug)	
	{
		Serial.println("term[0] is " + *cmd);
		Serial.println("term[1] is " + *param);
		Serial.println("term[2] is " + *val);
	}
	
	if (*cmd == "HIGH")
	{
		digitalWrite(LED_BUILTIN, HIGH);
		if (CLIDebug) Serial.println("HIGH");
	}
	else if (*cmd == "LOW")
	{
		digitalWrite(LED_BUILTIN, LOW); 
		if (CLIDebug) Serial.println("LOW");
	}
	else if (*cmd == "help" || cmd == "?")
	{
		if (CLIDebug) Serial.println("Listing all commands:");
		for (int i=0; commands[i]!="" && i<sizeof(commands); i++) 
		{
			Serial.println(commands[i] + "\t- " + commandDescriptions[i]);
		}
	}
	else if (*cmd == "set")
	{
		if (setParam(*param, *val)) { writeCLISettings(); if (CLIDebug) Serial.println("Successfully set " + *param + " to " + *val); }
		else if (CLIDebug) Serial.println("Failed to set parameter.");
	}
	else if (*cmd == "getVal")	// display current value of a specific parameter
	{
		for (int i=0; i<sizeof(cliParams); i++)
		{
			if (*param == cliParams[i]) Serial.println(String(cliParams[i]) + " = " + String(*cliParamPtrs[i]));
		}
	}
	else if (*cmd == "listVal")	// list all parameters
	{
		for (int i=0; cliParams[i]!="" && i<sizeof(cliParams); i++)
		{
			Serial.println(String(cliParams[i]) + " = " + String(*cliParamPtrs[i]));
		}
	}
	else if (*cmd == "restoreDefaults")
	{
		restoreDefaults();
	}
	else if (*cmd == "allDebugOn")
	{
		allDebugOn();
	}
	else 
	{
		Serial.println("Unrecognized command. Type 'help' or '?' for a list of commands.");
		return 0;
	}
	return 1;
}

boolean setParam(String param, String val)
{
	for (int i=0; i<sizeof(cliParams); i++)
	{
		if (param == cliParams[i])
		{
			if (val == "1" || val == "true") { *cliParamPtrs[i] = 1; return 1; }
			if (val == "0" || val == "false") { *cliParamPtrs[i] = 0; return 1; }
			Serial.println("Invalid value");
		}
	}
	Serial.println("Invalid parameter");
	return 0;
}

boolean readCLISettings()
{
	double readBuf = eeprom_read_dword(0);
	for (int i=0; i<32; i++)
	{
		if (readBuf % 2 == 1) *cliParamPtrs[i] = 1;
		else *cliParamPtrs[i] = 0;
		readBuf = readBuf >> 2;
	}
}

boolean writeCLISettings()
{
	double writeBuf = 0;
	for (int i=0; i<32; i++)
	{
		writeBuf += *cliParamPtrs[31-i];	// settings are stored as LIFO
		writeBuf = writeBuf << 2;
	}
}

boolean restoreDefaults()
{
		// settings that affect vehicle behavior
		nextionEnabled				= 1;
		contactorSafetiesActive		= 1;
		faultsOnSerial				= 1;
		debuggingActive				= 1;
		// settings that affect debug messages
		debuggingHMIActive			= 0;
		scanTimeDebugEnabled		= 0;
		faultsDebuggingActive		= 0;
		driveSystemDebuggingActive	= 0;
		analogDebuggingActive		= 0;
		speedDebugActive			= 0;
		clockDebugActive			= 0;
		motorControlDebugEnable		= 0;
		CLIDebug					= 0;
		Serial.println("Default debugging settings restored.");
}

boolean allDebugOn()
{		
	// settings that affect vehicle behavior
	nextionEnabled				= 1;
	contactorSafetiesActive		= 1;
	faultsOnSerial				= 1;
	debuggingActive				= 1;
	// settings that affect debug messages
	debuggingHMIActive			= 1;
	scanTimeDebugEnabled		= 1;
	faultsDebuggingActive		= 1;
	driveSystemDebuggingActive	= 1;
	analogDebuggingActive		= 1;
	speedDebugActive			= 1;
	clockDebugActive			= 1;
	motorControlDebugEnable		= 1;
	CLIDebug					= 1;
	
	Serial.println("All debugging options toggled on.");
	}



// REUSED FUNCTIONS
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
