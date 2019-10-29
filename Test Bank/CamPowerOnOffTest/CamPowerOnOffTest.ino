/*
  Turns on SeeStar DC-DC converter which applies power to the SeeStar camera then repeats.
  For testing purposes only
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  Modified by Eric Guerrero
 */   
const int CAMERA_POWER_ON_OFF_PIN = 6;

// the setup routine runs once when you press reset:
void setup() {              
  // initialize the digital pin as an output.
  pinMode(CAMERA_POWER_ON_OFF_PIN, OUTPUT);     
  // active high signal
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);
  Serial.begin(9600);    
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, HIGH);
}

// the loop routine runs over and over again forever:
void loop() {            
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, HIGH);    // pull high - this turns the DC/DC converter on
  delay(6000);                // wait 2 seconds, turn off  
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);
  delay(6000);                // wait 6 seconds and repeat
  
}
