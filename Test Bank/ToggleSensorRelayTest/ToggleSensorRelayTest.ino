/*
  Turns on SeeStar Sensor module on for 7 seconds, then off for 1 second, repeatedly. For testing purposes only.
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  
  Modified by Eric Guerrero
 */
 
int sensor_pin = 9;

// the setup routine runs once when you press reset:
void setup() {                
  // initialize the digital pin as an output.
  pinMode(sensor_pin, OUTPUT);
  Serial.begin(9600);     
}

// the loop routine runs over and over again forever:
void loop() {
  digitalWrite(sensor_pin, HIGH);   // turn the Sensor module on (HIGH is the voltage level)
  Serial.println("High");
  delay(3000);               // wait for 3 seconds
  Serial.print("Low");
  digitalWrite(sensor_pin, LOW);    // turn the Sensor module by making the voltage LOW
  delay(1000);               // wait for 1 second
}
