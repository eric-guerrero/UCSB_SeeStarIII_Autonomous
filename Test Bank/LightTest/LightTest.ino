
/*
  Turns on SeeStar LED module on for 3 seconds, then off for one second, repeatedly. For testing purposes only.
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  
  Modified by Eric Guerrero
 */
#include "LowPower.h"
#include "SleepyPi2.h"

int LED_RELAY_CONTROL_PIN = 8;
int LED_CONTROL_PIN= 5;

// the setup routine runs once when you press reset:
void setup() {                
  Serial.begin(9600);
  pinMode(LED_RELAY_CONTROL_PIN, OUTPUT);    
  pinMode(LED_CONTROL_PIN, OUTPUT);  
  
  // ensure the LED relay is off
  digitalWrite(LED_CONTROL_PIN, HIGH); // set to HIGH initially; turns the LED on with a falling edge, e.g. HIGH->LOW  
  SleepyPi.enableExtPower(true);
  digitalWrite(LED_RELAY_CONTROL_PIN, LOW); 
  delay(100);   
  digitalWrite(LED_RELAY_CONTROL_PIN, HIGH); // set to HIGH initially; turns the relay on with a falling edge, e.g. HIGH->LOW     
  SleepyPi.enableExtPower(false);  
}

// the loop routine runs over and over again forever:
void loop() {    
  digitalWrite(LED_CONTROL_PIN, HIGH); // set to HIGH initially; turns the LED on with a falling edge, e.g. HIGH->LOW  
  SleepyPi.enableExtPower(true);
  digitalWrite(LED_RELAY_CONTROL_PIN, LOW); 
  delay(3000);   
  digitalWrite(LED_RELAY_CONTROL_PIN, HIGH); // set to HIGH initially; turns the relay on with a falling edge, e.g. HIGH->LOW     
  SleepyPi.enableExtPower(false);  
  delay(3000);   
}
