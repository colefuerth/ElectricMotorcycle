
boolean oneShotBits[32];



boolean CLIDebug = 1;

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
		//Serial.write((int)buffer[n]);
		delay(1);
	}
	if (CLIDebug) Serial.println("Done.");
	if (CLIDebug) Serial.println("Msg was '" + String(buffer) + "'");
	return "\n" + String(buffer);
}

boolean chkCommand(String cmd)
{
	if (cmd == "HIGH") 
	{
		digitalWrite(LED_BUILTIN, HIGH);
		if (CLIDebug) Serial.println("HIGH");
	}
	else if (cmd == "LOW") 
	{
		digitalWrite(LED_BUILTIN, LOW);
		if (CLIDebug) Serial.println("LOW");
	}
	else return 0;
	return 1;
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
