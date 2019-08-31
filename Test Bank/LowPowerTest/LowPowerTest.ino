/*
  Loop to test baseline power consumption with no camera turned on in SeeStar module.
  Turns on, disables external power on the SleepyPi, turns offs all relays and and puts 
  the microcontroller into a low power mode. Repeats this every 8 seconds.  For testing 
  purposes only.
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  
  Modified by Eric Guerrero
 */
#include "LowPower.h"
#include "SleepyPi2.h"
   
const int LED_PIN = 13;
const int LED_RELAY_CONTROL_PIN = 8; 

// the setup routine runs once when you press reset
void setup() {                
  Serial.begin(9600);
  Serial.println("Initialized");
  SleepyPi.enableExtPower(false);

  // turn off the relay
  pinMode(LED_RELAY_CONTROL_PIN, OUTPUT);   
  
  // turn off the board LED
   pinMode(LED_PIN,OUTPUT);
}

void loop() 
{   
  Serial.println("Power down");
  delay(2000); 
  LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF); 
}
