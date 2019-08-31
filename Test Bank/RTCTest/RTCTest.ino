/* 
 Reads the SleepyPi RTC and prints out the time in local and UTC format.
 Assumes the RTC is set to local time. For testing purposes only.
 This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
 To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
 
 Modified by Eric Guerrero
*/
 
#include <SleepyPi2.h> 
#include <TimeLib.h>
#include <Timezone.h> 
 
//US Pacific Time Zone
TimeChangeRule usLOCALDT = {"PDT", Second, Sun, Mar, 2, -420}; //dowSunday
TimeChangeRule usLOCALST = {"PST", First, Sun, Nov, 2, -480};
Timezone myTZ(usLOCALDT, usLOCALST);
DateTime date;

// reads the time from the RTC
time_t readt(void)
{ 
  DateTime t; 
  t = SleepyPi.readTime();
  return t.unixtime();
}

void setup(void)
{
  Serial.begin(9600); 
  Serial.println("Initializing"); 
  setSyncProvider(readt);   // pass the function to get the time from the RTC
  if(timeStatus()!= timeSet) 
      Serial.println("Unable to sync with the RTC");
  else
      Serial.println("RTC has set the system time");   
}

void loop(void)
{
    time_t utc, local;
    local = now(); 
    utc = myTZ.toUTC(local);
    DateTime b(utc);        

    if (myTZ.utcIsDST(utc)){
      printTime(local, usLOCALDT.abbrev);
      Serial.println("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||");
      printTime(utc, "UTC");
      Serial.println("||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||");


    }
    else{
      printTime(local, usLOCALST.abbrev);
      printTime(utc, "UTC");
    }
    delay(5000);
}

//Print time with time zone
void printTime(time_t t, char *tz)
{
    DateTime d(t);
    //= SleepyPi.readTime();
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
    Serial.print(tz);
    Serial.println();
} 

//Print an integer in "00" format (with leading zero).
void print2digits(int number) { 
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
} 
