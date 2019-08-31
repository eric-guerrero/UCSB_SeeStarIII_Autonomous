/*
  Turns on SeeStar Camera, takes a 30 second video clip, delays 5 seconds, then repeats. For testing purposes only.
  Note - THE CAMERA needs to be in VIDEO RECORDING MODE, and have the Focus Mode set to Manual Focus MF
  Set from the phone app PlayMemories Mobile
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  
  Modified by Eric Guerrero
 */ 
//
int CAMERA_POWER_ON_OFF_PIN = 6;
int CAMERA_ON_OFF_PIN = 14; 
int CAMERA_VIDEO_PIN = 13;

// the setup routine runs once when you press reset:
void setup() {
  Serial.begin(9600);

  Serial.println("Initialized");
  // initialize the digital pin as an output
  pinMode(CAMERA_POWER_ON_OFF_PIN, OUTPUT);     
  pinMode(CAMERA_ON_OFF_PIN, OUTPUT); 
  pinMode(CAMERA_VIDEO_PIN, OUTPUT);
  
  // active low signals
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH);
  digitalWrite(CAMERA_VIDEO_PIN, HIGH);    
  
  // active high signal
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);  
  
}

// the loop routine runs over and over again forever:
void loop() {
 
  Serial.println("Starting"); 
  Serial.println("Camera DC-DC on");
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, HIGH);
     
  Serial.println("Delay for DC-DC activiation"); 
  delay(3000); 

  // turn on the camera 
  digitalWrite(CAMERA_ON_OFF_PIN, LOW); 
  delay(50); 
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH); 
  Serial.println("Camera on");
  Serial.println("Waiting 15 seconds for camera to boot...");
  delay(15000); 
  
  // turn video recording on 
  digitalWrite(CAMERA_VIDEO_PIN, LOW);
  delay(50); 
  Serial.println("Video recording on");
  digitalWrite(CAMERA_VIDEO_PIN, HIGH);  
 
  Serial.println("Delay 30 seconds");
  delay(30000);
  
  // turn video recording off 
  Serial.println("Video recording off");
  digitalWrite(CAMERA_VIDEO_PIN, LOW);
  delay(50); 
  digitalWrite(CAMERA_VIDEO_PIN, HIGH);
  
  Serial.println("Delay 120 seconds");
  delay(12000);
  
  // turn off the camera
  Serial.println("Camera off");
  digitalWrite(CAMERA_ON_OFF_PIN, LOW);
  delay(50); 
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH);
  
  Serial.println("Camera DC-DC off");
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);   
  Serial.println("Delay 5 seconds");  
  delay(5000);
}
