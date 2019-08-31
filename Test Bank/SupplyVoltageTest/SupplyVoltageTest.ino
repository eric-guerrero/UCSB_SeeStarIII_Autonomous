/* 
 Reads the SleepyPi voltage and prints out the voltage followed by a delimeter $.
 When voltage falls below the POWER_OFF_VOLTAGE, prints out message voltage ot too low. 
 For testing purposes only.
 This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
 To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
 
 Modified by Eric Guerrero
*/
 
#include "SleepyPi2.h" 
char DELIMETER='$';

#define POWER_OFF_VOLTAGE   12.6

void setup() {
  // Open serial communications and wait for port to open:
  Serial.begin(9600);  
}

void loop() {  
  delay(1000);  // voltage reading is artificially high if we don't delay first 
  float supply_voltage = SleepyPi.supplyVoltage();
  Serial.print(supply_voltage);
  Serial.println(DELIMETER); 
  if(supply_voltage < POWER_OFF_VOLTAGE)
     Serial.println("Power too low "); 
  delay(1000); 
}
