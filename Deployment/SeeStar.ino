/*
  SeeStar Version III
  SeeStar.ino

    Version: 1.00

  This sketch controls the SeeStar instrument which a low-cast time-lapse recording systembased on off-the-shelf cameras, 
  lights, and electronics.  It controls a single light, Sony Camera, and synchronizes with an optional external sensor module
  for recording ancillary data like temperature.

  The circuit:
  * See details for connecting the circuit here:
  * https://bitbucket.org/mbari/seestar/src/SeeStar_III/Documents/ 
  * 
  * See technical manual for the camera module which details what each signal is and how to control them here:
  * https://bitbucket.org/mbari/seestar/src/SeeStar_III/Documents/
  
  Created 12/01/2017
  By D.Cline dcline at mbari.org
  Modified 12/01/2017
  By D.Cline dcline at mbari.org
  Modified 05/02/2018
  Simplified deployment configuration and made DSCRX10 default
  By D.Cline dcline at mbari.org
  Modified 01/14/2019 Added focus before video capture
  By D.Cline dcline at mbari.org
  Modified 02/21/2019 Working video/still capture but can't confirm lighting. Merged video/still state machine into one method.
  By D.Cline dcline at mbari.org
  Modified 03/27/2019 Working video/still capture, disallow shutdown during recording to avoid SD card damage, more debugging.
  By D.Cline dcline at mbari.org
  https://bitbucket.org/mbari/seestar/src/SeeStar_III/Software/Autonomous/deploy/src/SeeStar.ino
  
  Modified by Eric Guerrero
*/
#include <SleepyPi2.h>
#include <TinyLightProtocol.h>
#include <TimeLib.h>
#include <Timezone.h>
#include <PCF8523.h>
#include <avr/pgmspace.h>    // needed from PROGMEM strings to save SRAM

// This describes the different recording modes the SeeStar can run in.  
// STILL = Single still frames, VIDEO = Video capture
typedef enum recordingMode {STILL=0, VIDEO }; 

// default camera QX1=Sony QX1 or DSCRX10=Sony Cyber-shot DSC-RX10 camera
//#define QX1_CAMERA
#define DSCRX10_CAMERA

// The version of this software/
const char *VERSION = "1.1";

////////////////////////////////////////////////////////////////////////////////////////////////////
// CHANGE THESE FOR YOUR DEPLOYMENT
////////////////////////////////////////////////////////////////////////////////////////////////////
// (optional) only used in GUI and to calculcate start_datetime/end_datetime
TimeChangeRule usLOCALDT = {"PDT", Second, dowSunday, Mar, 2, -420};
TimeChangeRule usLOCALST = {"PST", First, dowSunday, Nov, 2, -480};
Timezone myTZ(usLOCALDT, usLOCALST);

//  (optional) default time to start change to start at a specific time/date
//DateTime start_datetime =  start_datetime(2017, 12, 8, 14, 44, 0);
//DateTime end_datetime =  end_datetime(2017, 12, 8, 14, 50, 0)
//const time_t CFG_START_SECS = myTZ.toUTC(time_t(start_datetime.unixtime()))
//const time_t CFG_END_SECS = myTZ.toUTC(time_t(end_datetime.unixtime()))

//otherwise start immediately and run forever
const time_t CFG_START_SECS = 0; // starting time in seconds, set to 0 to start immediately
const time_t CFG_END_SECS = 0; // ending time in seconds, set to 0 to run forever

// default mode STILL or VIDEO
const recordingMode CFG_RECORDING_MODE = VIDEO;

// default time for interval between pictures/video
const uint8_t CFG_INTERVAL = 60; //80 30 minutes

const eTIMER_TIMEBASE CFG_TIMEBASE = eTB_MINUTE; // eTB_SECOND, eTB_MINUTE, eTB_HOUR

// default time for video capture (this is ignored in STILL mode)
const unsigned long CFG_VIDEO_DURATION_SECS = 10L;

// minimum voltage that SleepyPi can be powered with before shutting down.
// Leave this alone unless you plan on testing this with your system. This is a reasonable default.
const unsigned long CFG_POWER_MIN_SLEEPYPI_MILLVOLT = 12000L; // 12.0 V

// default minimum voltage before doing controlled power down.
// Leave this alone unless you plan on testing this with your system. This is a reasonable default
const unsigned long CFG_POWER_MIN_MILLVOLT = 12600L; // 12.6 V 

////////////////////////////////////////////////////////////////////////////////////////////////////
// These are constants that won't change 
////////////////////////////////////////////////////////////////////////////////////////////////////
// Default is to exclude the GUI comms. Uncomment this to include the serial communications between
// the sketch and the GUI. This is entirely optional, and may be more appropriate for a tethered application
// where you might want to monitor the output
// #define GUI_COMMS

// Pin I/O defaults. These define the pin mappings on the SleepyPi. These are all configured as output pins in initialize() 
const int CAMERA_POWER_ON_OFF_PIN = 6;
const int CAMERA_ON_OFF_PIN = 14;
const int CAMERA_FOCUS_PIN = 15; 
const int CAMERA_SHUTTER_PIN = 12;
const int CAMERA_VIDEO_PIN = 13;
const int LED_RELAY_CONTROL_PIN = 8; 
const int LED_CONTROL_PIN = 5; 
    
// The capture state machine states. These describe the states that are cycled through for a still/video capture 
typedef enum cam_state {CAM_INITIALIZED=0, CAM_POWER_ON, CAM_ON, CAM_BOOT, CAM_LIGHT_SYNC, CAM_RECORD, CAM_SSD_FLUSH, CAM_END, CAM_UNKNOWN};

// The intervalometer state machine states. These describe the states that are cycles through for the intervalometer
typedef enum interval_state {INITIALIZED=0, START, SHUTDOWN, SLEEP, DEPLOYMENT_END, UNKNOWN};

// Delays in capture state machine
typedef enum cam_state_delays {
  CAM_POWER_DELAY=0, // Delay in milliseconds between enabling camera DC-DC converter and camera accepting power up command
  CAM_BOOT_UP_DELAY, // Delay in milliseconds for camera to power up competely and be ready for remote control
  CAM_LIGHT_SYNC_DELAY, // Delay in milliseconds between turning the light on and shuttering 
  CAM_SSD_FLUSH_DELAY // Delay in milliseconds for images/video to be flushed and recorded to SSD card. Not delaying sufficiently long will not allow the camera
// to actually record, so modify with caution !!
};

const int NUM_CAM_STATE_DELAYS = 4;

// Camera state machine delays are different for different cameras
#ifdef QX1_CAMERA 
int state_delays_msec[NUM_CAM_STATE_DELAYS] = {3000, 22000, 1000, 5000};
#endif
#ifdef DSCRX10_CAMERA
int state_delays_msec[NUM_CAM_STATE_DELAYS] = {3000, 3000, 1000, 5000};
#endif

// The various pin control states. These are used to control the pins that control the light and camera
typedef enum enabledisable {ENABLE=0, DISABLE};
enum offon {OFF=0, ON};

// serial command string to reset the clock
const char TIME_RESET_CMD = 'T';
// serial command string to start apture 
const char START_CMD = 'S'; 
// delimeter for data sent over binary
const char DATA_DELIMITER = ';'; 
// For debugging
const boolean DEBUG = false; 

// The Realtime Clock on the SleepyPi board
PCF8523 rtc;

// This simply buffers a string for printing
char print_buffer[40];
const int BUFFER_SIZE = sizeof(print_buffer);

// This stores the instrument configuration
struct SeeStarConfig
{ 
  time_t start_secs;                // starting time in epoch seconds, or 0 if start upon power up
  time_t end_secs;                  // ending time in epoch seconds, or 0 if run indefinitely
  eTIMER_TIMEBASE timebase;         // timebase, eTB_SECOND, eTB_MINUTE, eTB_HOUR
  unsigned int interval_value;      // time between images/videos in timebase
  recordingMode mode;               // recording mode: video, still
  unsigned long video_dur_secs;     // length of video in seconds
  unsigned long power_off_mV;       // minimum battery voltage in millivolts; when voltage falls below this, system shuts down
}; 

SeeStarConfig cfg; 

// Variables that store information about timing and the state machines   
unsigned long now_msec = 0;
unsigned long prev_msec = 0;
unsigned long interval_msec = 1;
cam_state  capture_state;
cam_state  prev_capture_state;
interval_state  intervalometer_state;
interval_state  prev_intervalometer_state;

// These are placeholders for the Sensor module 
String sensor_clock="";
String sensor_log="NO DATA";

enabledisable light_power_state = DISABLE;    // stores whether power to the light is enabled/disabled; this does not mean the light is turned on
enabledisable camera_power_state = DISABLE;   // true if power to the camera is on; this does not mean the camera is turned on
offon camera_state = OFF;     // true if the camera is on
offon light_state = OFF;      // true if the light is on
offon video_state = OFF;      // true if the video recording is on

boolean low_power_shutdown = false; // true if supply voltage too low; disables capture
boolean config_err = false;         // true if an error in the instrument configuration
unsigned long supply_voltage_mV = 0; // current reading of the supply voltage in millivolts
int num_pics = 0;                   // total number of pictures/videos taken
unsigned long next_wakeup_secs;     // next time to wakeup
unsigned long last_shutdown_secs;   // last time shutdown 
int counter;

////////////////////////////////////////////////////////////////////////////////////////////////////
// These are all serial communication related variables for communicating between the SeeStar and your computer
#ifdef GUI_COMMS
Tiny::ProtoLight  proto; 
const char DATA_DELIMETER = ';';
const char PACKET_DELIMETER = '~';
const int PACKET_LENGTH = 128; 
// Buffer for packets to send and receive serial data
byte g_inBuf[PACKET_LENGTH];
byte g_outBuf[PACKET_LENGTH];
Tiny::Packet outPacket(g_outBuf, sizeof(g_outBuf));
Tiny::Packet inPacket(g_inBuf, sizeof(g_inBuf));
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////
// The setup routine runs once when you press reset, or anytime power is cycled
void setup() {    
  
  // Initializes the pins that control the light and camera   
  // Initialize each digital pin as an output versus an input
  pinMode(LED_RELAY_CONTROL_PIN, OUTPUT);    
  pinMode(LED_CONTROL_PIN, OUTPUT);  
  pinMode(CAMERA_ON_OFF_PIN, OUTPUT); 
  pinMode(CAMERA_SHUTTER_PIN, OUTPUT);
  pinMode(CAMERA_FOCUS_PIN, OUTPUT); 
  pinMode(CAMERA_VIDEO_PIN, OUTPUT); 
  pinMode(CAMERA_POWER_ON_OFF_PIN, OUTPUT);     

  // are not using the RPI board in this, so disable power for it
  SleepyPi.enablePiPower(false);     
  
  // Initialize serial port
  Serial.begin(9600);
  // No timeout, since we want non-blocking UART operations
  Serial.setTimeout(0);
  delay(50);  
  
  SleepyPi.rtcInit(true);
  
  // ensure the LED relay is off
  digitalWrite(LED_CONTROL_PIN, HIGH); // set to HIGH initially; turns the LED on with a falling edge, e.g. HIGH->LOW  
  SleepyPi.enableExtPower(true);
  digitalWrite(LED_RELAY_CONTROL_PIN, LOW); 
  delay(100);   
  digitalWrite(LED_RELAY_CONTROL_PIN, HIGH); // set to HIGH initially; turns the relay on with a falling edge, e.g. HIGH->LOW     
  SleepyPi.enableExtPower(false);  
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH); // set to HIGHOW initially; turns on/off the camera by pulsing low briefly, i.e. HIGH->LOW->HIGH
  digitalWrite(CAMERA_SHUTTER_PIN, HIGH);  // set to HIGH initially; shutters the camera by pulsing low briefly, i.e. HIGH->LOW->HIGH 
  digitalWrite(CAMERA_FOCUS_PIN, HIGH);  // set to HIGH initially; shutters the camera by pulsing low briefly, i.e. HIGH->LOW->HIGH 
  digitalWrite(CAMERA_VIDEO_PIN, LOW);// set to LOW initially; shutters the camera video mode on/off by pulsing low briefly, i.e. HIGH->LOW->HIGH,
   // but this also enables the onboard led which we want off so leave LOW
  digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);  // set to LOW initially;  pulling this high will enable power to the camera, but not turn it on.

  // initialize the states for the state machines
  capture_state = CAM_UNKNOWN;
  prev_capture_state = CAM_UNKNOWN;  
  intervalometer_state = INITIALIZED;
  prev_intervalometer_state = UNKNOWN;
  
  // Redirect protocol communication to Serial0 UART
  #ifdef GUI_COMMS
  outPacket.clear();
  inPacket.clear();  
  proto.beginToSerial();
  #endif
  
  // external time provided through the readRTC function, which reads from the realtime clock on the SleepyPi
  setSyncProvider(readRTC);
  DateTime t = rtc.readTime(); 
  last_shutdown_secs  = t.unixtime();
 
  // Initialize instrument configuration with some reasonable defaults   
  cfg.start_secs = CFG_START_SECS;
  cfg.end_secs = CFG_END_SECS;
  // uncomment below to end at an exact time
  //cfg.end_secs = myTZ.toUTC(time_t(end_datetime.unixtime())) 
  cfg.timebase = CFG_TIMEBASE;  // time value of the interval_value, (eTB_SECOND, eTB_MINUTE, or eTB_HOUR)
  cfg.interval_value = CFG_INTERVAL; // interval_value = 30 seconds (if eTB_SECOND), 30 minutes (if eTB_MINUTE)
  cfg.mode = CFG_RECORDING_MODE; 
  if (cfg.mode == VIDEO)
    cfg.video_dur_secs = CFG_VIDEO_DURATION_SECS;
  else
    cfg.video_dur_secs = 0L;
  cfg.power_off_mV = CFG_POWER_MIN_MILLVOLT;
  checkConfiguration();
  
  counter = 0;
  Serial.println("Setup complete");
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// The main control loop. This runs right after setup() is executed
void loop() {
  if (DEBUG == false) 
    delay(50);
  else
    delay(2000);
  #ifdef GUI_COMMS
  outPacket.clear();
  inPacket.clear(); 
  sendBinaryStatusPacket(); 
  processSerial();  
  outPacket.clear();
  inPacket.clear();
  #endif
  // only check voltage every 200 iterations
  if ( (counter++ % 200) == 0 )
   checkVoltage();
  reportState(); 
  runIntervalometer();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Runs state machine for the intervalometer
void runIntervalometer() {
  prev_intervalometer_state = intervalometer_state;

  DateTime t = SleepyPi.readTime();
  time_t secs_now = t.unixtime(); 
  
  if (config_err == true)
    intervalometer_state = SHUTDOWN;

  // don't allow shutdown during recording or can damage the SD card
  if (low_power_shutdown == true)
    if ((capture_state == CAM_RECORD  || capture_state == CAM_SSD_FLUSH))
      low_power_shutdown = false;
    else
      if (intervalometer_state != SLEEP && intervalometer_state != SHUTDOWN)
        intervalometer_state = SHUTDOWN;
        
  switch (intervalometer_state) {
    case INITIALIZED:  
      // if within the deployment period, start normal loop
      if ((cfg.start_secs  == 0 || secs_now >  cfg.start_secs) && (cfg.end_secs  == 0 || secs_now < cfg.end_secs )) {
        intervalometer_state = START;    
        capture_state = CAM_INITIALIZED;
        next_wakeup_secs = secs_now + calcIntervalSeconds();
      }
      break;
    case START:
      if (capture_state == CAM_END) {       
        intervalometer_state = SHUTDOWN; 
        prev_capture_state = CAM_END;
      }
      else
        runCapture();
      break; 
    case SHUTDOWN:
      powerShutdown();
      if (low_power_shutdown == true)
        intervalometer_state = SLEEP;
      else {
        // if not running forever and outside the deployment window, end
        if ( (cfg.start_secs != 0 && secs_now < cfg.start_secs) || (cfg.end_secs !=0 && secs_now > cfg.end_secs ))
          intervalometer_state = DEPLOYMENT_END;      
          
        // if within the deployment period, go to sleep, then start again
        if ((cfg.start_secs  == 0 || secs_now >  cfg.start_secs) && (cfg.end_secs  == 0 || secs_now < cfg.end_secs ))
          intervalometer_state = SLEEP;
      }
      break;  
    case SLEEP:
      runSleepCycle();      
      capture_state = CAM_UNKNOWN;
      prev_capture_state = CAM_UNKNOWN;
      intervalometer_state = INITIALIZED;
      prev_intervalometer_state = UNKNOWN;
      break;
    case DEPLOYMENT_END:
      break;
    case UNKNOWN:
      intervalometer_state = UNKNOWN;
      capture_state = CAM_UNKNOWN;
      break; 
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Processes serial commands
void processSerial() {
  #ifdef GUI_COMMS
  int len;
  unsigned long start = millis();
  do {
    len = proto.read(inPacket); 
    // Wait 500ms for any answer from remote side
    if (millis() - start >= 500)
        break; 
  } while (len < 0);            
  if (len > 0)   {
    
    // Process response from remote side
    char cmd = inPacket.getChar();
    inPacket.clear();  
    switch (cmd)  {
      case TIME_RESET_CMD: {
        uint32_t secs = inPacket.getUint32();
        setTime(secs); 
        DateTime dt(secs);
        SleepyPi.setTime(dt); 
        break;   
      }
      case START_CMD:  {  
        Serial.println("Setting start time");
        DateTime t = rtc.readTime(); 
        cfg.start_secs = t.unixtime();  
        // if time for ending is not set, set it to something reasonable
        if (cfg.end_secs < cfg.start_secs)
          cfg.end_secs = cfg.start_secs + 5*CFG_INTERVAL; 
      break;  
      }  
    }    
  }
  #endif
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Runs state machine for capture
void runCapture() {
  prev_capture_state = capture_state;
  
  switch (capture_state) {
    case CAM_INITIALIZED:  
      capture_state = CAM_POWER_ON; 
      break;
    case CAM_POWER_ON:
      cameraVoltage(ENABLE);
      capture_state = CAM_ON;
      initTimeout(); 
      break;
    case CAM_ON:
      if(delayTimeout(state_delays_msec[CAM_POWER_DELAY])) {
        turnCamera(ON);
        capture_state = CAM_BOOT;
        initTimeout();
      }
      break;    
    case CAM_BOOT:
      if(delayTimeout(state_delays_msec[CAM_BOOT_UP_DELAY])) {
        capture_state = CAM_LIGHT_SYNC;
        initTimeout();
        lightVoltage(ENABLE);
        turnLight(ON);
      }
      break;
    case CAM_LIGHT_SYNC:
      if(delayTimeout(state_delays_msec[CAM_LIGHT_SYNC_DELAY])) {
        capture_state = CAM_RECORD;
        if (cfg.mode == STILL){
            #ifdef CFG_CAMERA_TYPE == QX1
            shutterCamera();
            #else
            focusShutterCamera();
            #endif
            }
        else
          turnVideo(ON);
        initTimeout();
      }
      break;
    case CAM_RECORD:
      if(delayTimeout(cfg.video_dur_secs*1000L + 1000L)) { //1 second offset in recording delay and actual recorded files ****
         if (cfg.mode == VIDEO)
               turnVideo(OFF);
          turnLight(OFF);
          lightVoltage(DISABLE);
          initTimeout();
          capture_state = CAM_SSD_FLUSH;
      }
      break; 
    case CAM_SSD_FLUSH:
      if (delayTimeout(state_delays_msec[CAM_SSD_FLUSH_DELAY]))
          capture_state = CAM_END;
      break;
    case CAM_END:
      turnCamera(OFF);
      cameraVoltage(DISABLE);
      break;
    case CAM_UNKNOWN: 
      break; 
  } 
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Reads the time from the RTC
time_t readRTC(void) { 
  DateTime t; 
  t = SleepyPi.readTime();
  return t.unixtime();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Send status update as binary packet. This is intended to be received by the gui.pd processing script
void sendBinaryStatusPacket() {
  #ifdef GUI_COMMS
  time_t utc, local, wakeup, sensor_clock;  
  Serial.println("");
  utc = now();
  local= myTZ.toLocal(utc); 
  sensor_clock = utc;  // simulate clocks that are in sync 
  wakeup = utc + 5*60;
  outPacket.put(local);
  outPacket.put(DATA_DELIMITER); 
  outPacket.put(utc);
  outPacket.put(DATA_DELIMITER); 
  outPacket.put(wakeup);
  outPacket.put(DATA_DELIMITER); 
  outPacket.put((int) (supply_voltage_mV)); 
  outPacket.put(DATA_DELIMETER);  
  outPacket.put((int) (cfg.power_off_mV));  
  outPacket.put(DATA_DELIMETER);
  outPacket.put(num_pics); 
  outPacket.put(DATA_DELIMETER); 
  outPacket.put(cfg.mode); 
  outPacket.put(DATA_DELIMETER);  
  outPacket.put(capture_state); 
  outPacket.put(DATA_DELIMETER);  
  outPacket.put(cfg.video_dur_secs);
  outPacket.put(DATA_DELIMETER);   
  outPacket.put(sensor_clock);  
  outPacket.put(DATA_DELIMETER);    
  for (int i=0;i< sensor_log.length(); i++) // copy string array
    outPacket.put(sensor_log[i]);   
  outPacket.put(DATA_DELIMETER);

  // Send packet over UART to the receiving computer
  proto.write(outPacket); 
  Serial.println("");
  #endif
} 
////////////////////////////////////////////////////////////////////////////////////////////////////
// Return true if the timeout delay in milliseconds has elapsed
boolean delayTimeout(unsigned long msec)
{ 
  now_msec = millis(); 
  if ((unsigned long)(now_msec - prev_msec) > msec) {
    prev_msec = now_msec;
    return true;
  }
  return false;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Initializes timeout for needed delays in the state machine
void initTimeout(void) {
  prev_msec = millis();
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Holds down the focus line and shutters the camera. Currently unused, but included in case
// one wants to use the camera autofocus
void focusShutterCamera() {    
   // focus and shutter the camera
  Serial.println("Camera focus and shutter");
  if (DEBUG == false) {
  digitalWrite(CAMERA_FOCUS_PIN, LOW);
  delay(4000);
  digitalWrite(CAMERA_SHUTTER_PIN, LOW);
  delay(100);
  digitalWrite(CAMERA_SHUTTER_PIN, HIGH);  
  digitalWrite(CAMERA_FOCUS_PIN, HIGH);
  }
  num_pics += 1;  
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Shutters the camera
void shutterCamera() {
  if (DEBUG == false) {
  Serial.println("Camera shutter");
  digitalWrite(CAMERA_SHUTTER_PIN, LOW);
  delay(1000); 
  digitalWrite(CAMERA_SHUTTER_PIN, HIGH);
  }
  num_pics += 1;  
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Turns the video mode on the camera on/off
void turnVideo(offon state) {
  if (video_state == ON && state == OFF) {
    video_state = OFF;
    Serial.println("Video off");
    if (DEBUG == false) {
    digitalWrite(CAMERA_VIDEO_PIN, HIGH);
    digitalWrite(CAMERA_VIDEO_PIN, LOW);
    delay(500);
    digitalWrite(CAMERA_VIDEO_PIN, HIGH);
    }
  }
  if (video_state == OFF && state == ON) {
    video_state = ON;
    Serial.println("Camera focus");
    if (DEBUG == false) {
    digitalWrite(CAMERA_FOCUS_PIN, LOW);
    delay(4000);  
    digitalWrite(CAMERA_FOCUS_PIN, HIGH);
    Serial.println("Video on");
    digitalWrite(CAMERA_VIDEO_PIN, HIGH);
    digitalWrite(CAMERA_VIDEO_PIN, LOW);
    delay(100);
    digitalWrite(CAMERA_VIDEO_PIN, HIGH);
    }
    num_pics += 1; 
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Turns on/off the camera
void turnCamera(offon state) { 
  if (camera_state == ON && state == OFF) {
    camera_state = OFF;
    Serial.println("Camera off");
  }
  if (camera_state == OFF && state == ON) {
    camera_state = ON;
    Serial.println("Camera on");
  }  
  if (DEBUG == false) {
  digitalWrite(CAMERA_ON_OFF_PIN, LOW); 
  delay(200);
  digitalWrite(CAMERA_ON_OFF_PIN, HIGH);
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Toggles the camera DC-DC voltage converter on/off. This is the device that converts the battery voltage
// to the 8V the Sony camera requires
void cameraVoltage(enabledisable state) {
  if (camera_power_state == DISABLE && state == ENABLE) {
    Serial.println("Camera DC-DC on");
    if (DEBUG == false)
      digitalWrite(CAMERA_POWER_ON_OFF_PIN, HIGH);
    camera_power_state = ENABLE;
  }
  if (camera_power_state == ENABLE && state == DISABLE) {
    Serial.println("Camera DC-DC off");
    if (DEBUG == false)
      digitalWrite(CAMERA_POWER_ON_OFF_PIN, LOW);
    camera_power_state = DISABLE;
  }
}
////////////////////////////////////////////////////////////////////////////////////////////////////
// Enable/disable voltage to the LED light
void lightVoltage(enabledisable state) {
  if (light_power_state == DISABLE && state == ENABLE) {
    light_power_state = ENABLE;    
    Serial.println("Turning LED relay on");
    if (DEBUG == false) {
    SleepyPi.enableExtPower(true);
    digitalWrite(LED_RELAY_CONTROL_PIN, LOW);
    }
  }
  if (light_power_state == ENABLE && state == DISABLE) {
    light_power_state = DISABLE;
    if (DEBUG == false) {    
    Serial.println("Turning LED relay off");   
    digitalWrite(LED_RELAY_CONTROL_PIN, HIGH); 
    delay(100);
    SleepyPi.enableExtPower(false);
    }
  }
}  
////////////////////////////////////////////////////////////////////////////////////////////////////
// Turns the LED light on/off
void turnLight(offon state) {       
  if (light_state == OFF && state == ON) {
    light_state = ON;
    Serial.println("Turning LED light on");
  }
  if (light_state == ON && state == OFF) {
    light_state = OFF;
    Serial.println("Turning LED light off");
  }
  if (DEBUG == false) {
  digitalWrite(LED_CONTROL_PIN, LOW);  
  delay(200);
  digitalWrite(LED_CONTROL_PIN, HIGH);  
  }
}  
///////////////////////////////////////////////////////////////////////////////////////////////////
// Turns off the camera and lights in the correct order
void powerShutdown() {
  turnLight(OFF);
  lightVoltage(DISABLE);
  turnCamera(OFF);
  turnVideo(OFF);
  cameraVoltage(DISABLE);
  digitalWrite(CAMERA_VIDEO_PIN, LOW); //Turns LED off
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Prints out the instrument state to a serial port
void reportState() {
  
  // only report on a state change 
  if (prev_intervalometer_state != intervalometer_state) { 
    switch (intervalometer_state) {
      case INITIALIZED:
        Serial.println("INITIALIZED");
        break;
      case START:
        Serial.println("START");
        break;
      case SHUTDOWN:
        Serial.println("SHUTDOWN");
        break; 
      case SLEEP:
        Serial.println("SLEEP");
        break;
      case DEPLOYMENT_END:
        Serial.println("DEPLOYMENT_END");
        break;
      case UNKNOWN:
        Serial.println("UNKNOWN");
        break;
    }
  }  
  // only report on a state change 
  if (prev_capture_state != capture_state) { 
    switch (capture_state) {
      case CAM_INITIALIZED:   
        Serial.println("CAM_INITIALIZED");
        break;
      case CAM_POWER_ON:    
        Serial.print("CAM_POWER_ON, DELAYING FOR CAMERA ACTIVATION ");
        Serial.print(state_delays_msec[CAM_POWER_DELAY]);    
        Serial.println(" milliseconds");
        break;
      case CAM_ON: 
        Serial.println("CAMERA ON");
        break;
      case CAM_BOOT:      
        Serial.print("CAMERA BOOT, DELAYING FOR CAMERA BOOT ");
        Serial.print(state_delays_msec[CAM_BOOT_UP_DELAY]);  
        Serial.println(" milliseconds");
        break;
      case CAM_LIGHT_SYNC:
        Serial.print("CAM_LIGHT_SYNC, DELAYING FOR CAMERA LIGHT SYNC ");
        Serial.print(state_delays_msec[CAM_LIGHT_SYNC_DELAY]);
        Serial.println(" milliseconds");
        break;
      case CAM_RECORD:
        if (cfg.mode == VIDEO) {
          Serial.print("CAM_RECORD, CAMERA VIDEO RECORDING ");
          Serial.print(cfg.video_dur_secs);
          Serial.println(" seconds");
        }
        else
          Serial.println("CAM_RECORD, CAMERA SHUTTER");
        break;
      case CAM_SSD_FLUSH:
        Serial.print("CAM_SSD_FLUSH, DELAYING FOR CAMERA TO COMPLETE WRITE TO SSD CARD ");  ; 
        Serial.print(state_delays_msec[CAM_SSD_FLUSH_DELAY]);  
        Serial.println(" milliseconds");
        break;      
      case CAM_END:
        Serial.println("CAM_END");
        break; 
      case CAM_UNKNOWN:
        Serial.println("CAM_UNKNOWN");
        break; 
    }
  } 
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Alarm interrupt service routine. Currently unused, but included for future use
void alarm_isr() {
  // Just a handler for the alarm interrupt.
  // You could do something here
} 
//////////////////////////////////////////////////////////////////////////////////////////////////
// Checks if the voltage has fallen below a usable level and goes into low power shutdown
void checkVoltage() {          
  supply_voltage_mV = SleepyPi.supplyVoltage()*1000L;
  if(supply_voltage_mV < cfg.power_off_mV)
    low_power_shutdown = true;
  else 
    low_power_shutdown = false;
  Serial.print("Power supply: ");
  Serial.print(supply_voltage_mV);
  Serial.print(" mV");
  Serial.print(";minimum before shutdown: ");
  Serial.print(cfg.power_off_mV);
  Serial.println(" mV");
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Helper function to print the time
void printTime(time_t seconds, const char* str) {
  snprintf(print_buffer, BUFFER_SIZE, "%s %d.%02d.%02d %02d:%02d:%02d", str, year(seconds),month(seconds),day(seconds),
                    hour(seconds),minute(seconds),second(seconds));
  Serial.println(print_buffer);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Runs the intervalometer sleep cycle.  Puts the instrument to sleep, according to the defined start and end times
// in the configuration. If outside of those times, or there is a configuration error, puts the instrument to sleep every 30 seconds 
void runSleepCycle() {    
  DateTime t = SleepyPi.readTime();
  time_t secs_now = t.unixtime();

  printTime(secs_now, "Time now ");  
  if (cfg.start_secs != 0) 
    printTime(cfg.start_secs, "Start time ");
  else
    Serial.println("Start time NOW");
  if (cfg.end_secs != 0) 
    printTime(cfg.end_secs, "End time");
  else
    Serial.println("End time NEVER");

  unsigned long delay_secs;

  delay_secs =  calcIntervalSeconds();

  printTime(next_wakeup_secs, "Next wakeup");
  printTime(last_shutdown_secs, "Last shutdown"); 
  last_shutdown_secs = secs_now; 
  if (delay_secs > 0) { 
    SleepyPi.rtcClearInterrupts(); 
    attachInterrupt(0, alarm_isr, FALLING);    // Alarm pin     
    if (next_wakeup_secs != 0)
      printTime(next_wakeup_secs, "Next wakeup time"); 
    else
      Serial.println("Wakeup UNDEFINED");  
    Serial.print("Shutting down for ");
    printTimebase();
    if (DEBUG == false) {
      Serial.println("Shutting down now");
      Serial.flush();
      SleepyPi.setTimer1(cfg.timebase, cfg.interval_value);
      SleepyPi.powerDown(SLEEP_FOREVER, ADC_OFF, BOD_OFF);  
      detachInterrupt(0);
      SleepyPi.ackTimer1();
    }
    else {
      Serial.println("Simulated shutdown");
      delay(delay_secs*1000);
      Serial.println("Simulated wakeup");
      capture_state = CAM_UNKNOWN;
      prev_capture_state = CAM_UNKNOWN;  
      intervalometer_state = INITIALIZED;
      prev_intervalometer_state = UNKNOWN;
    }
  }   
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Utility function to print the timebase for the intervalometer 
void printTimebase() {
  Serial.print(cfg.interval_value, DEC);
  switch(cfg.timebase) {
    case eTB_SECOND:
      Serial.println(" seconds");
      break;
    case eTB_MINUTE:
      Serial.println(" minutes");
      break;
    case eTB_HOUR:
      Serial.println(" hours");
      break;
    default:
      Serial.println(" unknown timebase");
      break;
  }
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Utility function to calculcate the total seconds given the timebase for the intervalometer 
unsigned long calcIntervalSeconds()
{
  unsigned long seconds = 0L; 
  switch(cfg.timebase) {
    case eTB_SECOND:
      seconds = cfg.interval_value;
      break;
    case eTB_MINUTE:
      seconds = cfg.interval_value*60L; 
      break;
    case eTB_HOUR:
      seconds = cfg.interval_value*60L*60L; 
      break;
    default:
      seconds = cfg.interval_value;
      break;
  }
  return seconds;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
// Check configuration to make sure there are no errors
void checkConfiguration() {      
  
  Serial.println("\n=======SeeStar III Configuration=======");     
  if (cfg.start_secs != 0) 
    printTime(cfg.start_secs, "Start time");
  else
    Serial.println("Start time NOW");
  if (cfg.end_secs != 0) 
    printTime(cfg.end_secs, "End time");
  else
    Serial.println("End time NEVER");

  if (intervalometer_state == DEPLOYMENT_END)
    Serial.println("DEPLOYMENT ENDED"); 
  
  Serial.print("Intervalometer period: ");
  printTimebase();     

  if (cfg.mode == VIDEO) {
    Serial.println("Mode: VIDEO"); 
    Serial.print("Video duration: ");
    Serial.print( cfg.video_dur_secs);
    Serial.println(" seconds"); 
  }
  else
    Serial.println("Mode: STILL");

  Serial.print("Minimum power before shutdown: ");
  Serial.print(cfg.power_off_mV);
  Serial.println(" mV"); 
  
  Serial.println("=======================================");

  unsigned long interval_secs = 0;
  interval_secs = calcIntervalSeconds();
  unsigned long deployment_secs = cfg.end_secs - cfg.start_secs;
  unsigned long min_interval = 2000L;
 
  // the minimum intervalometer cycle is the sum of all the delays and a few seconds for Arduino overhead
  for(int i=0; i < NUM_CAM_STATE_DELAYS; i++)
    min_interval += state_delays_msec[i];
  
  min_interval /= 1000L; //convert milliseconds to seconds

  // add in the video recording overhead if in video mode
  if (cfg.mode == VIDEO)
    min_interval += cfg.video_dur_secs;
    
 if (cfg.start_secs != 0 && cfg.end_secs != 0 && deployment_secs <= min_interval) {  
    Serial.print("Error: ");
    Serial.print(deployment_secs);
    Serial.print(" second deployment duration less than minimum interval ");
    Serial.print(min_interval);
    Serial.println(" second minimum intervalometer cycle, please increase");  
    config_err = true;
  }
  if (interval_secs <= min_interval) {
    Serial.print("Error: ");
    Serial.print(interval_secs);
    Serial.print(" second interval less than ");
    Serial.print(min_interval);
    Serial.println(" second minimum intervalometer cycle, please increase"); 
    config_err = true;
  }
  if (cfg.power_off_mV <= CFG_POWER_MIN_SLEEPYPI_MILLVOLT) {
    Serial.print("Error: shutoff voltage ");    
    Serial.print(cfg.power_off_mV);    
    Serial.print(" mV too low; must be greater than ");    
    Serial.println(CFG_POWER_MIN_SLEEPYPI_MILLVOLT);      
    config_err = true;
  }
}  
