  
#include <DS3231.h>
#include <Wire.h>

void setup() {
    // Setup Serial connection
  Serial.begin(115200);
  Wire.begin();
  // Uncomment the next line if you are using an Arduino Leonardo
  //while (!Serial) {}
  DS3231 rtc;
  // Initialize the rtc object
  
  // The following lines can be uncommented to set the date and time
  // rtc.setDOW(MONDAY);     // Set Day-of-Week to SUNDAY
  rtc.setDoW(7);
  rtc.setYear(20);
  rtc.setMonth(3);
  rtc.setDate(28);
  rtc.setHour(13);
  rtc.setMinute(20);
  rtc.setSecond(0);
  
  Serial.println("done");
}

void loop() {
  // put your main code here, to run repeatedly:
	delay(100);
}
