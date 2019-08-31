/*
  Simple serial based control to test the SeeStar Camera. Use with the Arduino Serial Monitor.
  Press D to turn on/off DC/DC converter on that enabled power to the camera
  Press L to set delays in milliseconds for shutter and video pulse, e.g. L,1000,100,  to set a 1000 msec for shutter and 100 for video
  Press T to toggle the camera power on/off
  Press S to shutter the camera
  Press F to focus and shutter the camera
  Press V to toggle the camera video 
  Press C to execute the entire control sequence to take a picture, which is
  1. turn DC-DC converter on 2. delay 3. turn camera power on 4. delay for bootup 
  5. shutter camera 6. delay for shutter activation and reccording, and lastly
  7. turn camera off 8. turn DC-DC converter off
  Press M to execute the entire control sequence to record a video, which is
  1. turn DC-DC converter on 2. delay 3. turn camera power on 4. delay for bootup 
  5. shutter camera 6. delay for video activation and reccording, and lastly
  7. turn camera off 8. turn DC-DC converter off
  
  For testing purposes only.
  This work is licensed under a Creative Commons Attribution-ShareAlike 4.0 License.
  To view a copy of this license, see http://creativecommons.org/licenses/by-sa/4.0
  
  Modified by Eric Guerrero
 */
 
#include "SleepyPi2.h" 

const int CAMERA_POWER_ON_OFF_PIN = 6;
const int CAMERA_ON_OFF_PIN = 14;
const int CAMERA_SHUTTER_PIN = 12; 
const int CAMERA_FOCUS_PIN = 15; 
const int CAMERA_VIDEO_PIN = 13;

boolean camera_on = false;
boolean video_on = false;
boolean power_on = false;
const int NUM_DELAY_FIELDS = 2; // delays for video and shutter
int delays_msec[NUM_DELAY_FIELDS] = {4000, 100};
int field_index = 0;

String help="\r\nPress D to turn on/off DC/DC converter on that enabled power to the camera\r\nPress L to set delays in milliseconds for shutter and video pulse, e.g. L,300,100,  to set a 300 msec for shutter and 100 for video\r\nPress T to toggle the camera power on/off\r\nPress S to shutter the camera\r\nPress F to focus and shutter the camera\r\nPress V to toggle the camera video\r\nPress C to execute the entire control sequence to take a picture\r\nPress M to execute the entire control sequence to record a video";

// the setup routine runs once when you press reset:
void setup() {  
  Serial.begin(9600);
  Serial.println("Initialized");
  Serial.println(help);
  pinMode(CAMERA_ON_OFF_PIN, OUTPUT); 
  pinMode(CAMERA_SHUTTER_PIN, OUTPUT); 
  pinMode(CAMERA_FOCUS_PIN, OUTPUT); 
  pinMode(CAMERA_POWER_ON_OFF_PIN, OUTPUT);     
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH); 
  digitalWrite(CAMERA_SHUTTER_PIN, HIGH); 
  digitalWrite(CAMERA_FOCUS_PIN, HIGH);
  digitalWrite(CAMERA_VIDEO_PIN, HIGH);
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);  
}

void toggleCameraVoltage()
{
  if (power_on == false) {
    Serial.println("Camera DC-DC on"); 
    digitalWrite(CAMERA_POWER_ON_OFF_PIN, HIGH); 
    power_on = true;
  }
  else { 
    Serial.println("Camera DC-DC off"); 
    digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW); 
    power_on = false;
  }
} 

void toggleCameraPower()
{    
  if (camera_on == true) {
    Serial.println("Camera off");
    camera_on = false;
  }
  else {
    Serial.println("Camera on. Wait 3 seconds for it to completely turn on");
    camera_on = true;
  }
  
  digitalWrite(CAMERA_ON_OFF_PIN, LOW); 
  delay(50);  
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH); 
}
 
void shutterCamera()
{    
  //Shutter Camera
  Serial.println("Camera shutter"); 
  //digitalWrite(CAMERA_FOCUS_PIN, LOW); 
  //delay(delays_msec[1]);
  digitalWrite(CAMERA_SHUTTER_PIN, LOW);
  delay(delays_msec[1]);
  digitalWrite(CAMERA_SHUTTER_PIN, HIGH);  
  digitalWrite(CAMERA_SHUTTER_PIN, LOW);

}

void focusShutterCamera()
{     
   // focus and shutter the camera
  Serial.println("Camera focus and shutter");
  digitalWrite(CAMERA_FOCUS_PIN, LOW); 
  delay(delays_msec[0]);
  digitalWrite(CAMERA_SHUTTER_PIN, LOW);
  delay(delays_msec[1]);
  digitalWrite(CAMERA_SHUTTER_PIN, HIGH);
  digitalWrite(CAMERA_FOCUS_PIN, HIGH); 
  Serial.println("Camera shuttered"); 
}

void toggleVideo()
{   
  if (video_on == true) {
    Serial.println("Video off");
    video_on = false;
    digitalWrite(CAMERA_VIDEO_PIN, LOW);
    delay(delays_msec[1]);
    digitalWrite(CAMERA_VIDEO_PIN, HIGH);
  }
  else {
    Serial.println("Video on");
    video_on = true;
    Serial.println("Camera focus");
    digitalWrite(CAMERA_FOCUS_PIN, LOW);
    delay(delays_msec[0]);  
    digitalWrite(CAMERA_FOCUS_PIN, HIGH);
    delay(delays_msec[1]);
    Serial.println("Video on");
    digitalWrite(CAMERA_VIDEO_PIN, LOW);
    delay(delays_msec[1]);
    digitalWrite(CAMERA_VIDEO_PIN, HIGH);
  }     
}
 

// the loop routine runs over and over again forever:
void loop() { 
  
  // if there's any serial available, read it:
  while (Serial.available() > 0) {
    char c = Serial.read(); 
    switch (c) {
      case 'D': {
        toggleCameraVoltage();
      break;
      }
      case 'L': { 
        for(field_index = 0; field_index < NUM_DELAY_FIELDS; field_index++) { 
          delays_msec[field_index] = Serial.parseInt();
          Serial.println(delays_msec[field_index]);
        }
        break;
      }
      case 'F': {
        focusShutterCamera();
        break;
      }  
      case 'T': {
        toggleCameraPower();
        break;
      }  
      case 'S': {
        shutterCamera();
        break;     
      }
      case 'V': {
        toggleVideo();
        break;    
      }      
      case 'C': {
        toggleCameraVoltage();
        delay(3000); //delay for DC-DC activation
        toggleCameraPower();
        delay(3000); //delay for camera boot
        focusShutterCamera();
        delay(5000); //delay for recording
        toggleCameraPower();
        toggleCameraVoltage();
        break;    
      }     
      case 'M': {
        toggleCameraVoltage();
        delay(3000); //delay for DC-DC activation
        toggleCameraPower();
        delay(15000); //delay for camera boot
        toggleVideo();
        delay(30000); //delay 30 seconds for recording
        toggleVideo();  
        delay(2000); //delay 2 seconds for SSD write        
        toggleCameraVoltage();
        break;    
      }      
    }    
  }   
}
