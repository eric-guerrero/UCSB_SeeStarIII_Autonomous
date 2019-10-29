/*
  Turns on/off external 3.3/5V power on a button press. For testing purposes only
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  
  Modified by Eric Guerrero
*/
 
#include "SleepyPi2.h"
#include <Time.h>
#include <LowPower.h>
#include <PCF8523.h>
#include <Wire.h>

// States
typedef enum {
  eWAIT = 0,
  eBUTTON_PRESSED,
  eBUTTON_RELEASED
}eBUTTONSTATE;

typedef enum {
   eEP_OFF = 0, 
   eEP_ON
}eEPSTATE;

const int LED_PIN = 8;//13

volatile bool  buttonPressed = false;
eBUTTONSTATE   buttonState = eBUTTON_RELEASED;
eEPSTATE       ep_state = eEP_OFF; 

void button_isr()
{
    // A handler for the Button interrupt.
    buttonPressed = true;
}
 
void setup()
{  
  // Configure "Standard" LED pin
  pinMode(LED_PIN, OUTPUT);    
  digitalWrite(LED_PIN,LOW);    // Switch off LED
 
  SleepyPi.enableExtPower(true); 
  
   // Allow wake up triggered by button press
  attachInterrupt(1, button_isr, LOW);    // button pin 
  
  // initialize serial communication: In Arduino IDE use "Serial Monitor"
  Serial.begin(9600);
  
  SleepyPi.rtcInit(true);
  
}

void loop() 
{ 
    // Enter power down state with ADC and BOD module disabled.
    // Turn on external power when button is pressed.
    // Once button is pressed again, turn off external power 
    float current;
    
    // Button State changed
    if(buttonPressed == true){
        detachInterrupt(1);      
        buttonPressed = false;  
        switch(buttonState) { 
          case eBUTTON_RELEASED:
              // Button pressed
              if (ep_state == eEP_ON) {
                // Switch on the external power 
                SleepyPi.enableExtPower(false);
                ep_state = eEP_OFF;
                Serial.println("Turning external power off");
              }
              else {
                // Switch off the external power 
                SleepyPi.enableExtPower(true);
                Serial.println("Turning external power on");
                ep_state = eEP_ON;                
              }
              buttonState = eBUTTON_PRESSED;
              digitalWrite(LED_PIN,HIGH);           
              attachInterrupt(1, button_isr, HIGH);                    
              break;
          case eBUTTON_PRESSED:  
              buttonState = eBUTTON_RELEASED; 
              digitalWrite(LED_PIN,LOW);            
              attachInterrupt(1, button_isr, LOW);    
              // button pin       
              break;
           default:
              break;
        }                
    }
}
