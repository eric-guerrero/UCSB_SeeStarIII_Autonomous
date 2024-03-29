/* 
 Sets the RTC alarm pin to wake up the Arduino every 10 seconds.
 Turns on, disables external power on the SleepyPi, turns off the LED, and puts the microcontroller
 into a low power mode. Wakes up and repeats this every 10 seconds.  For testing purposes only
 This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
 To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
 
 Modified by Eric Guerrero
*/
#include "SleepyPi2.h"
#include <Time.h>
#include <TimeLib.h>
#include <LowPower.h>
#include <PCF8523.h>
#include <Wire.h>

const char *monthName[12] = {
  "Jan", "Feb", "Mar", "Apr", "May", "Jun",
  "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
};

//const int LED_PIN = 13;
const int LED_RELAY_CONTROL_PIN = 8;
const int LED_PIN= 5;

 
// Setup the Periodic Timer 
eTIMER_TIMEBASE  PeriodicTimer_Timebase     = eTB_SECOND;   // e.g. Timebase set to seconds. Other options: eTB_MINUTE, eTB_HOUR
uint8_t          PeriodicTimer_Value        = 10;           // Timer Interval in units of Timebase e.g 10 seconds 

tmElements_t tm;


void alarm_isr()
{
    // Just a handler for the alarm interrupt.
    // You could do something here

}

void setup()
{ 
  // Configure "Standard" LED pin
  pinMode(LED_PIN, OUTPUT);    
  digitalWrite(LED_PIN,LOW);    // Switch off LED

  // initialize serial communication: In Arduino IDE use "Serial Monitor"
  Serial.begin(9600);
  Serial.println("Turning external power off");
  SleepyPi.enableExtPower(false);
  
  // initialize the digital pins as outputs 
  pinMode(led_relay_ctrl_pin, OUTPUT);    
  pinMode(led_ctrl_pin, OUTPUT);

  // turn off the led relay control
  // active high signal
  digitalWrite(led_relay_ctrl_pin, HIGH);   
  
  // active low signal
  digitalWrite(led_ctrl_pin, HIGH); 
  
  Serial.println("Starting, but I'm going to go to sleep for a while...");
  delay(50);
  SleepyPi.rtcInit(true);

  // Default the clock to the time this was compiled.
  // Comment out if the clock is set by other means
  // ...get the date and time the compiler was run
  if (getDate(__DATE__) && getTime(__TIME__)) {
      // and configure the RTC with this info
      SleepyPi.setTime(DateTime(F(__DATE__), F(__TIME__)));
  } 
  
  printTimeNow();   

  Serial.print("Periodic Interval Set for: ");
  Serial.print(PeriodicTimer_Value);
  switch(PeriodicTimer_Timebase)
  {
    case eTB_SECOND:
      Serial.println(" seconds");
      break;
    case eTB_MINUTE:
      Serial.println(" minutes");
      break;
    case eTB_HOUR:
      Serial.println(" hours");
    default:
        Serial.println(" unknown timebase");
        break;
  }
}

void loop() 
{
    SleepyPi.rtcClearInterrupts();

    // Allow wake up alarm to trigger interrupt on falling edge.
    attachInterrupt(0, alarm_isr, FALLING);    // Alarm pin

    // Set the Periodic Timer
    SleepyPi.setTimer1(PeriodicTimer_Timebase, PeriodicTimer_Value);

    delay(500);

    // Enter power down state with ADC and BOD module disabled.
    // Wake up when wake up pin is low.
    SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF); 

    // Disable external pin interrupt on wake up pin.
    detachInterrupt(0);
    
    SleepyPi.ackTimer1();

    // Do something here
    // Example: Read sensor, data logging, data transmission.
    // Just a handler for the pin interrupt.
    digitalWrite(LED_PIN,HIGH);    // Switch on LED
    Serial.println("I've Just woken up on a Periodic Timer!");
    // Print the time
    printTimeNow();   
    delay(50);
    digitalWrite(LED_PIN,LOW);    // Switch off LED   
}

// **********************************************************************
// 
//  - Helper routines
//
// **********************************************************************
void printTimeNow()
{
    // Read the time
    DateTime now = SleepyPi.readTime();
    
    // Print out the time
    Serial.print("Ok, Time = ");
    print2digits(now.hour());
    Serial.write(':');
    print2digits(now.minute());
    Serial.write(':');
    print2digits(now.second());
    Serial.print(", Date (D/M/Y) = ");
    Serial.print(now.day());
    Serial.write('/');
    Serial.print(now.month()); 
    Serial.write('/');
    Serial.print(now.year(), DEC);
    Serial.println();

    return;
}
bool getTime(const char *str)
{
  int Hour, Min, Sec;

  if (sscanf(str, "%d:%d:%d", &Hour, &Min, &Sec) != 3) return false;
  tm.Hour = Hour;
  tm.Minute = Min;
  tm.Second = Sec;
  return true;
}

bool getDate(const char *str)
{
  char Month[12];
  int Day, Year;
  uint8_t monthIndex;

  if (sscanf(str, "%s %d %d", Month, &Day, &Year) != 3) return false;
  for (monthIndex = 0; monthIndex < 12; monthIndex++) {
    if (strcmp(Month, monthName[monthIndex]) == 0) break;
  }
  if (monthIndex >= 12) return false;
  tm.Day = Day;
  tm.Month = monthIndex + 1;
  tm.Year = CalendarYrToTm(Year);
  return true;
}

void print2digits(int number) {
  if (number >= 0 && number < 10) {
    Serial.write('0');
  }
  Serial.print(number);
}
