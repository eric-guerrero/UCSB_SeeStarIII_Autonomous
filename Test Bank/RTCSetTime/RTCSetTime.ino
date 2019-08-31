
/* 
 Sets the clock in response to serial port time message.
 To use, open the Arduino Serial Monitor and type the new time in the format:
 
 hour(24-format):minute:second month day year

 e.g.
 
  23:12:19  12 3 2017

 This will set the real time clock to the time 23:12:19 and the date 12-3-2017
 
 This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
 To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
 
 Modified by Eric Guerrero
*/

#include "SleepyPi2.h" //Used to talked with SleepyPi2 rtc time
#include <TimeLib.h>  //To verify what the system time sync now() reads
#include <Time.h> //Additional functionality
#include <Wire.h> // Allows you to communicate with I2C / TWI devices: SleepyPi2
#include <PCF8523.h>  //DateTime & RTC object
//#include <Timezone.h> 


PCF8523 rtc;
tmElements_t tm;
DateTime b;

// reads the time from the RTC
time_t readt(void)
{ 
  DateTime t; 
  t = SleepyPi.readTime();
  return t.unixtime();
}

void setup()  {
  Serial.begin(9600); 
  delay(100);
  setSyncProvider(readt);   // the function to get the time from the RTC
  if (timeStatus() != timeSet) 
     Serial.println("Unable to sync with the RTC");
  else
     Serial.println("RTC has set the system time"); 
  delay(100);      
}

void loop()
{

if (Serial.available()) {
  processSyncMessage();
  delay(100);
  rtc.setTime(b);
  setTime(b.unixtime());
  delay(100);
}

  digitalClockDisplay();
  delay(1000);
}

void digitalClockDisplay(){ 
    DateTime d = SleepyPi.readTime();
    Serial.print(d.hour(), DEC);
    Serial.print(':');
    Serial.print(d.minute(), DEC);
    Serial.print(':');
    Serial.print(d.second(), DEC);
    Serial.print(' ');
    Serial.print(d.month(), DEC);
    Serial.print('/');
    Serial.print(d.day(), DEC);
    Serial.print('/');
    Serial.print(d.year(), DEC);   
    Serial.println();
}

void printDigits(int digits)
{
  // utility function for digital clock display: prints preceding colon and leading 0
  Serial.print(":");
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
} 

unsigned long processSyncMessage() 
{ 
  long seconds = 0L;
  char buffer[32]; 
  String s = Serial.readStringUntil('\n');
  s.toCharArray(buffer, 40);
  if(getTime(buffer) == true) 
  return seconds;
}

bool getTime(const char *str)
{
  //Maybe remove NULL check?
  //Add in test case to return false 
  boolean pass = false;
  int Hour, Min, Sec, Day, Year, Month;
  char *temp = str;
  char * pch;
  int date [6] = {Hour, Min, Sec, Day, Year, Month};
  int i = 0;
  pch = strtok (str," :,.-");
  while (pch != NULL)
  {
    date[i] = atoi(pch);
    pch = strtok (NULL, " -,/:"); 
    i++;
  }

  tm.Hour = date[0];  tm.Minute = date[1];  tm.Second = date[2];  tm.Month = date[3]; tm.Day = date[4];

  //Only way that Arduino will allow the Year assignment
  uint16_t save = date[5];
  tm.Year = save; 
  
  DateTime tempDate (save,tm.Month,tm.Day,tm.Hour,tm.Minute,tm.Second);
  b = tempDate;
   
  return true;
}
